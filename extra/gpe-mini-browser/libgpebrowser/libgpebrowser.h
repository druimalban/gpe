/*
 * libgpebrowser - 0.1
 *
 * Basic web browser rendering library abstraction.
 * This is the generic include file.
 *
 * Copyright (c) 2007 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 *
 * Dedicated "The birthday Massacre" for entertaining music, 
 * and Nóra for inspiration.
 *
 * SUPPORT : If you like this library  and want it to be developed further,
 * your wishlist items implemented or just thank me, send me beer from my
 * native country (Belgium). (Very useful now I live in Finland)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <glib.h>

#ifndef __GPEBROWSER_H__
#define __GPEBROWSER_H__

G_BEGIN_DECLS

/*** Generic defines ***/
struct html_render_settings {
  gboolean javascript_enabled;
  gboolean java_enabled;
  gboolean plugins_enabled;
  gboolean cookies_enabled;
  gboolean autoload_images;
  int minimum_font_size;
  int default_font_size;
  int  default_fixed_font_size;
#if NEEDED
  const gchar *default_text_encoding;
  const gchar *serif_font_family;
  const gchar *sans_serif_font_family;
  const gchar *fixed_font_family;
  const gchar *standard_font_family;
  const gchar *cursive_font_family;
  const gchar *fantasy_font_family;
#endif /* NEEDED */
  const gchar *user_agent_string;
  const gchar* http_proxy;
};

/* possible status code */
typedef enum {
	LOADING_STARTED,
	LOADING,
	LOADING_COMPLETE,
	LOAD_ERROR
} render_state;

/* will be passed when a status change signal is emitted */
struct html_render_status {
  /* Status of the loading process. */
  render_state state;
  /* Number of files to download */
  int files;
  /* Number of files with Content-Length set*/
  int fileswithsize;
  /* Number of files received */
  int ready;
  /* Bytes to get from the resource */
  int size;
  /* Bytes received from the resource */
  int recieved;
  /* Total size of the resources including those that didn't have
      Content-Length set. */
  int totalsize;
  /* Bytes received total. */
  int totalrecieved
  /* Error or status strings */
  const gchar *statusmessage;
}

/*** Create usable widget which shows the rendered page for easy embedding ***/
GtkWidget *html_render_widget_new(void);

/*** setting widget properties ***/

/* enable passing internal status messages or not (should maybe move into the widget) */
void html_render_emit_internal_status(GtkWidget *renderer, gboolean enable);

/* get the current widget status (if supported! Returns NULL if not.) */
struct html_render_status * html_render_get_status(GtkWidget *renderer);

/* set settings like fontsize, proxy, javascript, plugins,  ... */
int html_render_set_settings(GtkWidget *renderer, struct html_render_settings *settings);

/* set user agent string (could also be used for user agent spoofing (0 = succes)*/
int html_render_set_user_agent_string(GtkWidget *renderer, const gchar *user_agent);

/* get user agent string */
const gchar * html_render_get_user_agent(GtkWidget *renderer);

/* enable special rendering modes (for small screens) */
gboolean html_render_get_small_screen(GtkWidget *renderer);

/* if supported set small screen rendering */
void html_render_set_small_screen_mode(GtkWidget *renderer, gboolean enable);

/* get current rendering mode (TRUE for small screen, FALSE for big screen */
gboolean html_render_get_screen_mode(GtkWidget *renderer);

/*** url loading/handling ***/
void html_render_load_url(GtkWidget *renderer, const gchar *url);

void html_render_stop (GtkWidget *renderer);

gboolean html_render_can_go_back(GtkWidget *renderer);

void html_render_back(GtkWidget *renderer);

gboolean html_render_can_go_forward(GtkWidget *renderer);

void html_render_forward(GtkWidget *renderer);

void html_render_refresh(GtkWidget *renderer);

/*** useful renderer utility functions ***/ 

/* returns the title of the currently loaded webpage */
const gchar * html_render_get_title(GtkWidget *renderer);

/* returns the url of the currently loaded webpage */
const gchar * html_render_get_url(GtkWidget *renderer);

/* find text in the renderer 
 * case_sensitive TRUE for case-sensitive search
 * down TRUE for downward searching 
 * returns TRUE on succes */
gboolean html_render_find(GtkWidget *renderer, const gchar *text, gboolean case_sensitive, gboolean down);

/* get the currently selected text */
const gchar * html_render_get_selected_text(GtkWidget *renderer);

G_END_DECLS

#endif /* __GPEBROWSER_H__ */
