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

GtkWidget *make_menu_from_dir(char *path, int(*entrytest)(char* path), char *current)
{
  DIR *dir;
  struct dirent *entry;
  GtkWidget *menu = gtk_menu_new();
  int i = 0, selected = 0;
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
	  if(!entrytest(temp))
	    {
	      g_free(temp);
	    }
	  else
	    {
	      menuitem = gtk_menu_item_new_with_label (entry->d_name);
	      gtk_object_set_data_full (GTK_OBJECT (menuitem), "fullpath", temp,
					(GtkDestroyNotify) gtk_widget_unref);  
	      // ?? I dont know really if gtk_widget_unref release temp..
	      i++;
	      gtk_widget_show (menuitem);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      if(current != 0 && strcmp(current,temp)==0)
		  selected = i-1;
	    }
	}
      closedir (dir);
    }
  gtk_menu_set_active(GTK_MENU(menu),selected);
  return menu;
}
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
