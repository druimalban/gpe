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
#include <gtk/gtk.h>
#include "sketchpad-cb.h"

void switch_windows(GtkWidget * window_to_hide, GtkWidget * window_to_show);

//--toolbar
void on_window_selector_destroy        (GtkObject *object, gpointer  user_data);
void on_button_selector_new_clicked    (GtkButton *button, gpointer  user_data);
void on_button_selector_open_clicked   (GtkButton *button, gpointer  user_data);
void on_button_selector_delete_clicked (GtkButton *button, gpointer  user_data);
void on_button_selector_about_clicked  (GtkButton *button, gpointer  user_data);
void on_button_sketchpad_view_clicked  (GtkButton *button, gpointer  user_data);

//--clist
void on_clist_selector_select_row      (GtkCList  *clist,  gint row, gint column,
                                        GdkEvent  *event,  gpointer user_data);
void on_clist_selector_unselect_row    (GtkCList  *clist,  gint row, gint column,
                                        GdkEvent  *event,  gpointer user_data);
void on_list_sketch_files_select_child (GtkList   *list,   GtkWidget *widget, gpointer user_data);
void on_clist_selector_click_column    (GtkCList  *clist,  gint column,  gpointer user_data);

