/*
 * gpe-mini-browser v0.17
 *
 * Basic web browser based on gtk-webcore 
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * Dedicated to Apocalyptica (Cult album) for keeping me sane.
 *
 * SUPPORT : If you like this program and want it to be developed further,
 * your wishlist items implemented or just thank me, send me beer from my 
 * native country (Belgium). (Very useful now I (almost) live in Finland)
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

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <getopt.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>

#include "gpe-mini-browser.h"

//#define DEBUG /* uncomment this if you want debug output*/

static int fullscreen = 0;

struct gpe_icon my_icons[] = 
{
  { "gpe-mini-browser-icon", PREFIX "/share/pixmaps/gpe-mini-browser.png" },
  {NULL, NULL}
};

static void
set_fullscreen (GtkWidget * button, GtkWidget * app)
{
  if (!fullscreen)
    {
      gtk_window_fullscreen (GTK_WINDOW (app));
      fullscreen = 1;
    }
  else
    {
      gtk_window_unfullscreen (GTK_WINDOW (app));
      fullscreen = 0;
    }
}


int
main (int argc, char *argv[])
{
  GtkWidget *html, *app, *contentbox;	/* html engine, application window, content box of application window */
  GtkWidget *toolbar, *urlbox = NULL;	/* toolbar, url entry box (big screen) */
  GtkToolItem *back_button, *forward_button, *home_button, *bookmarks_button,
    *fullscreen_button, *url_button = NULL;
  GtkToolItem *separator, *separator2;
  extern GtkToolItem *stop_reload_button;
  const gchar *base;
  gint width = 240, height = 320;
  static struct status_data status;
  int opt;

  WebiSettings s = { 0, };

  gpe_application_init (&argc, &argv);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  while ((opt = getopt (argc, argv, "vh")) != -1)
    {
      switch (opt)
	{
	case 'v':
	  printf
	    (_("GPE-mini-browser version 0.17. (C)2005, Philippe De Swert\n"));
	  exit (0);

	default:
	  printf
	    (_("GPE-mini-browser, basic web browser application. (c)2005, Philippe De Swert\n"));
	  printf (_("Usage: gpe-mini-browser <URL>\n"));
	  printf (_("Use -v for version info.\n"));
	  exit (0);
	}
    }

  if (argv[1] != NULL)
    base = parse_url (argv[1]);
  else
    base = NULL;
#ifdef DEBUG
  fprintf (stderr, "url = %s\n", base);
#endif

  //create application window
  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (app), "delete-event", gtk_main_quit, NULL);
  width = gdk_screen_width ();
  height = gdk_screen_height ();
  gtk_window_set_title (GTK_WINDOW (app), "mini-browser");
  gtk_window_set_default_size (GTK_WINDOW (app), width, height);
  gpe_set_window_icon (app, "gpe-mini-browser-icon");

  //create boxes
  contentbox = gtk_vbox_new (FALSE, 0);

  //fill in status to be sure everything is filled in when used
  //status = (struct status_data*) malloc (sizeof (struct status_data));
  status.main_window = contentbox;
  status.pbar = NULL;
  status.exists = FALSE;

  //create toolbar and add to topbox
  toolbar = gtk_toolbar_new ();

  //create html object (must be created before function calls to html to avoid segfault)
  html = webi_new ();
  webi_set_emit_internal_status (WEBI (html), TRUE);

  set_default_settings (WEBI (html), &s);

  /* set rendering mode depending on screen size (when working in gtk-webcore) 
     if(width <=320)
     {
     webi_set_device_type (WEBI(html), WEBI_DEVICE_TYPE_HANDHELD);
     }
     else
     {
     webi_set_device_type (WEBI(html), WEBI_DEVICE_TYPE_SCREEN);
     } */

  /* Connect all the signals to the rendering object */
  /* cookies will only decently work when fixed in gtk-webcore */
  g_signal_connect (WEBI (html), "set_cookie", 
		    G_CALLBACK (handle_cookie), NULL);

  g_signal_connect (WEBI (html), "load_start",
		    G_CALLBACK (create_status_window), &status);

  g_signal_connect (WEBI (html), "load_stop",
		    G_CALLBACK (destroy_status_window), &status);

  g_signal_connect (WEBI (html), "status", 
                    G_CALLBACK (activate_statusbar), &status);

  g_signal_connect (WEBI (html), "title", 
 		    G_CALLBACK (set_title), app);

  /*add home,  back, forward, refresh / stop, url (small screen) */
  back_button = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
  gtk_tool_item_set_homogeneous (back_button, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), back_button, -1);

  forward_button = gtk_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  gtk_tool_item_set_homogeneous (forward_button, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), forward_button, -1);

  stop_reload_button = gtk_tool_button_new_from_stock (GTK_STOCK_REFRESH);
  gtk_tool_item_set_homogeneous (stop_reload_button, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), stop_reload_button, -1);

  home_button = gtk_tool_button_new_from_stock (GTK_STOCK_HOME);
  gtk_tool_item_set_homogeneous (home_button, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), home_button, -1);

  separator = gtk_separator_tool_item_new ();
  gtk_tool_item_set_homogeneous (separator, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), separator, -1);

#ifndef NOBOOKMARKS
  bookmarks_button = gtk_tool_button_new_from_stock (GTK_STOCK_INDENT);
  gtk_tool_item_set_homogeneous (bookmarks_button, FALSE);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (bookmarks_button),
			     _("Bookmarks"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), bookmarks_button, -1);

#endif

  /* only show full Url bar if the screen is bigger than 240x320 | 320x240 */
  if ((width > 320) || (height > 320))
    {
      urlbox = show_big_screen_interface (WEBI (html), toolbar, &s);
    }
  else
    {
      url_button = gtk_tool_button_new_from_stock (GTK_STOCK_NETWORK);
      gtk_tool_item_set_homogeneous (url_button, FALSE);
      gtk_tool_button_set_label (GTK_TOOL_BUTTON (url_button), "Url");
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), url_button, -1);
      separator2 = gtk_separator_tool_item_new ();
      gtk_tool_item_set_homogeneous (separator2, FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), separator2, -1);

      g_signal_connect (GTK_OBJECT (url_button), "clicked",
			G_CALLBACK (show_url_window), html);
    }


  /* replace GTK_STOCK_ZOOM_FIT with GTK_STOCK_FULLSCREEN once GPE uses
     gtk 2.7.1 or higher. Or add it myself :-) */
  fullscreen_button = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_FIT);
  gtk_tool_item_set_homogeneous (fullscreen_button, FALSE);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (fullscreen_button),
			     _("Fullscreen"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), fullscreen_button, -1);

  if ((width < 320) || (height <320))
    {
      add_zoom_buttons(WEBI(html), toolbar, &s);
    }

  /* connect all button signals */
  g_signal_connect (GTK_OBJECT (back_button), "clicked",
		    G_CALLBACK (back_func), html);

  g_signal_connect (GTK_OBJECT (forward_button), "clicked",
		    G_CALLBACK (forward_func), html);

  g_signal_connect (GTK_OBJECT (stop_reload_button), "clicked",
		    G_CALLBACK (stop_reload_func), html);

  g_signal_connect (GTK_OBJECT (home_button), "clicked",
		    G_CALLBACK (home_func), html);

  g_signal_connect (GTK_OBJECT (fullscreen_button), "clicked",
		    G_CALLBACK (set_fullscreen), app);
#ifndef NOBOOKMARKS
  g_signal_connect (GTK_OBJECT (bookmarks_button), "clicked",
		    G_CALLBACK (show_bookmarks), html);
#endif

  /* 
     DEBUG CODE!
     only show icons if the screen is 240x320 | 320x240 or smaller 
     if ((width <= 240) || (height <= 240))
     gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
     gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

     gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
   */
  gtk_box_pack_start (GTK_BOX (contentbox), toolbar, FALSE, FALSE, 0);
  if ((width > 320) || (height > 320))
    {
      gtk_box_pack_start (GTK_BOX (contentbox), urlbox, FALSE, FALSE, 0);
      gtk_widget_show_all (urlbox);
    }
  gtk_box_pack_start (GTK_BOX (contentbox), html, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (app), contentbox);

  if (base != NULL)
    fetch_url (base, html);

  g_free ((gpointer *) base);

  //make everything viewable
  gtk_widget_show_all (toolbar);
  gtk_widget_show_all (html);
  gtk_widget_show (GTK_WIDGET (contentbox));
  gtk_widget_show (GTK_WIDGET (app));
  gtk_main ();

  exit (0);
}
