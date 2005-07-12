/*
 * gpe-mini-browser v0.13
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

/* General Defines */
#define HOME_PAGE "file:///usr/share/doc/gpe/mini-browser-index.html"

/* General include file */
struct url_data {
GtkWidget *window;
GtkWidget *html;
GtkWidget *entry;
};

struct status_data {
GtkWidget *main_window;
GtkWidget *statusbox;
GtkWidget *pbar;
gboolean exists;
};

/* interface call primitives */
extern void forward_func (GtkWidget * forward, GtkWidget * html);
extern void back_func (GtkWidget * back, GtkWidget * html);
extern void home_func (GtkWidget * home, GtkWidget * html);
extern void reload_func (GtkWidget * reload, GtkWidget * html);
extern void stop_func (GtkWidget * stop, GtkWidget * html);
extern void show_url_window (GtkWidget * show, GtkWidget * html);
extern void destroy_window (GtkButton * button, gpointer * window);
extern void create_status_window (Webi * html, gpointer * status_data);
extern void destroy_status_window (Webi * html, gpointer * status_data);
extern void activate_statusbar (Webi * html, WebiLoadStatus * status, gpointer status_data);



/* url loading and handling */
extern void fetch_url (const gchar * url, GtkWidget * html);
extern const gchar *parse_url (const gchar * url);
extern void load_text_entry (GtkWidget * Button, gpointer * text);
extern void handle_cookie (Webi * html, WebiCookie * cookie, gpointer * data);
