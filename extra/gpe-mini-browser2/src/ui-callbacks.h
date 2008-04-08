/* 
 * gpe-mini-browser2 
 * - Basic web browser for GPE and small screen GTK devices based on WebKitGtk -
 * - This file contains the declarations of the main UI callbacks 
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

gboolean main_window_key_press_event_cb (GtkWidget * widget, GdkEventKey * k,
                             		     GtkWidget * data);

void load_cb (WebKitWebView* page, WebKitWebFrame* frame, gpointer data);
void link_hover_cb (WebKitWebView* page, const gchar* title, const gchar* link, gpointer data);

void load_text_entry_cb (GtkWidget* widget, gpointer data);
void back_cb (GtkWidget* widget, gpointer data);
void forward_cb (GtkWidget* widget, gpointer data);
void stop_reload_cb (GtkWidget* widget, gpointer data);

void progress_changed_cb (WebKitWebView* page, gint progress, gpointer data);
void title_changed_cb (WebKitWebView* web_view, WebKitWebFrame* web_frame, const gchar* title, gpointer data);
void preferences_cb (GtkWidget* widget, gpointer data);
void fullscreen_cb (GtkWidget* widget, gpointer data);

void new_tab_cb (GtkWidget* widget, gpointer data);
void close_tab_cb (GtkWidget* widget, gpointer data);
void show_hide_tabs_cb (GtkNotebook* notebook, GtkWidget *child, guint tab_num, gpointer tab_data);
void update_tab_data_cb (GtkNotebook* notebook, GtkNotebookPage* page, guint tab_num, gpointer tab_data);

void browser_quit_cb (GtkWidget* widget, gpointer data);
