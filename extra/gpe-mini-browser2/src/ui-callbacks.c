/* 
 * gpe-mini-browser2 
 * - Basic web browser for GPE and small screen GTK devices based on WebKitGtk -
 * - This file contains the implementation of the main UI callbacks
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

gboolean main_window_key_press_event_cb (GtkWidget * widget, GdkEventKey * k, GtkWidget * data)
{
  switch (k->keyval)
    {
    case GDK_BackSpace:
      back_cb(widget, data);
      return TRUE;
    case GDK_t:
      new_tab_cb(NULL, NULL);
      return TRUE;		
    default:
      return FALSE;

    }
  /* we should not get here */
  return FALSE;
}

void load_start_cb (WebKitWebView* page, WebKitWebFrame* frame, gpointer data)
{
  /* when loading starts show stop button */
  gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (stop_reload_button),
                                "gtk-stop");
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (stop_reload_button),
                                   NULL);
  
}

void load_stop_cb (WebKitWebView* page, WebKitWebFrame* frame, gpointer data)
{
  /* when loading stops show refresh/reload button again */
  gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (stop_reload_button),
                                "gtk-refresh");
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (stop_reload_button),
                                   NULL);
  active_pbar = FALSE;
}

void load_cb (WebKitWebView* page, WebKitWebFrame* frame, gpointer data)
{
  const gchar *url = webkit_web_frame_get_uri(frame);

  if (url)
      gtk_entry_set_text (GTK_ENTRY (url_entry), url);
}

void link_hover_cb (WebKitWebView* page, const gchar* title, const gchar* link, gpointer data)
{
    /* underflow is allowed
    gtk_statusbar_pop (main_statusbar, status_context_id);
    if (link)
        gtk_statusbar_push (main_statusbar, status_context_id, link);
   */
}

void load_text_entry_cb (GtkWidget* widget, gpointer data)
{
  const gchar *url = gtk_entry_get_text (GTK_ENTRY (url_entry));
  if (!strcmp(url,"about:"))
  {
     gpe_info_dialog (("GPE mini-browser v0.0.1\n\nHTML engine : WebKitGtk \n\nCopyright (c)  2008 \n\nPhilippe De Swert\n<philippedeswert@scarlet.be>\n"));
      return;

  }
  else
    webkit_web_view_open (web_view, parse_url(url));
}


/* navigation callbacks */

void back_cb (GtkWidget* widget, gpointer data)
{
  webkit_web_view_go_back (web_view);
}

void forward_cb (GtkWidget* widget, gpointer data)
{
  webkit_web_view_go_forward (web_view);
}

void stop_reload_cb (GtkWidget* widget, gpointer data)
{
  const gchar *id;

  id = gtk_tool_button_get_stock_id (GTK_TOOL_BUTTON (stop_reload_button));
  if (!strcmp (id, "gtk-refresh"))
    webkit_web_view_reload (web_view);
  else
    webkit_web_view_stop_loading (web_view);

}

/* ui action callbacks */

void progress_changed_cb (WebKitWebView* page, gint progress, gpointer data)
{
//  if(active_pbar)
//    gtk_progress_bar_set_fraction ((GtkProgressBar *) pbar, (double) progress/100);
}

void title_changed_cb (WebKitWebView* web_view, WebKitWebFrame* web_frame, const gchar* title, gpointer data)
{
 GtkWidget * label;
 gchar *title2 = malloc(sizeof(char) * 26);
 
#ifdef DEBUG
 printf("title = %s\n", title);
#endif /* DEBUG */
 strncpy (title2, title, 25);
 title2[25] = '\0';
 label = gtk_label_new(title2);
#ifdef DEBUG
 printf("title = %s\n", title2);
#endif /* DEBUG */
 title = g_strconcat(title, " - mini-browser2", NULL);
 gtk_window_set_title(GTK_WINDOW(main_window), title);
 g_free(title2);
 
 gtk_notebook_set_tab_label(notebook, gtk_notebook_get_nth_page(notebook,  gtk_notebook_get_current_page(notebook)), label);
}

void preferences_cb (GtkWidget* widget, gpointer data)
{
  /* Not implemented yet warning */
  gpe_info_dialog ("Sorry. Not implemented yet.\n");
}

void fullscreen_cb (GtkWidget* widget, gpointer data)
{
  static bool fullscreen_status = FALSE;
  static bool toolbar_hidden = FALSE;
  static GtkWidget *unfullscreen_button, *fullscreen_popup = NULL;

  if(!fullscreen_status)
  {
    gtk_window_fullscreen(GTK_WINDOW(main_window));
    fullscreen_status = TRUE;
  }
  else if(fullscreen_status && !toolbar_hidden)
  {
    gtk_widget_hide(GTK_WIDGET(data));
    fullscreen_popup = gtk_window_new (GTK_WINDOW_POPUP);
    unfullscreen_button = gpe_button_new_from_stock (GTK_STOCK_ZOOM_FIT, GPE_BUTTON_TYPE_ICON);
    /* the callback will keep the reference, which is very handy :) */
    g_signal_connect (G_OBJECT (unfullscreen_button), "clicked", G_CALLBACK (fullscreen_cb), data);
    gtk_container_add (GTK_CONTAINER (fullscreen_popup), unfullscreen_button);
    gtk_widget_show_all (fullscreen_popup);
    toolbar_hidden = TRUE;
  }
  else
  {
    gtk_widget_destroy(fullscreen_popup);
    gtk_widget_show(GTK_WIDGET(data));
    gtk_window_unfullscreen(GTK_WINDOW(main_window));
    fullscreen_status = FALSE;
    toolbar_hidden = FALSE;
  }
}

void new_tab_cb (GtkWidget* widget, gpointer data)
{
  int tab_nr;

  tab_nr = gtk_notebook_append_page(notebook, create_htmlview(), NULL);
  tab_list = g_list_append(tab_list, web_view);

  gtk_notebook_set_show_tabs(notebook, TRUE);
  gtk_notebook_set_tab_label(notebook, GTK_WIDGET(web_view), add_close_button());
  gtk_widget_show_all (GTK_WIDGET(notebook));
  gtk_notebook_set_current_page(notebook, tab_nr);
  gtk_entry_set_text (GTK_ENTRY(url_entry), "");

}

void close_tab_cb (GtkWidget* widget, gpointer data)
{
  int tab_number;
  gpointer *web_view_data;

  /* TODO: last tab hides close button. */ 
  if((gtk_notebook_get_n_pages(notebook)) == 1) 
	return; 

  tab_number = gtk_notebook_get_current_page(notebook);
  gtk_notebook_remove_page(notebook, tab_number);
  web_view_data = g_list_nth_data(tab_list, tab_number);  
  tab_list = g_list_remove(tab_list, web_view_data);
}

void show_hide_tabs_cb (GtkNotebook *notebook, GtkWidget *child, guint tab_num, gpointer tab_data)
{
  int tab_amount = 0;

  tab_amount = gtk_notebook_get_n_pages(notebook);
  if(tab_amount == 1)
        gtk_notebook_set_show_tabs(notebook, FALSE);
  else
        gtk_notebook_set_show_tabs(notebook, TRUE);

}

void update_tab_data_cb (GtkNotebook* notebook, GtkNotebookPage* page, guint tab_num, gpointer tab_data)
{
  WebKitWebFrame *frame;

  web_view = g_list_nth_data(tab_list, tab_num);
  frame = webkit_web_view_get_main_frame(web_view);

  const gchar *url = webkit_web_frame_get_uri(frame);
  if (url)
      gtk_entry_set_text(GTK_ENTRY (url_entry), url);  
  else
      gtk_entry_set_text(GTK_ENTRY (url_entry), " ");
  url = webkit_web_frame_get_title(frame);
  if (url)
  {
     url = g_strconcat(url, " - mini-browser2", NULL);
     gtk_window_set_title(GTK_WINDOW(main_window), url);
  }
  g_free((gpointer *)url);
}

void browser_quit_cb (GtkWidget* widget, gpointer data)
{
  gtk_main_quit();
}

