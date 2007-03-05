/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003 - 2006  Florian Boor <florian.boor@kernelconcepts.de>
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
#include "modules/timeanddate.h"
#include "modules/screen/screen.h"
#include "modules/network.h"
#include "modules/theme.h"
#include "modules/sleep/sleep.h"
#include "modules/ownerinfo.h"
#include "modules/login-setup.h"
#include "modules/users/users.h"
#include "modules/gpe-admin.h"
#include "modules/serial.h"
#include "modules/usb.h"
#include "modules/cardinfo.h"
#include "modules/tasks.h"
#include "modules/keys/keys.h"
#include "modules/sleep/conf.h"
#include "modules/sound/sound.h"

#include <gdk/gdkx.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/spacing.h>
#include <gpe/infoprint.h>

#define N_(_x) (_x)

/* 
 * some prototypes
 */
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);

int suidPID;
char* PCMCIA_ERROR = NULL;

static struct {
  GtkWidget *w;

  GtkWidget *applet;
  GtkWidget *vbox;
  GtkWidget *viewport;

  GtkToolItem *save;
  GtkToolItem *cancel;
  GtkToolItem *dismiss;
  GtkWidget *toolbar;
	
  int cur_applet;
  int alone_applet;

}self;

GtkWidget *mainw; /* for some dialogs */

struct Applet applets[]=
  {
    { &Time_Build_Objects, &Time_Free_Objects, &Time_Save, &Time_Restore , 
		N_("Time") ,"time" ,N_("Time and Date Setup"),PREFIX "/share/pixmaps/gpe-config-time.png"},
    { &screen_Build_Objects, &screen_Free_Objects, &screen_Save, &screen_Restore,
		N_("Screen") , "screen", N_("Screen Setup"), PREFIX "/share/pixmaps/gpe-config-screen.png"},
    { &Keys_Build_Objects, NULL, &Keys_Save, &Keys_Restore ,
		N_("Keys & Buttons") ,"keys", N_("Keys and Buttons Setup"),PREFIX "/share/pixmaps/gpe-config-kbd.png"},
    { &Network_Build_Objects, &Network_Free_Objects, &Network_Save, &Network_Restore ,
		N_("Network") ,"network",N_("Network Setup"),PREFIX "/share/pixmaps/gpe-config-network.png"},
    { &Theme_Build_Objects, NULL, &Theme_Save, &Theme_Restore ,
		N_("Theme") ,"theme", N_("Look and Feel"),PREFIX "/share/pixmaps/gpe-config-theme.png"},
    { &Sleep_Build_Objects, NULL, &Sleep_Save, &Sleep_Restore ,
		N_("Power") ,"sleep",N_("Configure Power Saving"),PREFIX "/share/pixmaps/gpe-config-sleep.png"},
    { &Ownerinfo_Build_Objects, &Ownerinfo_Free_Objects, &Ownerinfo_Save, &Ownerinfo_Restore,
		N_("Owner"), "ownerinfo", N_("Owner Information"),PREFIX "/share/pixmaps/gpe-config-ownerinfo.png"},
    { &Login_Setup_Build_Objects, &Login_Setup_Free_Objects, &Login_Setup_Save, &Login_Setup_Restore,
		N_("Login"), "login-setup", N_("Login Setup"),PREFIX "/share/pixmaps/gpe-config-login-setup.png"},
    { &Users_Build_Objects, &Users_Free_Objects, &Users_Save, &Users_Restore ,
		N_("Users"),"users" ,N_("User Administration"),PREFIX "/share/pixmaps/gpe-config-users.png"},
    { &GpeAdmin_Build_Objects, &GpeAdmin_Free_Objects, &GpeAdmin_Save, &GpeAdmin_Restore , 
		N_("GPE") ,"admin",N_("GPE Conf Administration"),PREFIX "/share/pixmaps/gpe-config-admin.png"},
    { &Serial_Build_Objects, &Serial_Free_Objects, &Serial_Save, &Serial_Restore ,
		N_("Serial Ports") ,"serial", N_("Serial Port Configuration"),PREFIX "/share/pixmaps/gpe-config-serial.png"},
    { &USB_Build_Objects, NULL, &USB_Save, &USB_Restore ,
		N_("USB") ,"usb", N_("USB Configuration"),PREFIX "/share/pixmaps/gpe-config-serial.png"},
    { &Cardinfo_Build_Objects, &Cardinfo_Free_Objects, NULL, &Cardinfo_Restore ,
		N_("PCMCIA/CF Cards") ,"cardinfo", N_("PCMCIA/CF Card Info and Config"),PREFIX "/share/pixmaps/gpe-config-cardinfo.png"},
    { &Sound_Build_Objects, &Sound_Free_Objects, &Sound_Save, &Sound_Restore , 
		N_("Sound") ,"sound", N_("Sound Setup"), PREFIX "/share/pixmaps/gpe-config-sound.png"},
    { NULL, NULL, NULL, NULL ,
		N_("Task nameserver") ,"task_nameserver", N_("Task for changing nameserver"), PREFIX "/share/pixmaps/gpe-config-admin.png"},
    { NULL, NULL, NULL, NULL ,
		N_("Task sound") ,"task_sound", N_("Command line task saving/restoring sound settings."), PREFIX "/share/pixmaps/gpe-config-admin.png"},
    { NULL, NULL, NULL, NULL ,
		N_("Task background image") ,"task_background", N_("Only select background image."), PREFIX "/share/pixmaps/gpe-config-admin.png"},
    { NULL, NULL, NULL, NULL ,
		N_("Task backlight setting") ,"task_backlight", N_("Change backlight power and brightness."), PREFIX "/share/pixmaps/gpe-config-admin.png"}
  };

struct gpe_icon my_icons[] = {
  { "lock" },
  { "lock16" },
  { "ownerphoto", "tux-48" },
  { "warning16", "warning16" },
  { "icon", NULL },
  { NULL, NULL }
};


#define count_icons 5

int applets_nb = sizeof(applets) / sizeof(struct Applet);


void Save_Callback(GtkWidget *sender)
{
  if (sender == GTK_WIDGET(self.save))
	  gpe_popup_infoprint(GDK_DISPLAY(), _("Settings saved"));
  while (gtk_events_pending())
	  gtk_main_iteration_do(FALSE);
  if (applets[self.cur_applet].Save)
    applets[self.cur_applet].Save();
  gtk_widget_hide(self.w);
  sleep(1);
  if(self.alone_applet)
    {
      gtk_main_quit();
      exit(0);
    }
}


void Restore_Callback()
{
  while (gtk_events_pending())
	  gtk_main_iteration_do(FALSE);
  
  if (applets[self.cur_applet].Restore)
    applets[self.cur_applet].Restore();
  gtk_widget_hide(self.w);
  sleep(1);
  if (self.alone_applet)
  {
    gtk_main_quit();
  }
}


void item_select(int useronly, gpointer user_data)
{
  int i = (int) user_data;

  if (self.cur_applet != - 1)
    {
      if (applets[self.cur_applet].Restore)
        applets[self.cur_applet].Restore();
      if (applets[self.cur_applet].Free_Objects)
        applets[self.cur_applet].Free_Objects();      
    }
  if(self.applet)
    {
      gtk_widget_hide(self.applet);
      gtk_container_remove(GTK_CONTAINER(self.viewport), self.applet);
    }
  self.cur_applet = i;

  if (applets[i].Build_Objects)
    self.applet = applets[i].Build_Objects(useronly, self.toolbar);
  else 
	self.applet = NULL;
 
  if (self.applet)
  {
    gtk_container_add(GTK_CONTAINER(self.viewport), self.applet);
	
    gtk_window_set_title(GTK_WINDOW(self.w), applets[i].frame_label);
	
    gtk_widget_show_all(self.applet);
  
    if (applets[self.cur_applet].Save)
      {
        gtk_widget_show(GTK_WIDGET(self.save));
        gtk_widget_grab_default(GTK_WIDGET(self.save));
      }
    else
      {
        gtk_widget_hide(GTK_WIDGET(self.save));
      }  
  
    if (applets[self.cur_applet].Restore)
      {
        if(!applets[self.cur_applet].Save)
  	      {
            gtk_widget_hide(GTK_WIDGET(self.cancel));
            gtk_widget_show(GTK_WIDGET(self.dismiss));
            gtk_widget_grab_default(GTK_WIDGET(self.dismiss));
          }
        else
          {
            gtk_widget_hide(GTK_WIDGET(self.dismiss));
            gtk_widget_show(GTK_WIDGET(self.cancel));
          }
      }
    else
      gtk_widget_hide(GTK_WIDGET(self.cancel));
  }
}


int killchild()
{
  kill(suidPID,SIGTERM);
  return 0; 
}

void initwindow()
{
  gint size_x, size_y;

   /* screen layout detection */
   size_x = gdk_screen_width();
   size_y = gdk_screen_height();  

	
   /* main window */	
   self.w = mainw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
   if ((size_x > 480) && (size_y >= 480))
   {
      gtk_window_set_type_hint(GTK_WINDOW(self.w), GDK_WINDOW_TYPE_HINT_DIALOG);
	  gtk_window_set_default_size(GTK_WINDOW(self.w), 460, 480);
   }
   else
     gtk_window_set_default_size(GTK_WINDOW(self.w), 240, 310);
	   
	   
   g_signal_connect (G_OBJECT(self.w), "delete-event",
	                 G_CALLBACK(gtk_main_quit), NULL);

   g_signal_connect (G_OBJECT(self.w), "delete-event",
	                 G_CALLBACK(killchild), NULL);

   g_signal_connect (G_OBJECT(self.w), "destroy", 
	                 G_CALLBACK(gtk_main_quit), NULL);
}


void make_container()
{
  GtkToolItem *item;
	
  GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  self.toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(self.toolbar), 
	                          GTK_ORIENTATION_HORIZONTAL);	
  gtk_box_pack_start(GTK_BOX(self.vbox), self.toolbar, FALSE, TRUE, 0);

  self.cancel = gtk_tool_button_new_from_stock(GTK_STOCK_CANCEL);
  self.save = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
  self.dismiss = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);

  gtk_toolbar_insert(GTK_TOOLBAR(self.toolbar), self.cancel, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(self.toolbar), self.save, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(self.toolbar), self.dismiss, -1);

  item = gtk_separator_tool_item_new();
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), TRUE);
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(self.toolbar), item, 0);

  self.viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), self.viewport);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (self.viewport), GTK_SHADOW_NONE);

  gtk_container_add (GTK_CONTAINER (self.vbox), scrolledwindow);

  GTK_WIDGET_SET_FLAGS(self.save, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(self.dismiss, GTK_CAN_DEFAULT);
  g_signal_connect (G_OBJECT(self.save), "clicked",
		      G_CALLBACK(Save_Callback), NULL);
  g_signal_connect (G_OBJECT(self.cancel), "clicked",
		      G_CALLBACK(Restore_Callback), NULL);
  g_signal_connect (G_OBJECT(self.dismiss), "clicked",
		      G_CALLBACK(Save_Callback), NULL);
}


void 
main_one (int argc, char **argv,int applet)
{
  int handled = FALSE;
  gboolean special_flag = FALSE; /* Don't change to suid mode or similar. */  
  gboolean standalone = FALSE; /* applet creates its own window */
	
  self.alone_applet = 1;
  self.applet = NULL;

  my_icons[count_icons - 1].filename = applets[applet].icon_file;

  if (!strcmp(argv[1],"task_backlight"))
  {
	  handled = TRUE;
	  if (argc == 2)
		  task_backlight(NULL, NULL);
	  else if (argc == 3)
		  task_backlight(argv[2], NULL);
	  else	if (argc == 4)
		task_backlight(argv[2], argv[3]);
	  else
		  fprintf(stderr,_("'task_backlight' supports zero, one and two arguments.\n"));
	  exit(0);
  }
	
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  /* check if we are called to do a command line task */
  if (argc > 2)
  {
	  if (!strcmp(argv[1], "task_nameserver"))
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
		  special_flag = TRUE;
	  }
	  if (!strcmp(argv[2],"password"))
	  {
		  special_flag = TRUE;
		  standalone = TRUE;
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
	  if (!strcmp(argv[1], "task_background"))
	  {
		  special_flag = TRUE;
		  standalone = TRUE;
		  task_change_background_image();
		  exit(0);
	  }
  }
  
  /* If no task? - start applet */
  if (!handled)
  { 
	  self.cur_applet = -1;
	  self.applet = NULL;
	  
	  if (!standalone)
	  {
		  initwindow();
		
		  self.vbox = gtk_vbox_new(FALSE,0);
		  gtk_container_add(GTK_CONTAINER(self.w), self.vbox);
				
		  make_container();
		
		  gpe_set_window_icon(self.w, "icon");
		  gtk_widget_show_all(self.w);
		 
		  gtk_widget_show(self.w);
	  }
	   
	  item_select(special_flag, (gpointer)applet);
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

      setresuid(getuid(), getuid(), getuid()); // abandon privilege..
      setresgid(getgid(), getgid(), getgid()); // abandon privilege..
	
	signal(SIGINT, Restore_Callback);
	signal(SIGTERM, Restore_Callback);
	
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
