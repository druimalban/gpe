/*
 * gpe-mini-browser v0.1
 *
 * Basic web browser based on gtk-webcore 
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>

#include "gpe-mini-browser.h"

#define DEBUG /* uncomment this if you want debug output*/


int
main (int argc, char *argv[])
{
  GtkWidget *html;
  GtkWidget *app;
  GtkWidget *contentbox;
  GtkWidget *toolbar;
  GtkWidget *iconw;
  GtkWidget *back_button, *forward_button, *home_button, *search_button,
    *exit_button;
  const gchar *base;
  gchar *p;
  gint width = 240, height = 320;

  WebiSettings s = { 0, };
  WebiSettings *ks = &s;

  gpe_application_init (&argc, &argv);

  if (argc > 2)
    {

      printf
	("GPE-mini-browser, basic web browser application. (c)2005, Philippe De Swert\n");
      printf ("Usage: gpe-minibrowser\n");
      exit (0);
    }

  if( argv[1] != NULL)
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

  //create toolbar and add to topbox
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  //create html object (must be created before function calls to html to avoid segfault)
  html = webi_new ();
  webi_set_emit_internal_status (WEBI (html), TRUE);

  ks->default_font_size = 11;
  ks->default_fixed_font_size = 11;
  ks->autoload_images = 1;
  webi_set_settings (WEBI (html), ks);

  /*add home, search, back, forward and exit button */
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

  /* TODO: Actually implement search function 
     iconw = gtk_image_new_from_file ("help/search.png"); 
     search_button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Find", "Find a word in the text","Find",
     iconw, GTK_SIGNAL_FUNC (find_func), NULL); */

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_HOME,
			    ("Go to home page"), ("Home"),
			    GTK_SIGNAL_FUNC (home_func), html, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));	/* space after item */

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NETWORK,
			    ("Type in url"), ("URL"),
			    GTK_SIGNAL_FUNC (show_url_window), html, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    ("Exit gpe-mini-browser"), ("Exit"),
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL, -1);

  gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar),
			     GTK_ICON_SIZE_SMALL_TOOLBAR);
  /* only show icons if the screen is 240x320 | 320x240 or smaller */
  if ((width <= 240) || (height <= 240))
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_widget_show (toolbar);

  //create html reader
  gtk_widget_show_all (html);

  //make everything viewable
  gtk_box_pack_start (GTK_BOX (contentbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (contentbox), html, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (app), contentbox);

  if (base != NULL)
	  fetch_url (base, html);

  gtk_widget_show (GTK_WIDGET (contentbox));
  gtk_widget_show (GTK_WIDGET (app));
  gtk_main ();

  exit (0);
}
