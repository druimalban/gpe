/*
 *  general_functions.c - this and that.
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

#include <string.h>

#include <gtk/gtk.h>

#include "galculator.h"
#include "general_functions.h"
#include "calc_basic.h"
#include "display.h"

#include "support.h"

GtkStatusbar 	*statusbar;
double			memory_value=0;

void statusbar_init (GtkWidget *a_parent_widget)
{
	int		context_id;
	
	statusbar = (GtkStatusbar*) lookup_widget (a_parent_widget, "statusbar");
	context_id = gtk_statusbar_get_context_id (statusbar, "standard statusbar");
	gtk_statusbar_push (statusbar, context_id, g_strdup_printf ("welcome to %s v%s", PROG_NAME, PROG_VERSION));
}

double error_unsupported_inv (double dummy)
{
	error_message ("unsupported inverse");
	return dummy;
}

double error_unsupported_hyp (double dummy)
{
	error_message ("unsupported hyperbolic function");
	return dummy;
}

void error_message (char *message)
{
	int		context_id;
	
	context_id = gtk_statusbar_get_context_id (statusbar, "error message");
	gtk_statusbar_push (statusbar, context_id, message);
}

/*NOT USED*/

char *kill_trailing_zeros (char *text)
{
	int		counter;
	
	if (strchr (text, '.') == NULL) return text;
	counter = strlen (text) - 1;
	while ((counter > 0) && (text[counter] == '0')) text[counter--] = '\0';
	if (text[counter] == '.') text[counter] = '\0';
	return text;
}

/* the last entered number is removed. it is impossible to remove the last operation:
 * this would be a quite difficult taks, as every possible computation is done asap.
 * so if you entered 1+2- and you want to correct to 1+2/ calc_basic would have already
 * calculated 1+2=3 and set the tree to 3-. you would have to make a backup of the tree!*/

void clear ()
{
	display_result_set ("0");
}

/* clear all: display ("0"), calc_tree ... */

void all_clear ()
{
	clear();
	/* additionally, forget all subtree computations */
	calc_tree_free();
}

void memory_save ()
{
	memory_value = display_result_get_as_double ();
}

void memory_read ()
{
	display_result_set_double (memory_value);
}

void memory_add2mem ()
{
	memory_value += display_result_get_as_double ();
	/* display this value ? */
}

void set_table_child_sensitivity (gpointer data, gpointer user_data)
{
	gboolean		*sensitive;
	GtkTableChild	*table_child;
	
	/* dereference them */
	sensitive = user_data;
	table_child = data;
	gtk_widget_set_sensitive (table_child->widget, *sensitive);
}

void set_button_group_sensitivity (GtkWidget *a_parent_widget, char *table_name, gboolean sensitive)
{
	GtkTable	*this_table;
	
	this_table = (GtkTable *) lookup_widget (a_parent_widget, table_name);
	g_list_foreach (this_table->children, set_table_child_sensitivity, &sensitive);
}