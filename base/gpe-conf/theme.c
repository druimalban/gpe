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
#include "theme.h"
#include "gpe/errorbox.h"
#include "gpe/question.h"

static struct
{
  GtkWidget *MatchboxMenu;
  GtkWidget *GtkMenu;
  GtkWidget *WallPaper;
}self;

char * get_cur_matchboxtheme()
{
  FILE *pipe;
  static char cur[256];
  pipe = popen ("mbcontrol -r", "r");
  
  if (pipe > 0)
    {
      fgets (cur, 255, pipe);
      cur[strlen(cur)-1] = 0; //remove the last \n
      pclose (pipe);
    }
  else
    {
      gpe_error_box( "couldn't get matchbox theme\n");
    }

  return cur;

}
void get_gtkrc(char *cur)
{
  char *home = getenv("HOME");
  if(strlen(home) > 240)
      gpe_error_box( "bad $HOME !!");

  sprintf(cur,"%s/.gtkrc",home);

}
char * get_cur_gtktheme()
{
  FILE *f;
  static char cur[256];
  int found = 0;
  get_gtkrc(cur);
  f = fopen (cur, "r");
  
  if (f > 0)
    {
      while ((feof(f) == 0) && (!found))
	{
	  char buffer[256], buffer2[256];
	  fgets (buffer, 255, f);
	  if (sscanf (buffer, "include \"%s", buffer2) > 0)
	    {
	      int i,slashcount=0;
	      for( i=strlen(buffer2)-1; i > 0 && slashcount < 2;i--)
		if(buffer2[i] == '/')
		  slashcount++;
	      cur[i+1]=0;
	      for( ; i >= 0 ;i--)
		cur[i] = buffer2[i];
	      found = 1;
	    }

	}
      fclose (f);
    }

  if(f==0 || !found)
    {
      gpe_error_box( "your .gtkrc seems to be edited, change gtk theme at your own risk\n");
      return 0;
    }
  return cur;

}

void
on_matchbox_entry_changed              (GtkWidget     *menu,
                                        gpointer         user_data)
{

  GtkWidget *active=gtk_menu_get_active(GTK_MENU(menu));
  char * fullpath= gtk_object_get_data(GTK_OBJECT(active),"fullpath");
  char buf[255];
  //  fork_exec("mbcontrol" "-t" fullpath); TODO
  sprintf(buf,"mbcontrol -t %s",fullpath);
#if __i386__
  printf("%s\n",buf);
#else
  system(buf);
#endif

}

void
on_gtk_entry_changed              (GtkWidget     *menu,
                                        gpointer         user_data)
{
  GdkEventClient rcevent;
  GtkWidget *active=gtk_menu_get_active(GTK_MENU(menu));
  char * fullpath= gtk_object_get_data(GTK_OBJECT(active),"fullpath");
  char rc[256];
  get_gtkrc(rc);

  
 if(get_cur_gtktheme() || 1/*!gpe_question_ask_yn ("your .gtkrc seems \nto have been edited.\n do you really want\n to reset it?")*/) // there seems to be a problem with the current question.c
    {
      FILE *f = fopen(rc,"w");
      //  fork_exec("mbcontrol" "-t" fullpath); TODO
      fprintf(f,"# -- THEME AUTO-WRITTEN DO NOT EDIT\n");
      fprintf(f,"include \"%s/gtk/gtkrc\"\n\ninclude \"%s.mine\"\n",fullpath,rc);
      fprintf(f,"# -- THEME AUTO-WRITTEN DO NOT EDIT\n");
      fclose(f);
      
      rcevent.type = GDK_CLIENT_EVENT;
      rcevent.window = 0;
      rcevent.send_event = TRUE;
      rcevent.message_type = gdk_atom_intern ("_GTK_READ_RCFILES", FALSE);
      rcevent.data_format = 8;
      gdk_event_send_clientmessage_toall ((GdkEvent *)&rcevent);
      gdk_flush ();
    }
}
int gtk_entry_test(char *path)
{
  char buf[256];
  if(strlen(path)< 256 - 10)
    {
      sprintf(buf,"%s/gtk/gtkrc",path);
      return file_exists(buf);
    }
  return 0;
}
int void_entry_test(char *path)
{
  return 1;
}
static char * matchboxpath = "/usr/share/matchbox/themes";
static char * gtkpath = "/usr/share/themes";

GtkWidget *Theme_Build_Objects()
{
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *menu ;
 
  frame = gtk_frame_new (_("global apparence:"));

  table = gtk_table_new(3,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
    
  label = gtk_label_new(_("Matchbox Theme:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);

  hbox2 = gtk_hbutton_box_new();
  self.MatchboxMenu = gtk_option_menu_new();
  gtk_container_add(GTK_CONTAINER(hbox2),self.MatchboxMenu);

  menu =  make_menu_from_dir (matchboxpath, &void_entry_test, get_cur_matchboxtheme() );

  gtk_option_menu_set_menu (GTK_OPTION_MENU (self.MatchboxMenu),menu);

  gtk_table_attach (GTK_TABLE (table), hbox2, 1, 2, 0, 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);

  gtk_signal_connect (GTK_OBJECT (menu), "selection-done",
                      GTK_SIGNAL_FUNC (on_matchbox_entry_changed),
                      NULL);


  
  label = gtk_label_new(_("Gtk Theme:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);

  hbox2 = gtk_hbutton_box_new();
  self.GtkMenu = gtk_option_menu_new();
  gtk_container_add(GTK_CONTAINER(hbox2),self.GtkMenu);

  menu =  make_menu_from_dir(gtkpath, &gtk_entry_test, get_cur_gtktheme());

  gtk_option_menu_set_menu (GTK_OPTION_MENU (self.GtkMenu),menu);

  gtk_table_attach (GTK_TABLE (table), hbox2, 1, 2, 1, 2,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);

  gtk_signal_connect (GTK_OBJECT (menu), "selection-done",
                      GTK_SIGNAL_FUNC (on_gtk_entry_changed),
                      NULL);


  label = gtk_label_new(_("WallPaper:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);
  hbox = gtk_hbox_new(FALSE,0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);
/*
  self.WallPaper = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(self.WallPaper),"/home/pierre/toto.png");
  gtk_container_add(GTK_CONTAINER(hbox),self.WallPaper);
  hbox2 = gtk_hbutton_box_new();
  button = gtk_button_new_with_label("...");
  gtk_container_add(GTK_CONTAINER(hbox2),button);
  gtk_container_add(GTK_CONTAINER(hbox),hbox2);
*/
  return frame;
}
