/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#define _XOPEN_SOURCE /* Pour GlibC2 */
#include <time.h>
#include "applets.h"
#include "gpe/gtkminifilesel.h"
#include "gpe/gtkminifilesel.h"
#include "gpe/errorbox.h"


/***************************************************************************************/
/* Return 1 if a file exists & can be read, 0 otherwise.*/

int file_exists (char *fn)
{
	FILE *inp;

	inp = fopen (fn, "r");
	if (inp)
	{
		fclose (inp);
		return 1;
	}
	return 0;
}

/***************************************************************************************/

GtkWidget *make_menu_from_dir(char *path, int(*entrytest)(char* path), char *current)
{
  DIR *dir;
  struct dirent *entry;
  GtkWidget *menu = gtk_menu_new();
  int i = 0, selected = -1;
  dir = opendir (path);
  if(dir)
    {
      while ((entry = readdir (dir)))
	{
	  char *temp;
	  GtkWidget *menuitem;

	  if (entry->d_name[0] == '.') 
	    continue;
	  
	  /* read the file if we don't want to ignore it */
	  temp = g_strdup_printf ("%s/%s",path , entry->d_name);
	  if(entrytest!=0 && !entrytest(temp))
	    {
	      g_free(temp);
	    }
	  else
	    {
	      menuitem = gtk_menu_item_new_with_label (entry->d_name);
	      gtk_object_set_data_full (GTK_OBJECT (menuitem), "fullpath", temp,
					(GtkDestroyNotify) g_free);  
	      i++;
	      gtk_widget_show (menuitem);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      if(current != 0 && strcmp(current,temp)==0)
		  selected = i-1;
	    }
	}
      closedir (dir);
    }
  if(selected !=-1)
    gtk_menu_set_active(GTK_MENU(menu),selected);
  return menu;
}
/***************************************************************************************/
int mystrcmp(char *s,char*c)
{
  int i=0;
  while(*s ==' ')
    {  s++; i++;}
  while(*c)
    {
      if(*s!=*c)
	return -1;
      s++;c++;i++;
    }
  return i;
}

/***************************************************************************************/


// Maybe this should be in libgpewidget
// but, this doesnt fit in the gtk "way of life"

struct fstruct
{
  GtkWidget *fs;
  void (*File_Selected)(char *filename, gpointer data);  
  void (*Cancel)(gpointer data);
  gpointer data;
};

static void
select_fs                                (GtkButton       *button,
                                        gpointer         user_data)
{
  char * file;
  struct fstruct *s = (struct fstruct *)user_data;

  file=gtk_mini_file_selection_get_filename(GTK_MINI_FILE_SELECTION(s->fs));
  s->File_Selected(file,s->data);
  gtk_widget_destroy(GTK_WIDGET(s->fs));
}

static void
cancel_fs                               (GtkButton       *button,
                                        gpointer         user_data)
{
  struct fstruct *s = (struct fstruct *)user_data;

  if(s->Cancel)
    s->Cancel(s->data);

  gtk_widget_destroy(GTK_WIDGET(s->fs));
}
static void
freedata                               (GtkButton       *button,
                                        gpointer         user_data)
{
  struct fstruct *s = (struct fstruct *)user_data;
  free(s);

}
void ask_user_a_file(char *path, char *prompt,
		     void (*File_Selected)(char *filename, gpointer data),
		     void (*Cancel)(gpointer data),
		     gpointer data)
{
  char buf[1024];
  char * ret = getcwd (buf, 1024);
  if(path)                         // this is a hack, we're all waiting a gtk_mini_file_selection_change_directory().. (TODO)
    chdir(path);
  {
    GtkWidget *fileselection1 = gtk_mini_file_selection_new (prompt ? prompt : "Select File");
    GtkWidget *ok_button1 = GTK_MINI_FILE_SELECTION (fileselection1)->ok_button;
    GtkWidget *cancel_button1 = GTK_MINI_FILE_SELECTION (fileselection1)->cancel_button;
    struct fstruct *s= (struct fstruct *)malloc(sizeof(struct fstruct));
    if(ret)
      chdir(buf);
    s->fs = fileselection1;
    s->File_Selected = File_Selected;
    s->Cancel=Cancel;
    s->data=data;
  

    GTK_WINDOW (fileselection1)->type = GTK_WINDOW_DIALOG;

    
    gtk_widget_show (ok_button1);
    gtk_signal_connect (GTK_OBJECT (ok_button1), "clicked",
			GTK_SIGNAL_FUNC(select_fs),
			(gpointer)s);

    gtk_widget_show (cancel_button1);
    gtk_signal_connect (GTK_OBJECT (cancel_button1), "clicked",
			GTK_SIGNAL_FUNC(cancel_fs),
			(gpointer)s);

    gtk_signal_connect (GTK_OBJECT(s->fs) , "destroy", 
			(GtkSignalFunc) freedata, (gpointer)s); // in case of destruction by close (X) button

    gtk_widget_show (s->fs);
  }
}



/***************************************************************************************/


int system_and_gfree(gchar *cmd)
{
  int rv;
  gchar *buf;
  rv = system(cmd);
  if(rv != 0)
    {
      buf = g_strdup_printf("%s\n failed with return code %d\nperhaps you have \nmisinstalled something",cmd, rv);
      gpe_error_box(buf);
      g_free(buf);
    }
  g_free(cmd);
}
