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
#include "users.h"

#include "gpe/init.h"

#include <gpe/picturebutton.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>


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
  //  GtkWidget *frame;

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
    { &Kbd_Build_Objects, &Unimplemented_Free_Objects, &Kbd_Save, &Unimplemented_Restore , "vKeyboard" ,"keyboard", "Virtual Keyboard Setup"},
    { &Keyctl_Build_Objects, &Unimplemented_Free_Objects, &Keyctl_Save, &Unimplemented_Restore , "Buttons" ,"keyctl", "Button Configuration"},
    { &Network_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Network" ,"network","IP Addresses"},
    { &Theme_Build_Objects, &Unimplemented_Free_Objects, &Theme_Save, &Unimplemented_Restore , "Theme" ,"theme", "Global Appearance Setup"},
    { &Sleep_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Sleep" ,"sleep","Sleep Configuration"},
    { &Ownerinfo_Build_Objects, &Ownerinfo_Free_Objects, &Ownerinfo_Save, &Ownerinfo_Restore , "Owner" ,"ownerinfo","Owner Information"},
    { &Users_Build_Objects, &Users_Free_Objects, &Users_Save, &Users_Restore , "Users" ,"users","User Administration"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Sound" ,"sound","Sound Setup"},
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
   self.w= mainw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   wstyle = self.w->style;
   gtk_window_set_title(GTK_WINDOW(self.w),"GPE-Conf " VERSION);
   gtk_widget_set_usize(GTK_WIDGET(self.w),300, 400);


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

  self.cancel = gpe_picture_button (self.w->style, ("Cancel"), "cancel");
  gtk_box_pack_start(GTK_BOX(hbuttons),self.cancel,TRUE, TRUE, 0);

  self.save = gpe_picture_button (self.w->style, ("Apply"), "save");
  gtk_box_pack_start(GTK_BOX(hbuttons),self.save,TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT(self.save), "clicked",
		      (GtkSignalFunc) Save_Callback, NULL);
  gtk_signal_connect (GTK_OBJECT(self.cancel), "clicked",
		      (GtkSignalFunc) Restore_Callback, NULL);

}
void main_all()
{
  int i;
  GtkWidget *ntree;
  GtkWidget *root_tree;
  GtkWidget *sys_root;
  GtkWidget *split;

  self.alone_applet=0;

  initwindow();
  split=gtk_hpaned_new();

  gtk_container_add (GTK_CONTAINER (self.w), split);

  gtk_widget_show(split);

  root_tree = gtk_tree_new();
  sys_root = gtk_tree_item_new_with_label("System");
  gtk_widget_show(sys_root);
  gtk_tree_append(GTK_TREE(root_tree),sys_root);

   

  gtk_paned_add1(GTK_PANED(split),root_tree);


  self.vbox = gtk_vbox_new(FALSE,0);
  gtk_paned_add2(GTK_PANED(split),self.vbox);

  
  make_container();

  self.applet = gtk_label_new("GPE Configuration\nby Pierre Tardy\n\nInspired by sysset\nby James Weatherall.");
  gtk_container_add(GTK_CONTAINER(self.viewport),self.applet);

  ntree =  gtk_tree_new();
  gtk_widget_show(ntree);

  for(i = 0; i< applets_nb ; i++)
    {
      GtkWidget *item;
      item = gtk_tree_item_new_with_label(applets[i].label);
      gtk_signal_connect (GTK_OBJECT(item), "select",
			  (GtkSignalFunc) item_select, (gpointer)i);
       
      gtk_widget_show(item);
      gtk_tree_append(GTK_TREE(ntree),item);
    }

  gtk_tree_item_set_subtree(GTK_TREE_ITEM(sys_root),ntree);
  gtk_tree_item_expand(GTK_TREE_ITEM(sys_root));

  self.cur_applet = -1;
  gtk_widget_show_all(self.w);
 
  gtk_widget_show(self.w);

  gtk_widget_hide(self.save);
  gtk_widget_hide(self.cancel);
  
  gtk_main();
  gtk_exit(0);
  return;
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
	  main_all(argc,argv);
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
