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

/* General include file */
struct url_data {
GtkWidget *window;
GtkWidget *html;
GtkWidget *entry;
};

/* interface call primitives */
extern void forward_func (GtkWidget * forward, GtkWidget * html);
extern void back_func (GtkWidget * back, GtkWidget * html);
extern void home_func (GtkWidget * home, GtkWidget * html);
extern void reload_func (GtkWidget * reload, GtkWidget * html);
extern void stop_func (GtkWidget * stop, GtkWidget * html);
extern void show_url_window (GtkWidget * show, GtkWidget * html);
extern void destroy_window (GtkButton * button, gpointer * window);



/* url loading and handling */
extern void fetch_url (const gchar * url, GtkWidget * html);
extern const gchar *parse_url (const gchar * url);
extern void load_text_entry (GtkWidget * Button, gpointer * text);
extern void handle_cookie (Webi * html, WebiCookie * cookie, gpointer * data);
