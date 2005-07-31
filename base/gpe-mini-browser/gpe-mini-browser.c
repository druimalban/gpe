/*
 * gpe-mini-browser v0.15
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

static void set_fullscreen (GtkWidget *button, GtkWidget *app)
{
	if(!fullscreen)
	{
		gtk_window_fullscreen(GTK_WINDOW (app));
		fullscreen = 1;
	}
	else
	{
		gtk_window_unfullscreen(GTK_WINDOW (app));
		fullscreen = 0;
	}
}


int
main (int argc, char *argv[])
{
  GtkWidget *html, *app, *contentbox;	/* html engine, application window, content box of application window */
  GtkWidget *toolbar, *urlbox, *iconw;	/* toolbar, url entry box (big screen), icon for url pop-up window (small screens) */
  GtkWidget *back_button, *forward_button, *home_button, *search_button,
    *exit_button;
  const gchar *base;
  gchar *p;
  gint width = 240, height = 320;
  struct status_data *status;
  int opt;

  WebiSettings s = { 0, };

  gpe_application_init (&argc, &argv);

  while ((opt = getopt (argc, argv, "vh")) != -1)
    {
      switch (opt)
	{
	case 'v':
	  printf
	    ("GPE-mini-browser version 0.15. (C)2005, Philippe De Swert\n");
	  exit (0);

	default:
	  printf
	    ("GPE-mini-browser, basic web browser application. (c)2005, Philippe De Swert\n");
	  printf ("Usage: gpe-mini-browser <URL>\n");
	  printf ("Use -v for version info.\n");
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
  gtk_window_set_default_size (GTK_WINDOW (app), width, height);
  gtk_window_set_title (GTK_WINDOW (app), "mini-browser");
  gtk_window_set_resizable (GTK_WINDOW (app), TRUE);
  gtk_widget_realize (app);

  //create boxes
  contentbox = gtk_vbox_new (FALSE, 0);

  //fill in status to be sure everything is filled in when used
  status = (struct status_data*) malloc (sizeof (struct status_data));
  status->main_window = contentbox;
  status->exists = FALSE;

  //create toolbar and add to topbox
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 3);

  //create html object (must be created before function calls to html to avoid segfault)
  html = webi_new ();
  webi_set_emit_internal_status (WEBI (html), TRUE);

  set_default_settings(WEBI (html), &s );

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
  g_signal_connect (WEBI (html), "set_cookie", G_CALLBACK (handle_cookie),
		    NULL);

  g_signal_connect (WEBI (html), "load_start",
		    G_CALLBACK (create_status_window), status);

  g_signal_connect (WEBI (html), "load_stop",
		    G_CALLBACK (destroy_status_window), status);

  g_signal_connect (WEBI (html), "status", G_CALLBACK (activate_statusbar),
		    status);
  g_signal_connect (WEBI (html), "title", G_CALLBACK (set_title), app);

  /*add home, search, back, forward, refresh, stop, url and exit button */
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_BACK,
			    ("Go back a page"), ("Back"),
			    GTK_SIGNAL_FUNC (back_func), html, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_FORWARD,
			    ("Go to the next page"), ("Next"),
			    GTK_SIGNAL_FUNC (forward_func), html, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_REFRESH,
			    ("Reload the current page"), ("Reload"),
			    GTK_SIGNAL_FUNC (reload_func), html, -1);
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_STOP,
			    ("Stop loading this page"), ("Stop"),
			    GTK_SIGNAL_FUNC (stop_func), html, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_HOME,
			    ("Go to home page"), ("Home"),
			    GTK_SIGNAL_FUNC (home_func), html, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));	/* space after item */

  /* only show full Url bar if the screen is bigger than 240x320 | 320x240 */
  if ((width > 320) || (height > 320))
    {
	urlbox = show_big_screen_interface(WEBI (html), toolbar, &s);
    }
  else
    {
      iconw = gtk_image_new_from_stock (GTK_STOCK_NETWORK,
					GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), ("URL"),
			       ("Type in url"), ("URL"), iconw,
			       GTK_SIGNAL_FUNC (show_url_window), html);
      gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
    }
  /* replace GTK_STOCK_ZOOM_FIT with GTK_STOCK_FULLSCREEN once GPE uses
     gtk 2.7.1 or higher. Or add it myself :-) */
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_ZOOM_FIT,
			    ("Set Fullscreen"), ("Fullscreen"),
			    GTK_SIGNAL_FUNC (set_fullscreen), app , -1);

  /* remove this line that might override user settings for now
  gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar),
			     GTK_ICON_SIZE_SMALL_TOOLBAR);
  */  
 
  /* only show icons if the screen is 240x320 | 320x240 or smaller */
  if ((width <= 240) || (height <= 240))
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  //make everything viewable
  gtk_widget_show (toolbar);
  gtk_widget_show_all (html);
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

  gtk_widget_show (GTK_WIDGET (contentbox));
  gtk_widget_show (GTK_WIDGET (app));
  gtk_main ();

  exit (0);
}
