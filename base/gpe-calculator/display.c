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

#include "galculator.h"
#include "display.h"
#include "general_functions.h"

#include "support.h"

static GtkTextView 		*view;
static GtkTextBuffer 	*buffer;
static GtkTextMark 		*mark_result_start, *mark_result_end;

static int display_result_counter = 0;

/*
 * display_init. 
 */

void display_init (GtkWidget *a_parent_widget)
{
	GtkTextIter 	iter;
	GdkColor		color;
	
	calc_entry_start_new = FALSE;
	
	view = (GtkTextView *) lookup_widget (a_parent_widget, "textview");
	gdk_color_parse ("#e6edbd", &color);
	gtk_widget_modify_base ((GtkWidget *)view, GTK_STATE_NORMAL, &color);
	
	buffer = gtk_text_view_get_buffer (view);
	
	/* remark: wrap MUST NOT be set to none in order to justify the text! */

	gtk_text_buffer_create_tag (buffer, "result", \
		"justification", GTK_JUSTIFY_RIGHT, \
		"weight", PANGO_WEIGHT_BOLD, \
		"scale", 1.8, \
		"pixels_above_lines", 1, \
		"pixels_below_lines", 1, \
		NULL);
	
	gtk_text_buffer_create_tag (buffer, "active_base", \
		"scale", 0.8, \
	 	"weight", PANGO_WEIGHT_BOLD, \
		NULL);
	
	gtk_text_buffer_create_tag (buffer, "passive_base", \
		"scale", 0.8, \
		"foreground", "darkgrey", \
		NULL);

	gtk_text_buffer_get_start_iter (buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "0", -1, "result", NULL);
	
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
	mark_result_start = gtk_text_buffer_create_mark (buffer, "result start", &iter, TRUE);
	
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, 1);
	mark_result_end = gtk_text_buffer_create_mark (buffer, "result end", &iter, FALSE);
}

/*
 * display_result_add_digit. appends the given digit to the current entry, handles zeros, decimal
 *		points. call e.g. with *(gtk_button_get_label (button))
 */

void display_result_add_digit (char digit)
{
	char			digit_as_string[2];
	GtkTextIter		end;
	
	digit_as_string[0] = digit;
	digit_as_string[1] = '\0';
	
	if (calc_entry_start_new == TRUE)
	{
		/* fool the following code */
		display_result_set ("0");
		calc_entry_start_new = FALSE;
		display_result_counter = 1;
	}
	if (digit == '.') 
	{
		/* don't manipulate display_result_counter here! */
		if (strlen (display_result_get()) == 0) display_result_set ("0");
		else if ((strchr (display_result_get(), '.') == NULL) && \
					(strchr (display_result_get(), 'e') == NULL))
		{
			gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
			gtk_text_buffer_insert_with_tags_by_name (buffer, &end, digit_as_string, -1, "result", NULL);
		}
	}
	else if (display_result_counter < DISPLAY_RESULT_PRECISION)
	{
		if (strcmp (display_result_get(), "0") == 0) display_result_set (digit_as_string);
		else 
		{
			gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
			gtk_text_buffer_insert_with_tags_by_name (buffer, &end, digit_as_string, -1, "result", NULL);
			/* increment counter only in this if directive as above the counter remains 1! */
			display_result_counter++;
		}
	}
}

/*
 * display_result_set_double. set text of calc_entry to string given by float value.
 *	the float value is manipulated (rounded, ...)
 */

void display_result_set_double (double value)
{
	GtkTextIter 	start, end;
	char			*string_value;
	
	/* at first clear the result field */
	
	gtk_text_buffer_get_iter_at_mark (buffer, &start, mark_result_start);
	gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
	gtk_text_buffer_delete (buffer, &start, &end);
	
	/* then set it to the new value */
	
	gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
	string_value = g_strdup_printf ("%.*g", DISPLAY_RESULT_PRECISION, value);
	gtk_text_buffer_insert_with_tags_by_name (buffer, &end, string_value, -1, "result", NULL);
	display_result_counter = strlen (string_value);
	g_free (string_value);
}

void display_result_set (char *string_value)
{
	GtkTextIter 	start, end;
	
	/* at first clear the result field */
	
	gtk_text_buffer_get_iter_at_mark (buffer, &start, mark_result_start);
	gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
	gtk_text_buffer_delete (buffer, &start, &end);
	
	/* then set it to the new value */
	gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
	/* here we call no kill_trailing_zeros. we set the result_field to what we entered */
	gtk_text_buffer_insert_with_tags_by_name (buffer, &end, string_value, -1, "result", NULL);
	display_result_counter = strlen (string_value);
	
	/* this is some cosmetics. try to keep counter up2date */
	if (strchr (string_value, '.') != NULL) display_result_counter--;
	if (strchr (string_value, 'e') != NULL) 
	{
		display_result_counter -= (strchr(string_value, 'e') + sizeof(char) - string_value)/sizeof(char);
		display_result_counter += DISPLAY_RESULT_PRECISION - DISPLAY_RESULT_E_LENGTH - 1;
	}
}

/*
 * display_get_entry. returns a pointer to the current entry text. should be freed with g_free.
 */

char *display_result_get ()
{
	GtkTextIter 	start, end;
	
	gtk_text_buffer_get_iter_at_mark (buffer, &start, mark_result_start);
	gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
	return gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
}

double display_result_get_as_double ()
{
	return atof(display_result_get());
}	

void display_append_e ()
{
	GtkTextIter		end;
	
	if (calc_entry_start_new == FALSE)
	{
		if (strstr (display_result_get(), "e+") == NULL) \
		{
			gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
			gtk_text_buffer_insert_with_tags_by_name (buffer, &end, "e+", -1, "result", NULL);
		}
	}
	else 
	{
		display_result_set ("0e+");
		calc_entry_start_new = TRUE;
	}
	display_result_counter = DISPLAY_RESULT_PRECISION - DISPLAY_RESULT_E_LENGTH;
}

void display_result_toggle_sign ()
{
	GtkTextIter		start, end;
	char			*result_field, *e_pointer;
	
	/* we could call display_result_get but we need start iterator later on anyway */
	gtk_text_buffer_get_iter_at_mark (buffer, &start, mark_result_start);
	gtk_text_buffer_get_iter_at_mark (buffer, &end, mark_result_end);
	result_field = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
	/* if there is no e? we toggle the leading sign, otherwise the sign after e */
	if ((e_pointer = strchr (result_field, 'e')) == NULL)
	{
		if (*result_field == '-')
		{
			gtk_text_buffer_get_iter_at_offset (buffer, &end, gtk_text_iter_get_offset (&start) + 1);
			gtk_text_buffer_delete (buffer, &start, &end);
		}
		else
		{
			if (strcmp (result_field, "0") != 0) \
				gtk_text_buffer_insert_with_tags_by_name (buffer, &start, "-", -1, "result", NULL);
		}
	}
	else
	{
		if (*(++e_pointer) == '-') *e_pointer = '+';
		else *e_pointer = '-';
		display_result_set (result_field);
	}
}

/* display_result_backspace - deletes the tail of the display.
 *		sets the display with display_result_set => no additional manipulation of 
 *		display_result_counter
 *		necessary
 */

void display_result_backspace ()
{															
	char	*current_entry;
	
	if (calc_entry_start_new == TRUE) 
	{
		calc_entry_start_new = FALSE;
		display_result_set ("0");
	}
	else
	{
		current_entry = display_result_get();
		/* to avoid an empty/senseless result field */
		if (strlen(current_entry) == 1) current_entry[0] = '0';
		else if ((strlen(current_entry) == 2) && (*current_entry == '-')) current_entry = "0\0";
		else if (current_entry[strlen(current_entry) - 2] == 'e') current_entry[strlen(current_entry) - 2] = '\0';
		else current_entry[strlen(current_entry) - 1] = '\0';
		display_result_set (current_entry);
		g_free (current_entry);
	}
}

