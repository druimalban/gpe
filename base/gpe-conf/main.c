/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003, 2004  Florian Boor <florian.boor@kernelconcepts.de>
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
#include <locale.h>
#include "suid.h"
#include "applets.h"
#include "timeanddate.h"
#include "appmgr_setup.h"
#include "screen.h"
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
#include "serial.h"
#ifdef PACKAGETOOL		
#include "packages.h"
#include "keyboard.h"
#endif
#include "cardinfo.h"
#include "tasks.h"
#include "sound/sound.h"

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/spacing.h>


/* These aren't in the header files, so we prototype them here.
 */
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);

int suidPID;
char* PCMCIA_ERROR = NULL;

GtkStyle *wstyle;
static struct {
  GtkWidget *w;

  GtkWidget *applet;
  GtkWidget *vbox;
  GtkWidget *viewport;

  GtkWidget *save;
  GtkWidget *cancel;
  GtkWidget *dismiss;
  
  int cur_applet;
  int alone_applet;

}self;

GtkWidget *mainw; // for dialogs

struct Applet applets[]=
  {
    { &Time_Build_Objects, &Time_Free_Objects, &Time_Save, &Time_Restore , 
		"Time" ,"time" ,"Time and Date Setup",PREFIX "/share/pixmaps/gpe-config-time.png"},
/*    { &Appmgr_Build_Objects, &Appmgr_Free_Objects, &Appmgr_Save, &Appmgr_Restore , 
		"Appmgr" ,"appmgr", "Launcher Setup", PREFIX "/share/pixmaps/gpe-config-appmgr.png"},
*/    { &screen_Build_Objects, &screen_Free_Objects, &screen_Save, &screen_Restore,
		"Screen" , "screen", "Screen Setup", PREFIX "/share/pixmaps/gpe-config-screen.png"},
    { &Kbd_Build_Objects, &Unimplemented_Free_Objects, &Kbd_Save, &Kbd_Restore ,
		"vKeyboard" ,"kbd", "Virtual Keyboard Setup",PREFIX "/share/pixmaps/gpe-config-kbd.png"},
    { &Keyctl_Build_Objects, &Unimplemented_Free_Objects, &Keyctl_Save, &Keyctl_Restore ,
		"Buttons" ,"keyctl", "Button Configuration",PREFIX "/share/pixmaps/gpe-config-keyctl.png"},
    { &Network_Build_Objects, &Network_Free_Objects, &Network_Save, &Network_Restore ,
		"Network" ,"network","Network Setup",PREFIX "/share/pixmaps/gpe-config-network.png"},
    { &Theme_Build_Objects, &Unimplemented_Free_Objects, &Theme_Save, &Theme_Restore ,
		"Theme" ,"theme", "Look and Feel",PREFIX "/share/pixmaps/gpe-config-theme.png"},
    { &Sleep_Build_Objects, &Unimplemented_Free_Objects, &Sleep_Save, &Sleep_Restore ,
		"Sleep" ,"sleep","Sleep Configuration",PREFIX "/share/pixmaps/gpe-config-sleep.png"},
    { &Ownerinfo_Build_Objects, &Ownerinfo_Free_Objects, &Ownerinfo_Save, &Ownerinfo_Restore,
		"Owner", "ownerinfo", "Owner Information",PREFIX "/share/pixmaps/gpe-config-ownerinfo.png"},
    { &Login_Setup_Build_Objects, &Login_Setup_Free_Objects, &Login_Setup_Save, &Login_Setup_Restore,
		"Login", "login-setup", "Login Setup",PREFIX "/share/pixmaps/gpe-config-login-setup.png"},
    { &Users_Build_Objects, &Users_Free_Objects, &Users_Save, &Users_Restore ,
		"Users" ,"users","User Administration",PREFIX "/share/pixmaps/gpe-config-users.png"},
    { &GpeAdmin_Build_Objects, &GpeAdmin_Free_Objects, &GpeAdmin_Save, &GpeAdmin_Restore , 
		"GPE" ,"admin","GPE Conf Administration",PREFIX "/share/pixmaps/gpe-config-admin.png"},
    { &Serial_Build_Objects, &Serial_Free_Objects, &Serial_Save, &Serial_Restore ,
		"Serial" ,"serial","Serial Port Configuration",PREFIX "/share/pixmaps/gpe-config-serial.png"},
    { &Cardinfo_Build_Objects, &Cardinfo_Free_Objects, &Unimplemented_Save, &Cardinfo_Restore ,
		"Cardinfo" ,"cardinfo","PC/CF Card Info and Config",PREFIX "/share/pixmaps/gpe-config-cardinfo.png"},
    { &Sound_Build_Objects, &Sound_Free_Objects, &Sound_Save, &Sound_Restore , 
		"Sound" ,"sound","Sound Setup",PREFIX "/share/pixmaps/gpe-config-sound.png"},
#ifdef PACKAGETOOL		
    { &Packages_Build_Objects, &Packages_Free_Objects, &Unimplemented_Save, &Packages_Restore ,
		"Packages" ,"packages","Add and Remove packages",PREFIX "/share/pixmaps/gpe-config-packages.png"},
    { &Keyboard_Build_Objects, &Unimplemented_Free_Objects, &Keyboard_Save, &Keyboard_Restore ,
		"Keyboard" ,"keyboard", "External Keyboard Setup",PREFIX "/share/pixmaps/gpe-config-keyboard.png"},
#endif		
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore ,
		"Task nameserver" ,"task_nameserver","Task for changing nameserver", PREFIX "/share/pixmaps/gpe-config-admin.png"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore ,
		"Task sound" ,"task_sound","Command line task saving/restoring sound settings.", PREFIX "/share/pixmaps/gpe-config-admin.png"}
  };
  
struct gpe_icon my_icons[] = {
  { "save" },
  { "cancel" },
  { "delete" },
  { "properties" },
  { "new" },
  { "lock" },
  { "lock16" },
  { "exit" },
  { "ownerphoto", "tux-48" },
  { "warning16", "warning16" },
  { "icon", NULL },
  { NULL, NULL }
};


#define count_icons 11

int applets_nb = sizeof(applets) / sizeof(struct Applet);


void Save_Callback()
{
  applets[self.cur_applet].Save();
  if(self.alone_applet)
  {
    gtk_main_quit();
    gtk_exit(0);
  }
}


void Restore_Callback()
{
  applets[self.cur_applet].Restore();
  if(self.alone_applet)
  {
    gtk_main_quit();
    gtk_exit(0);
  }
}


void item_select(int useronly, gpointer user_data)
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
      gtk_container_remove(GTK_CONTAINER(self.viewport),self.applet);
    }
  self.cur_applet = i;

  self.applet = applets[i].Build_Objects(useronly);
  gtk_container_add(GTK_CONTAINER(self.viewport),self.applet);
	
  gtk_window_set_title(GTK_WINDOW(self.w), applets[i].frame_label);
	
  gtk_widget_show_all(self.applet);
  
  if(applets[self.cur_applet].Save != &Unimplemented_Save)
  {
    gtk_widget_show(self.save);
	gtk_widget_grab_default(self.save);
  }
  else
  {
    gtk_widget_hide(self.save);
  }  
  
  if(applets[self.cur_applet].Restore != &Unimplemented_Restore)
  {
	if(applets[self.cur_applet].Save == &Unimplemented_Save)
	{
      gtk_widget_hide(self.cancel);
      gtk_widget_show(self.dismiss);
      gtk_widget_grab_default(self.dismiss);
	}
    else
	{
      gtk_widget_hide(self.dismiss);
      gtk_widget_show(self.cancel);
	}
  }
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
   // main window
   self.w = mainw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   wstyle = self.w->style;
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
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  self.viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow),self.viewport);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (self.viewport), GTK_SHADOW_NONE);

  gtk_container_add (GTK_CONTAINER (self.vbox), scrolledwindow);

  hbuttons = gtk_hbutton_box_new();
  gtk_box_pack_end(GTK_BOX(self.vbox),hbuttons, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbuttons), gpe_get_boxspacing() );

  self.cancel = gpe_button_new_from_stock(GTK_STOCK_CANCEL,GPE_BUTTON_TYPE_BOTH);
  gtk_box_pack_start(GTK_BOX(hbuttons),self.cancel,FALSE, TRUE, 0);

  self.save = gpe_button_new_from_stock(GTK_STOCK_OK,GPE_BUTTON_TYPE_BOTH);
  GTK_WIDGET_SET_FLAGS(self.save, GTK_CAN_DEFAULT);
  
  gtk_box_pack_start(GTK_BOX(hbuttons),self.save,FALSE, TRUE, 0);

  self.dismiss = gpe_button_new_from_stock(GTK_STOCK_CLOSE,GPE_BUTTON_TYPE_BOTH);
  GTK_WIDGET_SET_FLAGS(self.dismiss, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(hbuttons),self.dismiss,FALSE, TRUE, 0);
  
  gtk_signal_connect (GTK_OBJECT(self.save), "clicked",
		      (GtkSignalFunc) Save_Callback, NULL);
  gtk_signal_connect (GTK_OBJECT(self.cancel), "clicked",
		      (GtkSignalFunc) Restore_Callback, NULL);
  gtk_signal_connect (GTK_OBJECT(self.dismiss), "clicked",
		      (GtkSignalFunc) Save_Callback, NULL);
}


void main_one(int argc, char **argv,int applet)
{
  int handled = FALSE;
  gboolean user_only_setup = FALSE; /* Don't change to suid mode. */  
	
  self.alone_applet = 1;

  my_icons[count_icons - 1].filename = applets[applet].icon_file;
	
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  /* check if we are called to do a command line task */
  if (argc > 2)
  {
	  if (!strcmp(argv[1],"task_nameserver"))
	  {
		  handled = TRUE;
		  if (argc == 3)
			  task_change_nameserver(argv[2]);
		  else
			  fprintf(stderr,_("'task_nameserver' needs a new (and only one) nameserver as argument.\n"));
		  exit(0);
	  }
	  if (!strcmp(argv[2],"user_only"))
	  {
		  user_only_setup = TRUE;
	  }
	  if (!strcmp(argv[1],"task_sound"))
	  {
		  handled = TRUE;
		  if (argc == 3)
			  task_sound(argv[2]);
		  else
			  fprintf(stderr,_("'task_sound' needs (s)ave/(r)estore as argument.\n"));
		  exit(0);
	  }
  }
  
  /* If no task? - start applet */
  if (!handled)
  { 
	  initwindow();
	
	  self.vbox = gtk_vbox_new(FALSE,0);
	  gtk_container_add(GTK_CONTAINER(self.w),self.vbox);
	
	  self.cur_applet = -1;
	  self.applet = NULL;
	
	  make_container();
	
	  gpe_set_window_icon(self.w, "icon");
	  gtk_widget_show_all(self.w);
	 
	  gtk_widget_show(self.w);
	
	   
	  item_select(user_only_setup, (gpointer)applet);
	  gtk_main();
	  gtk_exit(0);
  }
  return ;
  
}


int main(int argc, char **argv)
{
  int i;
  int pipe1[2];
  int pipe2[2];

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if(pipe(pipe1))
    {
      fprintf(stderr, "Can't open pipe\n");
      exit(errno);
    }
  if(pipe(pipe2))
    {
      fprintf(stderr, "Can't open pipe\n");
      exit(errno);
    }
	
  /* init pcmcia stuff if necessary */
  if ((argc > 1) && (!strcmp("cardinfo",argv[1])))
  {
	  if (init_pcmcia_suid() != 0)
	  	perror(_("Warning: PCMCIA init failed."));
  }

  /* fork suid root process */
  switch(suidPID = fork())
    {
    case -1:
      fprintf(stderr, "Can't fork\n");
      exit(errno);
    case 0:
      close(pipe1[0]);
      close(pipe2[1]);
      suidloop(pipe1[1],pipe2[0]);
      exit(0);
    default:
      close(pipe2[0]);
      close(pipe1[1]);
	
	  suidoutfd = pipe2[1];
	  suidinfd = pipe1[0];
	
      suidout = fdopen(pipe2[1],"w");
      suidin = fdopen(pipe1[0],"r");

      setresuid(getuid(),getuid(),getuid()); // abandon privilege..
      setresgid(getgid(),getgid(),getgid()); // abandon privilege..
	
	signal(SIGINT,Restore_Callback);
	signal(SIGTERM,Restore_Callback);
	
      if(argc == 1)
	{
		fprintf(stderr,_("This mode is disabled, please try:\n"));
	    fprintf(stderr,_("\ngpe-conf [AppletName]\nwhere AppletName is in:\n"));
	    for ( i = 0 ; i < applets_nb ; i++)
		  fprintf(stderr,"%s\t\t:%s\n",applets[i].name,applets[i].frame_label);
	}
      else
	{
	  for( i = 0 ; i < applets_nb ; i++)
	    {
          if (strcmp(argv[1], applets[i].name) == 0)
		  {
          	main_one(argc, argv, i);
			break;
		  }
	    }
	  if (i == applets_nb)
	    {
	      fprintf(stderr,_("Applet %s unknown!\n"),argv[1]);
	      fprintf(stderr,_("\n\nUsage: gpe-conf [AppletName]\nwhere AppletName is in:\n"));
	      for( i = 0 ; i < applets_nb ; i++)
		    fprintf(stderr,"%s\t\t:%s\n",applets[i].name,applets[i].frame_label);
	    }
	}
      fclose(suidout);
      fclose(suidin);
      kill(suidPID,SIGTERM);
    }
  return 0;
}
