/*
 *  display.c - code for this nifty display.
 *	part of galculator
 *  	(c) 2002 Simon Floery (simon.floery@gmx.at)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

static GtkTextView 	*view;
static GtkTextBuffer 	*buffer;

/*
 * display_init. 
 */

void display_init (GtkWidget *widget)
{
	GtkTextTag 		*result_tag, *active_base_tag, *passive_base_tag;
	GtkTextIter 	iter;
	GdkColor		color;
	
	view = (GtkTextView *) widget;
	gdk_color_parse ("#e6edbd", &color);
	gtk_widget_modify_base ((GtkWidget *)view, GTK_STATE_NORMAL, &color);
	
	buffer = gtk_text_view_get_buffer (view);
	
	/* remark: wrap MUST NOT be set to none in order to justify the text! */
	
	result_tag = gtk_text_buffer_create_tag (buffer, "result", \
		"justification", GTK_JUSTIFY_RIGHT, \
		"weight", PANGO_WEIGHT_BOLD, \
		"scale", 1.8, \
		"pixels_above_lines", 1, \
		"pixels_below_lines", 1, \
		NULL);
	
	active_base_tag = gtk_text_buffer_create_tag (buffer, "active_base", \
		"scale", 0.8, \
	 	"weight", PANGO_WEIGHT_BOLD, \
		NULL);
	
	passive_base_tag = gtk_text_buffer_create_tag (buffer, "passive_base", \
		"scale", 0.8, \
		"foreground", "darkgrey", \
		NULL);

	gtk_text_buffer_get_start_iter (buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "0", -1, "result", NULL);
}


void display_result_set (char *string_value)
{
	GtkTextIter 	start, end;
	
	/* at first clear the result field */
	
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_delete (buffer, &start, &end);
	
	/* here we call no kill_trailing_zeros. we set the result_field to what we entered */
	gtk_text_buffer_insert_with_tags_by_name (buffer, &start, string_value, -1, "result", NULL);
}
