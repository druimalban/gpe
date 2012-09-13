/* 
 * gpe-mini-browser2 v0.0.1
 * - Basic web browser for GPE and small screen GTK devices based on WebKitGtk -
 * - This file contains the main UI and functions 
 *
 * Dedicated to my dear Nóra.
 *
 * Author: Philippe De Swert <philippe.deswert@scarlet.be>
 *
 * Copyright (C) 2008 Philippe De Swert
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <getopt.h>
#include <string.h>

#include <gtk/gtk.h>

#include <webkit/webkit.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/gpedialog.h>


#include "browser-data.h"
#include "ui.h"
#include "ui-callbacks.h"
#include "utility-functions.h"

/* definitions and function declarations */

bool kiosk_mode = FALSE; /*global boolean for kiosk mode */
bool smallscreen = FALSE;
bool active_pbar = FALSE;

struct gpe_icon my_icons[] = {
  {"gpe-mini-browser-icon", PREFIX "/share/pixmaps/gpe-mini-browser2.png"},
  {NULL, NULL}
};

/*----------------------------------------------------------------------------------*/

int main (int argc, char *argv[])
{
  int opt;

  /* application init */
#if !GLIB_CHECK_VERSION(2, 31, 0)
  g_thread_init(NULL);
#endif
  gpe_application_init (&argc, &argv);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  /* parse command line options and input */
  while ((opt = getopt (argc, argv, "bBfFsSvVh")) != -1)
    {
      switch (opt)
        {
	case 'b':
	case 'B':
	  smallscreen = FALSE;
	  break;
	case 'f':
	case 'F':
	  kiosk_mode = TRUE;
	  break;
	case 's':
	case 'S':
	  smallscreen = TRUE;
	  break; 
	case 'v':
	case 'V':
          printf
            (("GPE-mini-browser2 version 0.0.1. (C) 2008, Philippe De Swert\n"));
          exit (0);

	case 'h':
        default:
          printf
            (("GPE-mini-browser2, basic web browser application. (c) 2008, Philippe De Swert\n"));
          printf (("Usage: gpe-mini-browser2 <URL>\n"));
          printf (("Use -v or -V for version info.\n"));
          printf (("Use -s or -S to force small screen version.\n"));
          printf (("Use -b or -B to force big screen version.\n"));
	  printf (("Use -f or -F to start in fullscreen kiosk mode.\n"));
          exit (0);
        }
    }

  /* create main window */
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name(main_window, "gpe-mini-browser2");
  gtk_window_set_default_size (GTK_WINDOW (main_window), 800, 600);
  g_signal_connect (G_OBJECT(main_window), "destroy", G_CALLBACK(browser_quit_cb), NULL);
  gpe_set_window_icon (main_window, "gpe-mini-browser-icon");

  /* pack components before adding them to the main window */
  main_window_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_window_vbox), create_toolbar(), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_window_vbox), create_urlbar(), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_window_vbox), create_tabs(), TRUE, TRUE, 0);

  /* load command line url if there is one, otherwise load google */
  gchar *url = (gchar*) (argc != optind ? argv[optind] : "http://www.google.com/");
  webkit_web_view_open (web_view, parse_url(url));

  /* populate main window and show everything */
  gtk_container_add(GTK_CONTAINER(main_window), main_window_vbox);

  /* start in fullscreen if requested */
  if (kiosk_mode)
    gtk_window_fullscreen(GTK_WINDOW(main_window));

  gtk_widget_show_all(main_window);
  gtk_main();
  
  return 0;
}
