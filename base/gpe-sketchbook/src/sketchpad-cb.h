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
#ifndef SKETCHPAD_CB_H
#define SKETCHPAD_CB_H

#include <gtk/gtk.h>

/* general */
void on_window_sketchpad_destroy   (GtkObject *object, gpointer user_data);
void on_window_size_allocate       (GtkWidget *widget, GtkAllocation *a, gpointer data);
void on_button_list_view_clicked   (GtkButton *button, gpointer user_data);
gint on_key_press                  (GtkWidget *widget, GdkEventKey *ev, gpointer data);

/* files toolbar*/
void on_button_file_save_clicked   (GtkButton *button, gpointer user_data);
void on_button_file_new_clicked    (GtkButton *button, gpointer user_data);
void on_button_file_import_clicked (GtkButton *button, gpointer user_data);
void on_button_file_properties_clicked (GtkButton *button, gpointer user_data);
void on_button_file_delete_clicked (GtkButton *button, gpointer user_data);
void on_button_file_exit_clicked   (GtkButton *button, gpointer unused);

void on_button_file_prev_clicked   (GtkButton *button, gpointer user_data);
void on_button_file_next_clicked   (GtkButton *button, gpointer user_data);

/* Drawing toolbar */
void on_radiobutton_tool_clicked   (GtkButton * button, gpointer user_data);
void on_button_brushes_clicked     (GtkButton * button, gpointer brushbox);
void on_button_colors_clicked      (GtkButton * button, gpointer colorbox);
void on_radiobutton_brush_clicked  (GtkButton * button, gpointer user_data);
void on_radiobutton_color_clicked  (GtkButton * button, gpointer user_data);

/* Drawing Area */
gboolean on_drawing_area_configure_event     (GtkWidget *widget,
                                              GdkEventConfigure *event, gpointer user_data);
gboolean on_drawing_area_expose_event        (GtkWidget *widget,
                                              GdkEventExpose    *event, gpointer user_data);
gboolean on_drawing_area_motion_notify_event (GtkWidget *widget,
                                              GdkEventMotion    *event, gpointer user_data);
gboolean on_drawing_area_button_press_event  (GtkWidget *widget,
                                              GdkEventButton    *event, gpointer user_data);
gboolean on_drawing_area_button_release_event(GtkWidget *widget,
                                              GdkEventButton    *event, gpointer user_data);

#endif
