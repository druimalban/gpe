/*
 * gpe-mini-browser2 v0.0.1
 *
 * - Basic web browser for GPE and small screen GTK devices based on WebKitGtk -
 * - This file contains the browser wide includes
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

/*****************************************************************
                        GENERAL INCLUDES
*****************************************************************/

/* Global variables */

WebKitWebView *web_view; /* every new window/tab needs its own html render object */
GtkWidget *url_entry; /* keeping track of the url entry box */
GtkWidget *main_window; /* for fullscreen */
GtkWidget *main_window_vbox; /* for packing and removing the loading progressbar */
GtkNotebook *notebook; /* for adding and removing tabs */
GtkWidget *current_tab; /* to modify the current tab */
GList *tab_list; /* the list that contains the tabs */

extern bool kiosk_mode; /*global boolean for kiosk mode */
extern bool smallscreen;
extern bool active_pbar; /* announce active pbar, to avoid several loading bars */

GtkToolItem *stop_reload_button;

struct status_box
{
  WebKitWebView* web_view;
  bool pbar_exists;
  GtkWidget *statusbox; /* contains the progressbar */
  GtkWidget *pbar; /* progressbar */
};
