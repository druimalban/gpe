/*
 * gpe-mini-browser v0.16
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

/*****************************************************************
			GENERAL INCLUDES
*****************************************************************/

/* General Defines */
#define HOME_PAGE "file:///usr/share/doc/gpe/mini-browser-index.html"

GtkToolItem *stop_reload_button;

/* contains the necessary data for passing urls between the different functions */
struct url_data {
GtkWidget *window;
GtkWidget *html;
GtkWidget *entry;
};

/* contains all the necessary data to make the progressbar work */
struct status_data {
GtkWidget *main_window;
GtkWidget *statusbox;
GtkWidget *pbar;
gboolean exists;
};

/* contains all the necessary data for the zoom functions */
struct zoom_data {
Webi *html;
WebiSettings *settings;
};

/********************************************************
	      interface call primitives 
	      see: interface-calls.c
*********************************************************/

/* show pop_up window for url input */
void show_url_window (GtkWidget * show, GtkWidget * html);
/* destroy the pop_up window */
void destroy_window (GtkButton * button, gpointer * window);
/* pop up the status window with status bar at the bottom of the screen */
void create_status_window (Webi * html, gpointer * status_data);
/* destroy the status window, making more screen estate available for viewing */
void destroy_status_window (Webi * html, gpointer * status_data);
/* make progressbar actually show progress */
void activate_statusbar (Webi * html, WebiLoadStatus * status, gpointer status_data);
/* set the webpage title in as the window title */
void set_title (Webi *html, GtkWidget *app_window);
/* show the current and correct url in the urlbox (big screen only) */
void update_text_entry (Webi *html, GtkWidget *entrybox);
/* show urlbar and extra buttons for big screens */
GtkWidget * show_big_screen_interface ( Webi *html, GtkWidget *toolbar, WebiSettings *set);

/********************************************************
	       url loading and handling
	       see: loading-calls.c 
*********************************************************/

/* fetch the parsed url and send it to the renderer */
void fetch_url (const gchar * url, GtkWidget * html);
/* parse the url. automatically add the http://, if it is a file check if it exists */
const gchar *parse_url (const gchar * url);
/* get the url from the entry box (url bar or pop-up window */
void load_text_entry (GtkWidget * Button, gpointer * text);
/* accept cookies further improvements depend on gtk-webcore cookie handling */
void handle_cookie (Webi * html, WebiCookie * cookie, gpointer * data);
/* go forward when forward button is pressed */
void forward_func (GtkWidget * forward, GtkWidget * html);
/* go backward if the back button is pressed */
void back_func (GtkWidget * back, GtkWidget * html);
/* go to the home page */
void home_func (GtkWidget * home, GtkWidget * html);
/* stop or reload the current page depending on status */
void stop_reload_func (GtkWidget * reload, GtkWidget * html);

/******************************************************
	            misc handling 
		    see: misc-calls.c
*******************************************************/

/* (re)set the settings to sane defaults for small screens */
void set_default_settings(Webi *html, WebiSettings *ks);
/* zoom in (basically making text size bigger)*/
void zoom_in(GtkWidget * zoom_in, gpointer *data);
/* zoom out (basically decreasing text size)*/
void zoom_out(GtkWidget * zoom_out, gpointer *data);
