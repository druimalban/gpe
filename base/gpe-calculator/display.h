/*
 *  display.h
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
 
#define DISPLAY_RESULT_PRECISION	12
#define DISPLAY_RESULT_E_LENGTH		5

/* general */

void display_init (GtkWidget *a_parent_widget);

/* the result field */

void display_result_add_digit (char digit);
void display_result_set (char *string_value);
void display_result_set_double (double value);
char *display_result_get ();
double display_result_get_as_double ();
void display_append_e ();
void display_result_toggle_sign ();
void display_result_backspace ();

gboolean calc_entry_start_new;