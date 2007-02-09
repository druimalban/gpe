/*
 * libgpebrowser - 0.1
 *
 * Basic web browser rendering library abstraction.
 * This is the gtk-webcore abstraction layer.
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

#include <libgpebrowser.h>

/*** gtk-webcore specific includes ***/
#include <webi.h>

/*** Create usable widget which shows the rendered page for easy embedding ***/
GtkWidget *html_render_widget_new(void)
{
 /* webi_new already returns a GtkWidget 
 TODO: Find out how to handle the signals
 TODO: Javascript callback handling
 */
 return webi_new();
}

/*** setting widget properties ***/

/* enable passing internal status messages or not (should maybe move into the widget) */
void html_render_emit_internal_status(GtkWidget *renderer, gboolean enable)
{
 if(enable)
	webi_set_emit_internal_status (WEBI (html), TRUE);
 else
	webi_set_emit_internal_status (WEBI (html), FALSE);
}

/* get the current widget status (if supported! Returns NULL if not.) */
struct html_render_status * html_render_get_status(GtkWidget *renderer)
{
 /* not implemented */
 return NULL;
}

/* set settings like fontsize, proxy, javascript, plugins,  ... */
int html_render_set_settings(GtkWidget *renderer, struct html_render_settings *settings)
{
 /* TODO: settings conversion if necessary */
 webi_set_settings(WEBI(renderer), settings);
 /* assume that the settings get applied */
 return 0;
}

/* set user agent string (could also be used for user agent spoofing (0 = succes)*/
int html_render_set_user_agent_string(GtkWidget *renderer, const gchar *user_agent)
{
 WebiSettings settings;

 strcpy(settings->user_agent_string, user_agent);
 webi_set_settings(WEBI(renderer), &settings);
 /* assume success */
 return 0;
}

/* get user agent string */
const gchar * html_render_get_user_agent(GtkWidget *renderer)
{
 return const gchar* webi_get_engine_user_agent_string ();
}

/* enable special rendering modes (for small screens) */
gboolean html_render_get_small_screen(GtkWidget *renderer)
{
 /* not implemented */
 return 0;
}

/* if supported set small screen rendering */
void html_render_set_small_screen_mode(GtkWidget *renderer, gboolean enable)
{
 /* would be nice if it ever works */
 if(enable)
	webi_set_device_type(WEBI(renderer), WEBI_DEVICE_TYPE_HANDHELD);
 else
	webi_set_device_type(WEBI(renderer), WEBI_DEVICE_TYPE_SCREEN);
} 

/* get current rendering mode (TRUE for small screen, FALSE for big screen */
gboolean html_render_get_screen_mode(GtkWidget *renderer)
{
 if( webi_get_device_type(WEBI(renderer)))
	return TRUE;
 else
	return FALSE;
}


/*** url loading/handling ***/
void html_render_load_url(GtkWidget *renderer, const gchar *url)
{
 webi_load_url(WEBI(renderer), url);
}

void html_render_stop (GtkWidget *renderer)
{
 webi_Stop_load(WEBI(renderer));
}

gboolean html_render_can_go_back(GtkWidget *renderer)
{
 webi_can_go_back(WEBI(renderer));
}

void html_render_back(GtkWidget *renderer)
{
 webi_go_back(WEBI(renderer));
}

gboolean html_render_can_go_forward(GtkWidget *renderer)
{
 webi_can_go_forward(WEBI(renderer));
}

void html_render_forward(GtkWidget *renderer)
{
 webi_go_forward(WEBI(renderer));
}

void html_render_refresh(GtkWidget *renderer)
{
 webi_refresh(WEBI(renderer));
}

/*** useful renderer utility functions ***/ 

/* returns the title of the currently loaded webpage */
const gchar * html_render_get_title(GtkWidget *renderer)
{
 return webi_get_title(WEBI(renderer));
}

/* returns the url of the currently loaded webpage */
const gchar * html_render_get_url(GtkWidget *renderer)
{
 return webi_get_location(WEBI(renderer));
}

/* find text in the renderer 
 * case_sensitive TRUE for case-sensitive search
 * down TRUE for downward searching 
 * returns TRUE on succes */
gboolean html_render_find(GtkWidget *renderer, const gchar *text, gboolean case_sensitive, gboolean down)
{
 return webi_find (WEBI(renderer), text, case_sensitive, dir_down);
}

/* get the currently selected text */
const gchar * html_render_get_selected_text(GtkWidget *renderer)
{
 return webi_get_current_selection_as_text(WEBI(renderer));
}
