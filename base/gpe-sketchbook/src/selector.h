/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef SELECTOR_H
#define SELECTOR_H

#include <gtk/gtk.h>

extern GtkWidget * window_selector;
extern GtkCList  * selector_clist; 
extern GdkColor bg_color;
void set_selector_clist(GtkCList * clist);

extern gint sketch_list_size;
#define is_sketch_list_empty (sketch_list_size == 0)

extern gint        current_sketch; //[0 .. list_size -1] to match CList indexes
extern gboolean is_current_sketch_selected;
#define set_current_sketch_selected()   (is_current_sketch_selected = TRUE)
#define set_current_sketch_unselected() (is_current_sketch_selected = FALSE)
#define SKETCH_NEW -1
#define SKETCH_LAST sketch_list_size -1
#define is_current_sketch_new   (current_sketch == SKETCH_NEW)
#define is_current_sketch_first (current_sketch == 0)
#define is_current_sketch_last  (current_sketch == sketch_list_size -1)
//extern gboolean is_current_sketch_modified;  set_current_sketch_[un]modified()

void selector_init();
void window_selector_init (GtkWidget * window_selector);
gchar * make_label_from_filename(const gchar * filename);

void open_indexed_sketch(gint index);
void delete_current_sketch();

#endif
