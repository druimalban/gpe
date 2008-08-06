/* 
 * gpe-mini-browser2 
 * - Basic web browser for GPE and small screen GTK devices based on WebKitGtk -
 * - This file contains the implementation of the main UI functions
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
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

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

/* Implementations of static functions related to the UI */

GtkWidget * create_toolbar(void)
{
  GtkWidget *toolbar;
  GtkToolItem *button;
  extern GtkToolItem *stop_reload_button;

  toolbar = gtk_toolbar_new();
  if (smallscreen)
  {
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
  }
  else
  {
    gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
  }	

  /* back button */
  button = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (back_cb), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);

  /* forward button */
  button = gtk_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  /* set forward not sensitive on start-up to make clear there is no way to go forward
  gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE); */
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (forward_cb), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);

  /* stop/refresh button */
  stop_reload_button = gtk_tool_button_new_from_stock (GTK_STOCK_REFRESH);
  g_signal_connect (G_OBJECT (stop_reload_button), "clicked", G_CALLBACK (stop_reload_cb), NULL);
  /* set id, so we can use it to toggle the button status and id */
  gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (stop_reload_button), "gtk-refresh");
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), stop_reload_button, -1);

  /* separator */
  button = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);

  /* preferences */
  button = gtk_tool_button_new_from_stock (GTK_STOCK_PREFERENCES);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (preferences_cb), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);

  /* fullscreen button */
  if(!kiosk_mode)
  {
  	/* separator for fullscreen button */
  	button = gtk_separator_tool_item_new ();
  	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);

  	button = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_FIT);
  	gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), "fullscreen");
  	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (fullscreen_cb), toolbar);
  	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);
  }

  button = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);

  button = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), "New tab");
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (new_tab_cb), toolbar);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), button, -1);

  return toolbar;
}

GtkWidget * create_urlbar(void)
{
  GtkWidget *urlbox, *urllabel, *okbutton, *close_tab_button;

  /* create all necessary widgets */
  urlbox = gtk_hbox_new (FALSE, 0);
  urllabel = gtk_label_new ((" Url:"));
  gtk_misc_set_alignment (GTK_MISC (urllabel), 0.0, 0.5);
  url_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (url_entry), TRUE);
  okbutton = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  close_tab_button = gpe_button_new_from_stock (GTK_STOCK_CLOSE, GPE_BUTTON_TYPE_ICON);

  /* pack everything in the hbox */
  gtk_box_pack_start (GTK_BOX (urlbox), urllabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (urlbox), url_entry, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (urlbox), okbutton, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (urlbox), close_tab_button, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (load_text_entry_cb), NULL);
  g_signal_connect (G_OBJECT (url_entry), "activate", G_CALLBACK (load_text_entry_cb), NULL);
  g_signal_connect (G_OBJECT (close_tab_button), "clicked", G_CALLBACK (close_tab_cb), NULL);

  gtk_widget_grab_focus (url_entry);
  /*final settings */
  GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
  gtk_button_set_relief (GTK_BUTTON (okbutton), GTK_RELIEF_NONE);
  gtk_widget_activate (okbutton);

  return (urlbox);
}

GtkWidget* create_htmlview(void)
{
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (web_view));

  g_signal_connect (G_OBJECT (web_view), "title-changed", G_CALLBACK (title_changed_cb), web_view);
  g_signal_connect (G_OBJECT (web_view), "load-progress-changed", G_CALLBACK (progress_changed_cb), web_view);
  g_signal_connect (G_OBJECT (web_view), "load-committed", G_CALLBACK (load_cb), web_view);
  g_signal_connect (G_OBJECT (web_view), "load-started", G_CALLBACK (load_start_cb), web_view);
  g_signal_connect (G_OBJECT (web_view), "load-finished", G_CALLBACK (load_stop_cb), web_view);
  g_signal_connect (G_OBJECT (web_view), "hovering-over-link", G_CALLBACK (link_hover_cb), web_view);

  return scrolled_window;
}


GtkWidget * create_tabs(void)
{
  notebook = GTK_NOTEBOOK(gtk_notebook_new());
  gtk_notebook_set_scrollable(notebook, TRUE);
  gtk_notebook_popup_enable(notebook);
  gtk_notebook_set_show_tabs(notebook, FALSE);
  /* evaluate if we want homogeneous tabs. Also the function
     is deprecated so we might run into issues.  
  */
  gtk_notebook_set_homogeneous_tabs(notebook, TRUE);
  gtk_container_add(GTK_CONTAINER(notebook), create_htmlview());
 
  /* initialize list to keep tabs and their associated web_views 
     the place in the list should be the same as the tab number  */
  tab_list = NULL;
  tab_list = g_list_append(tab_list, web_view);

  g_signal_connect (G_OBJECT (notebook), "page-added", G_CALLBACK (show_hide_tabs_cb), notebook);
  g_signal_connect (G_OBJECT (notebook), "page-removed", G_CALLBACK (show_hide_tabs_cb), notebook);
  g_signal_connect (G_OBJECT (notebook), "switch-page", G_CALLBACK (update_tab_data_cb), notebook); 

  return GTK_WIDGET(notebook);
}
