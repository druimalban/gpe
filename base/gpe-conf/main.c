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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include "suid.h"
#include "applets.h"
#include "timeanddate.h"
#include "appmgr_setup.h"
#include "ipaqscreen.h"
#include "unimplemented.h"
#include "kbd.h"
#include "network.h"
#include "theme.h"
#include "keyctl.h"
#include "sleep.h"
#include "ownerinfo.h"
#include "login-setup.h"
#include "users.h"
#include "gpe-admin.h"
#include "sleep/conf.h"

#include "gpe/init.h"

#include <gpe/picturebutton.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/spacing.h>


/* These aren't in the header files, so we prototype them here.
 */
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);

int suidPID;

GtkStyle *wstyle;
static struct {
  GtkWidget *w;

  GtkWidget *applet;
  GtkWidget *vbox;
  GtkWidget *viewport;

  GtkWidget *save;
  GtkWidget *cancel;
  
  int cur_applet;
  int alone_applet;

}self;

GtkWidget *mainw; // for dialogs

struct Applet applets[]=
  {
    { &Time_Build_Objects, &Time_Free_Objects, &Time_Save, &Time_Restore , "Time" ,"time" ,"Time and Date Setup"},
    { &Appmgr_Build_Objects, &Appmgr_Free_Objects, &Appmgr_Save, &Appmgr_Restore , "Appmgr" ,"appmgr", "Launcher Setup"},
    { &ipaqscreen_Build_Objects, &ipaqscreen_Free_Objects, &ipaqscreen_Save, &ipaqscreen_Restore , "Screen" , "ipaqscreen", "Screen Setup"},
    { &Kbd_Build_Objects, &Unimplemented_Free_Objects, &Kbd_Save, &Kbd_Restore , "vKeyboard" ,"keyboard", "Virtual Keyboard Setup"},
    { &Keyctl_Build_Objects, &Unimplemented_Free_Objects, &Keyctl_Save, &Keyctl_Restore , "Buttons" ,"keyctl", "Button Configuration"},
    { &Network_Build_Objects, &Network_Free_Objects, &Network_Save, &Network_Restore , "Network" ,"network","IP Addresses"},
    { &Theme_Build_Objects, &Unimplemented_Free_Objects, &Theme_Save, &Theme_Restore , "Theme" ,"theme", "Global Appearance Setup"},
    { &Sleep_Build_Objects, &Unimplemented_Free_Objects, &Sleep_Save, &Sleep_Restore , "Sleep" ,"sleep","Sleep Configuration"},
    { &Ownerinfo_Build_Objects, &Ownerinfo_Free_Objects, &Ownerinfo_Save, &Ownerinfo_Restore, "Owner", "ownerinfo", "Owner Information"},
    { &Login_Setup_Build_Objects, &Login_Setup_Free_Objects, &Login_Setup_Save, &Login_Setup_Restore, "Login", "login-setup", "Login Setup"},
    { &Users_Build_Objects, &Users_Free_Objects, &Users_Save, &Users_Restore , "Users" ,"users","User Administration"},
    { &GpeAdmin_Build_Objects, &GpeAdmin_Free_Objects, &GpeAdmin_Save, &GpeAdmin_Restore , "GPE" ,"admin","GPE Conf Administration"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Mouse" ,"mouse","Mouse Configuration"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Energy" ,"apm", "Advanced Power Management Setup"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Screensvr" ,"screensaver","Screen Saver Configuration"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Software" ,"software","Adding and Removing Programs"},
  };
struct gpe_icon my_icons[] = {
  { "save" },
  { "cancel" },
  { "delete" },
  { "properties" },
  { "new" },
  { "lock" },
  { "media-play" },
  { "media-stop" },
  { "ipaq" },
  { "ownerphoto", "tux-48" },
  { "warning16", "warning16" },
  { "default-bg", PREFIX "/share/pixmaps/gpe-default-bg.png" }, 
  { NULL, NULL }
};

int applets_nb = sizeof(applets) / sizeof(struct Applet);

void Save_Callback()
{
  applets[self.cur_applet].Save();
  if(self.alone_applet)
    gtk_exit(0);

}
void Restore_Callback()
{
  applets[self.cur_applet].Restore();
  if(self.alone_applet)
    gtk_exit(0);
}

void item_select(GtkWidget *ignored, gpointer user_data)
{
  int i = (int) user_data;

  if(self.cur_applet != - 1)
    {
      applets[self.cur_applet].Restore(); // for the time and date
      applets[self.cur_applet].Free_Objects();      
    }
  if(self.applet)
    {
      gtk_widget_hide(self.applet);
      // TODO there must be a memory leak here..
      gtk_container_remove(GTK_CONTAINER(self.viewport),self.applet);
    }
  self.cur_applet = i;

  self.applet = applets[i].Build_Objects();
  gtk_container_add(GTK_CONTAINER(self.viewport),self.applet);
  //gtk_frame_set_label(GTK_FRAME(self.frame),applets[i].frame_label);
  gtk_widget_show_all(self.applet);

  gtk_window_set_title(GTK_WINDOW(self.w), applets[i].frame_label);
  
  if(applets[self.cur_applet].Save != &Unimplemented_Save)
    gtk_widget_show(self.save);
  else
    gtk_widget_hide(self.save);
    
  if(applets[self.cur_applet].Restore != &Unimplemented_Restore)
    gtk_widget_show(self.cancel);
  else
    gtk_widget_hide(self.cancel);
}

int killchild()
{
  kill(suidPID,SIGTERM);
 return 0; 
}

void initwindow()
{
  if (gpe_application_init (NULL, NULL) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

   // main window
   self.w = mainw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   wstyle = self.w->style;
   gtk_window_set_title(GTK_WINDOW(self.w),"GPE-Config " VERSION);
   gtk_widget_set_usize(GTK_WIDGET(self.w),240, 310);


   gtk_signal_connect (GTK_OBJECT(self.w), "delete-event",
		       (GtkSignalFunc) gtk_main_quit, NULL);

   gtk_signal_connect (GTK_OBJECT(self.w), "delete-event",
		       (GtkSignalFunc) killchild, NULL);

   gtk_signal_connect (GTK_OBJECT(self.w), "destroy", 
      (GtkSignalFunc) gtk_main_quit, NULL);


}

void make_container()
{
  GtkWidget *hbuttons;

  GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  self.viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow),self.viewport);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (self.viewport), GTK_SHADOW_NONE);

  gtk_container_add (GTK_CONTAINER (self.vbox), scrolledwindow);

  hbuttons = gtk_hbutton_box_new();
  gtk_box_pack_end(GTK_BOX(self.vbox),hbuttons, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbuttons), 2);

  self.cancel = gpe_picture_button (self.w->style, _("Cancel"), _("cancel"));
  gtk_box_pack_start(GTK_BOX(hbuttons),self.cancel,TRUE, TRUE, 0);

  self.save = gpe_picture_button (self.w->style, _("Apply"), _("save"));
  gtk_box_pack_start(GTK_BOX(hbuttons),self.save,TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT(self.save), "clicked",
		      (GtkSignalFunc) Save_Callback, NULL);
  gtk_signal_connect (GTK_OBJECT(self.cancel), "clicked",
		      (GtkSignalFunc) Restore_Callback, NULL);

}


void main_one(int argc, char **argv,int applet)
{

  self.alone_applet=1;
  initwindow();

  self.vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(self.w),self.vbox);

  self.cur_applet = -1;
  self.applet = NULL;

  make_container();


  gtk_widget_show_all(self.w);
 
  gtk_widget_show(self.w);

   
  item_select(NULL, (gpointer)applet);
  gtk_main();
  gtk_exit(0);
  return ;
  
}



int main(int argc, char **argv)
{
  int i;
  int pipe1[2];
  int pipe2[2];

  if(pipe(pipe1))
    {
      fprintf(stderr, "cant pipe\n");
      exit(errno);
    }
  if(pipe(pipe2))
    {
      fprintf(stderr, "cant pipe\n");
      exit(errno);
    }

  switch(suidPID = fork())
    {
    case -1:
      fprintf(stderr, "cant fork\n");
      exit(errno);
    case 0:
      close(pipe1[0]);
      close(pipe2[1]);
      suidloop(pipe1[1],pipe2[0]);
      exit(0);
    default:
      close(pipe2[0]);
      close(pipe1[1]);
      suidout = fdopen(pipe2[1],"w");
      suidin = fdopen(pipe1[0],"r");

	
      setresuid(getuid(),getuid(),getuid()); // abandon privilege..
      setresgid(getgid(),getgid(),getgid()); // abandon privilege..
      if(argc == 1)
	{
		fprintf(stderr,"This mode is disabled, please try:\n");
	      printf("\ngpe-conf [AppletName]\nwhere AppletName is in:\n");
	      for( i = 0 ; i< applets_nb ; i++)
		if(applets[i].Build_Objects != Unimplemented_Build_Objects)
		  printf("%s\t\t:%s\n",applets[i].name,applets[i].frame_label);
	}
      else
	{
	  for( i = 0 ; i< applets_nb ; i++)
	    {
	      if(strcmp(argv[1], applets[i].name) == 0)
		main_one(argc,argv,i);
	    }
	  if (i ==applets_nb)
	    {
	      fprintf(stderr,"Applet %s unknown!\n",argv[1]);
	      printf("\n\nUsage: gpe-conf [AppletName]\nwhere AppletName is in:\n");
	      for( i = 0 ; i< applets_nb ; i++)
		if(applets[i].Build_Objects != Unimplemented_Build_Objects)
		  printf("%s\t\t:%s\n",applets[i].name,applets[i].frame_label);
	    }
	}
      fclose(suidout);
      fclose(suidin);
      kill(suidPID,SIGTERM);
    }
      return 0;
}
