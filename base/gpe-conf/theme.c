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
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE /* Pour GlibC2 */
#endif
#include <time.h>
#include "applets.h"
#include "theme.h"

#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/spacing.h>


static struct
{
  GtkWidget *MatchboxMenu;
  GtkWidget *GtkMenu;
  GtkWidget *WallPaper;
}self;

static char * matchboxpath = "/usr/share/matchbox/themes";
static char * gtkpath = "/usr/share/themes";


/*******************/
/*   init stuff    */
/*******************/

char * get_cur_matchboxtheme()
{
  FILE *pipe;
  static char cur[256];
  return 0;
  pipe = popen ("mbcontrol -r", "r");

  if (pipe > 0)
    {
      fgets (cur, 255, pipe);
      if(feof(pipe) || strlen(cur)==0) //test if mcontrol is here
	return 0;
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

void get_wallpaper_filename(char *cur)
{
  char *home = getenv("HOME");
	
  if(strlen(home) > 240)
      gpe_error_box( "bad $HOME !!");

  sprintf(cur,"%s/.gpe/wallpaper",home);

}

char * get_cur_gtktheme()
{
  FILE *f;
  static char cur[256];
  int found = 0;
  return 0;
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

char*
get_wallpaper              ()
{
  static char buf[255];
  FILE *f;

  get_wallpaper_filename(buf);
  f = fopen(buf,"r");
  buf[0]=0;
  if(f>0)
    {
      fgets(buf,255,f);
      fclose(f);
    }
  return buf;
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


/*******************/
/*  Changing stuff */
/*******************/

void
on_matchbox_entry_changed              (GtkWidget     *menu,
                                        gpointer         user_data)
{
#if ! __i386__
	
  system_printf("mbcontrol -t %s",(char *)gtk_object_get_data(GTK_OBJECT(
								 gtk_menu_get_active(GTK_MENU(menu))
								 ),"fullpath"));
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

static void File_Selected(char *file, gpointer data)
{
  gtk_entry_set_text(GTK_ENTRY(self.WallPaper),file);	
}

void
choose_wallpaper              (GtkWidget     *button,
			       gpointer         user_data)
{
  ask_user_a_file(getenv("HOME"),NULL,File_Selected,NULL,NULL);
}

void Theme_Save()
{
  char *file;
  char configfile[255];
  FILE * f;
  file = gtk_entry_get_text(GTK_ENTRY(self.WallPaper));	
  get_wallpaper_filename(configfile);
  if(file[0])
    {
      f = fopen (configfile,"w");
      printf("%s<-%s\n",configfile,file);
      if(f>0)
	{
	  fprintf(f,file);
	  fclose(f);
	}
    if(system_printf("gpe-setbg %s",file))
	gpe_error_box("You need gpe-setbg installed\nto set the wallpaper!!");
    }
}


/****************/
/*  interface   */
/****************/

GtkWidget *Theme_Build_Objects()
{
  GtkWidget *table;
  GtkWidget *scrolledwindow;
  GtkWidget *viewport;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *menu ;
  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  GtkJustification table_justify_right_col;
  guint gpe_border     = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();

  /* 
   * GTK_EXPAND  the widget should expand to take up any extra space
                 in its container that has been allocated.
   * GTK_SHRINK  the widget should shrink as and when possible.
   * GTK_FILL    the widget should fill the space allocated to it.
   */
  
  /*
   * GTK_SHRINK to make it as small as possible, but use GTK_FILL to
   * let it fill on the left side (so that the right alignment
   * works:
   */ 
  table_attach_left_col_x = GTK_FILL; 
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;
  
  /*
   * GTK_JUSTIFY_LEFT
   * GTK_JUSTIFY_RIGHT
   * GTK_JUSTIFY_CENTER (the default)
   * GTK_JUSTIFY_FILL
   */
  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  /* ======================================================================== */
  /* draw the GUI */
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

  table = gtk_table_new(3,2,FALSE);
  gtk_widget_set_name (table, "table");
  gtk_container_add (GTK_CONTAINER (viewport), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);

  /* ------------------------------------------------------------------------ */
  label = gtk_label_new(_("Matchbox Theme:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label),
			gpe_boxspacing, gpe_boxspacing);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  /* make the label grey: */
  gtk_rc_parse_string ("widget '*label' style 'gpe_labels'");
  gtk_widget_set_name (label, "label");

  self.MatchboxMenu = gtk_option_menu_new();

  menu =  make_menu_from_dir (matchboxpath, NULL, get_cur_matchboxtheme() );
  gtk_option_menu_set_menu (GTK_OPTION_MENU (self.MatchboxMenu),menu);

  gtk_table_attach (GTK_TABLE (table), self.MatchboxMenu, 1, 2, 0, 1,
		    (GtkAttachOptions) (table_attach_right_col_x),
		    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  gtk_signal_connect (GTK_OBJECT (menu), "selection-done",
                      GTK_SIGNAL_FUNC (on_matchbox_entry_changed),
                      NULL);

  label = gtk_label_new(_("GTK+ Theme:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label),
			gpe_boxspacing, gpe_boxspacing);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  /* make the label grey: */
  gtk_rc_parse_string ("widget '*label' style 'gpe_labels'");
  gtk_widget_set_name (label, "label");

  self.GtkMenu = gtk_option_menu_new();
  menu =  make_menu_from_dir(gtkpath, &gtk_entry_test, get_cur_gtktheme());

  gtk_option_menu_set_menu (GTK_OPTION_MENU (self.GtkMenu),menu);

  gtk_table_attach (GTK_TABLE (table), self.GtkMenu, 1, 2, 1, 2,
		    (GtkAttachOptions) (table_attach_right_col_x),
		    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  gtk_signal_connect (GTK_OBJECT (menu), "selection-done",
                      GTK_SIGNAL_FUNC (on_gtk_entry_changed),
                      NULL);

  label = gtk_label_new(_("Wallpaper:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    (GtkAttachOptions) (table_attach_left_col_x),
		    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label),
			gpe_boxspacing, gpe_boxspacing);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  /* give the label a style: */
  gtk_rc_parse_string ("widget '*label' style 'gpe_labels'");
  gtk_widget_set_name (label, "label");

  hbox = gtk_hbox_new(FALSE,0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 3,
		    (GtkAttachOptions) (table_attach_right_col_x),
		    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);
  self.WallPaper  = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(self.WallPaper),get_wallpaper());
  gtk_box_pack_start (GTK_BOX (hbox), self.WallPaper, TRUE, TRUE, 0);
  
  button = gtk_button_new_with_label("...");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  /* FIXME: do not hardcode the usize here, but use a global GPE constant [CM] */
  gtk_widget_set_usize (button, 25, -2);


  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (choose_wallpaper),
                      NULL);

  return scrolledwindow;
}
