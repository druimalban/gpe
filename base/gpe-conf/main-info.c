/*
 * gpe-info
 *
 * Copyright (C) 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 * 
 * main module
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
#include "sysinfo.h"

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/spacing.h>

#define _(x) gettext(x)

GtkWidget *mainw; /* for some dialogs */

struct gpe_icon my_icons[] = {
  { "save" },
  { "cancel" },
  { "exit" },
  { "icon", PREFIX "/share/pixmaps/gpe-config-sysinfo.png" },
  { NULL, NULL }
};

static char *tabs[] = {"global","hardware","battery","storage","network","syslog"};
static int num_tabs = sizeof(tabs) / sizeof(char*);

static struct {
  GtkWidget *w;

  GtkWidget *applet;
  GtkWidget *vbox;
  GtkWidget *viewport;

  GtkWidget *dismiss;
}self;

static void
usage()
{
	int i;
	
	fprintf(stderr,_("usage: gpe-info [page]\n"));
	fprintf(stderr,_("    Where page may be \"global\" (default)\n"));
	for (i=1;i<num_tabs;i++)
		fprintf(stderr,"                      \"%s\"\n",tabs[i]);
	fprintf(stderr,"\n");
}


void initwindow()
{
   self.w = mainw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(self.w),_("System Information"));
   gtk_widget_set_usize(GTK_WIDGET(self.w),240, 310);

   gtk_signal_connect (GTK_OBJECT(self.w), "delete-event",
		       (GtkSignalFunc) gtk_main_quit, NULL);

   gtk_signal_connect (GTK_OBJECT(self.w), "destroy", 
      (GtkSignalFunc) gtk_main_quit, NULL);
}


void make_container(int whichtab)
{
  GtkWidget *hbuttons;

  hbuttons = gtk_hbutton_box_new();
  gtk_box_pack_end(GTK_BOX(self.vbox),hbuttons, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbuttons), gpe_get_boxspacing());

  self.dismiss = gpe_button_new_from_stock(GTK_STOCK_CLOSE,GPE_BUTTON_TYPE_BOTH);
  GTK_WIDGET_SET_FLAGS(self.dismiss, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(hbuttons),self.dismiss,FALSE, TRUE, 0);
  
  gtk_signal_connect (GTK_OBJECT(self.dismiss), "clicked",
		      (GtkSignalFunc) Sysinfo_Free_Objects, NULL);
			  
  self.applet = Sysinfo_Build_Objects(whichtab);
  gtk_container_add(GTK_CONTAINER(self.vbox),self.applet);
	
  gtk_widget_show(self.applet);
  gtk_widget_show_all(hbuttons);
  gtk_widget_grab_default(self.dismiss);
}


void main_one(int argc, char **argv)
{
  int nrtab = 0;
  int i;

  if (argc > 2) 
  {
	  usage();
	  exit(1);
  }
  
  if (argc == 2)
  {
	  nrtab = -1;
	  for (i = 0;i < num_tabs;i++) 
	  	if (!strcmp(argv[1],tabs[i]))
		{
			nrtab = i;
			break;
		}
	  if (nrtab < 0)
	  {
	  	usage();
	  	exit(1);
	  }
  }
  
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);
  
  initwindow();

  self.vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(self.vbox);
  gtk_container_add(GTK_CONTAINER(self.w),self.vbox);
  gpe_set_window_icon(self.w,"icon");
  gtk_widget_show(self.w);

  make_container(nrtab);
  gtk_main();
  gtk_exit(0);
}


int main(int argc, char **argv)
{

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

	signal(SIGINT,Sysinfo_Free_Objects);
	signal(SIGTERM,Sysinfo_Free_Objects);
	
	main_one(argc,argv);
	
   return 0;
}
