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

void switch_to_page(guint page);
enum {ACTION_CANCELED, ACTION_DONE};

//--toolbar
void on_window_selector_destroy        (GtkObject *object, gpointer  user_data);
void on_button_selector_exit_clicked   (GtkToolButton *button, gpointer user_data);
void on_button_selector_new_clicked    (GtkToolButton *button, gpointer  user_data);
void on_button_selector_open_clicked   (GtkToolButton *button, gpointer  user_data);
void on_button_selector_delete_clicked (GtkToolButton *button, gpointer  user_data);
void on_button_selector_import_clicked (GtkToolButton *button, gpointer  user_data);
void on_button_selector_change_view_clicked (GtkToolButton *button, gpointer user_data);
void on_button_selector_preferences_clicked (GtkToolButton *button, gpointer _unused);

//--list
gboolean on_treeview_event(GtkWidget *treeview, GdkEvent *event, gpointer the_model);

gboolean on_treeview_selection_change(GtkTreeSelection *selection,
                                      GtkTreeModel *model,
                                      GtkTreePath *path,
                                      gboolean path_currently_selected,
                                      gpointer data);

void on_iconlist_item_activated(GtkIconView *iconview, GtkTreePath *treepath, gpointer user_data);
void on_iconlist_selection_changed(GtkIconView *iconview, gpointer user_data);
