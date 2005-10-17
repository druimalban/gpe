/*
 * gpe-mini-browser v0.17
 *
 * Basic web browser based on gtk-webcore.
 * Hildon interface version for Maemo / Nokia 770
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * Dedicated to Dark Tranquility for inspiration while struggling
 * with Maemo "specials".
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
#include <stdlib.h>
#include <getopt.h>
#include <libintl.h>

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

/* Hildon includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <libosso.h>

//#define DEBUG /* uncomment this if you want debug output*/

/*
static void
fullscreen (GtkWidget * button, HildonAppView * app)
{
  gboolean fullscreen;

  fullscreen = hildon_appview_get_fullscreen (app);
  if (!fullscreen)
    hildon_appview_set_fullscreen (app, TRUE);
  else
    hildon_appview_set_fullscreen (app, FALSE);

}*/

static void
set_fullscreen (GtkWidget * button, gpointer * fullscreen_info)
{
  static int fullscreen;
  static int totalscreen;
  struct fullscreen_info *info;

  info = (struct fullscreen_info *) fullscreen_info;

  if (!fullscreen)
    {
      gtk_window_fullscreen (GTK_WINDOW (info->app));
      fullscreen = 1;
    }
  else if (fullscreen && !totalscreen)
    {
      GtkWidget *close_button, *fullscreen_popup;

      totalscreen = 1;
      gtk_widget_hide (info->toolbar);
      if (info->urlbox)
	gtk_widget_hide (info->urlbox);
      fullscreen_popup = gtk_window_new (GTK_WINDOW_POPUP);

      close_button =
	gpe_button_new_from_stock (GTK_STOCK_ZOOM_FIT, GPE_BUTTON_TYPE_ICON);
      g_signal_connect (G_OBJECT (close_button), "clicked",
			G_CALLBACK (set_fullscreen), fullscreen_info);
      g_signal_connect (G_OBJECT (close_button), "clicked",
			G_CALLBACK (destroy_window), fullscreen_popup);
      gtk_container_add (GTK_CONTAINER (fullscreen_popup), close_button);
      gtk_widget_show_all (fullscreen_popup);

    }
  else if (totalscreen == 1 && fullscreen == 1)
    {
      gtk_window_unfullscreen (GTK_WINDOW (info->app));
      gtk_widget_show_all (info->toolbar);
      if (info->urlbox)
	gtk_widget_show_all (info->urlbox);
      fullscreen = 0;
      totalscreen = 0;
    }
}



static void
osso_top_callback (const gchar * arguments, gpointer ptr)
{
  GtkWindow *window = ptr;

  gtk_window_present (GTK_WINDOW (window));
}


int
main (int argc, char *argv[])
{
  HildonApp *app = NULL;
  HildonAppView *mainview = NULL;


  GtkWidget *html, *contentbox;	/* html engine, application window, content box of application window */
  GtkWidget *toolbar, *urlbox;	/* toolbar, url entry box (big screen), icon for url pop-up window (small screens) */
  GtkToolItem *back_button, *forward_button, *home_button, *fullscreen_button,
    *bookmarks_button;
  GtkToolItem *separator;
  extern GtkToolItem *stop_reload_button;
  const gchar *base;
  static struct status_data status;
  static struct fullscreen_info fsinfo;
  int opt;
  osso_context_t *context;

  WebiSettings s = { 0, };

  /* osso stuff  */
  context = osso_initialize ("gpe-mini-browser", "0.17", TRUE, NULL);
  if (context == NULL)
    {
      fprintf (stderr, "osso_initialize failed.\n");
      exit (1);
    }

  gpe_application_init (&argc, &argv);

  while ((opt = getopt (argc, argv, "vh")) != -1)
    {
      switch (opt)
	{
	case 'v':
	  printf
	    (_
	     ("GPE-mini-browser version 0.17. (C)2005, Philippe De Swert\n"));
	  exit (0);

	default:
	  printf
	    (_
	     ("GPE-mini-browser, basic web browser application. (c)2005, Philippe De Swert\n"));
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
  app = HILDON_APP (hildon_app_new ());
  hildon_app_set_two_part_title (HILDON_APP (app), FALSE);
  hildon_app_set_title (app, ("mini-browser"));
  mainview = HILDON_APPVIEW (hildon_appview_new ("main_view"));
  osso_application_set_top_cb (context, osso_top_callback,
			       (gpointer) mainview);

  hildon_app_set_appview (app, mainview);
  gtk_widget_show_all (GTK_WIDGET (app));
  gtk_widget_show_all (GTK_WIDGET (mainview));

  hildon_appview_set_fullscreen_key_allowed (mainview, TRUE);

  //create boxes
  contentbox = gtk_vbox_new (FALSE, 0);

  //fill in status to be sure everything is filled in when used
  status.main_window = contentbox;
  status.pbar = NULL;
  status.exists = FALSE;

  //create toolbar and add to topbox
  toolbar = gtk_toolbar_new ();
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 3);
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

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
  g_signal_connect (WEBI (html), "set_cookie", G_CALLBACK (handle_cookie),
		    NULL);

  g_signal_connect (WEBI (html), "load_start",
		    G_CALLBACK (create_status_window), &status);

  g_signal_connect (WEBI (html), "load_stop",
		    G_CALLBACK (destroy_status_window), &status);

  g_signal_connect (WEBI (html), "status", G_CALLBACK (activate_statusbar),
		    &status);
  g_signal_connect (WEBI (html), "title", G_CALLBACK (set_title), app);

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

#ifndef NOBOOKMARKS
  separator = gtk_separator_tool_item_new ();
  gtk_tool_item_set_homogeneous (separator, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), separator, -1);

  bookmarks_button = gtk_tool_button_new_from_stock (GTK_STOCK_INDENT);
  gtk_tool_item_set_homogeneous (bookmarks_button, FALSE);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (bookmarks_button),
			     _("Bookmarks"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), bookmarks_button, -1);

#endif


  hildon_appview_set_toolbar (mainview, GTK_TOOLBAR (toolbar));

  urlbox = show_big_screen_interface (WEBI (html), toolbar, &s);

  /* fill in fullscreen info */
  fsinfo.app = GTK_WIDGET (app);
  fsinfo.toolbar = toolbar;
  fsinfo.urlbox = urlbox;

/* replace GTK_STOCK_ZOOM_FIT with GTK_STOCK_FULLSCREEN once GPE uses
     gtk 2.7.1 or higher. Or add it myself :-) */
  fullscreen_button = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_FIT);
  gtk_tool_item_set_homogeneous (fullscreen_button, FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), fullscreen_button, -1);

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
		    G_CALLBACK (set_fullscreen), &fsinfo);

#ifndef NOBOOKMARKS
  g_signal_connect (GTK_OBJECT (bookmarks_button), "clicked",
		    G_CALLBACK (show_bookmarks), html);
#endif

//  gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
//  toolbar size seems to be the same, icons are clearer

  gtk_box_pack_start (GTK_BOX (contentbox), urlbox, FALSE, FALSE, 0);
  gtk_widget_show_all (urlbox);

  gtk_box_pack_start (GTK_BOX (contentbox), html, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (mainview), contentbox);

  if (base != NULL)
    fetch_url (base, html);

  g_free ((gpointer *) base);

  //make everything viewable
  gtk_widget_show_all (GTK_WIDGET (contentbox));
  gtk_widget_show_all (toolbar);
  gtk_widget_show_all (html);
  gtk_main ();

  exit (0);
}
