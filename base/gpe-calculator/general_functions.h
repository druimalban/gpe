/*
 *  general_functions.h
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

#define BIT(val, index) ((val & (1 << index)) >> index)

void statusbar_init (GtkWidget *a_parent_widget);
double error_unsupported_inv (double dummy);
double error_unsupported_hyp (double dummy);
void error_message (char *message);
char *kill_trailing_zeros (char *text);
void clear ();
void all_clear ();
void memory_save ();
void memory_read ();
void memory_add2mem ();
void set_button_group_sensitivity (GtkWidget *a_parent_widget, char *table_name, gboolean sensitive);
