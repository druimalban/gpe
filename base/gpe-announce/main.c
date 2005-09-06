/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <gtk/gtk.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>

#include "announce.h"

#define _(_x) gettext (_x)

char *filename = NULL;
int alarm_volume = 999;
struct gpe_icon my_icons[] = {
  { "bell" },
  { "clock-popup" },
  { NULL, }
};

int times=0;

	
void* gst_handle=NULL;

int
main (int argc, char *argv[])
{
  char *announcetext;
  GtkWidget *window;
  g_thread_init(NULL);

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  
  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if (argc >= 2){
  	announcetext = argv[1];
	if(argc >= 3){
		filename = argv[2];
		gst_handle = dlopen("libgstreamer-0.8.so.1",RTLD_LAZY);
		if(gst_handle)
			printf("Gstreamer library loaded\n");
		else
			printf("Error: %s\n",dlerror());
	}
	if(argc == 4){
		alarm_volume = atoi(argv[3]);
	}
  }else{
	announcetext = NULL;
  }
  
  
  window = create_window (announcetext);

  gtk_widget_show_all (window);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (gtk_exit), NULL);

  gtk_main ();

  return 0;
}
