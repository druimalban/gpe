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
#include "sketchpad-gui.h"
#include "sketchpad.h"

#include "gpe-sketchbook.h"
#include "files.h"
#include "selector.h"
#include "selector-cb.h"

#include "dialog.h"

//---------------------------------------------------------
//--------------------- GENERAL ---------------------------
void on_window_sketchpad_destroy(GtkObject *object, gpointer user_data){
  app_quit();
}
//FIXME: maybe connect "delete_event"

void on_button_list_view_clicked(GtkButton *button, gpointer user_data){
  switch_windows(window_sketchpad, window_selector);
}

//---------------------------------------------------------
//--------------------- FILE TOOLBAR ----------------------
void on_button_file_save_clicked(GtkButton *button, gpointer user_data){
  gchar * fullpath_filename;

  //if(is_current_sketch_unmodified) return; FIXME: feature to add!

  if(is_current_sketch_new) fullpath_filename = file_new_fullpath_filename();
  else fullpath_filename = (gchar *) gtk_clist_get_row_data(selector_clist, current_sketch);

  file_save_xpm(drawing_area, fullpath_filename); //FIXME: should catch saving errors
  if(is_current_sketch_new){
    gchar * name[1];

    sketch_list_size++;

    name[0] = make_label_from_filename(fullpath_filename);
    current_sketch = gtk_clist_append(selector_clist, name);
    sketchpad_set_title(name[0]);
    g_free(name[0]);

    gtk_clist_select_row(selector_clist, current_sketch, 1);
    gtk_clist_set_row_data_full(selector_clist, current_sketch, fullpath_filename, g_free);
    if(current_sketch%2) gtk_clist_set_background(selector_clist, current_sketch, &bg_color);
  }
}


void on_button_file_new_clicked (GtkButton *button, gpointer user_data){
  //FIXME: maybe save the previous one if unsaved
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();
}


void on_button_file_delete_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_new) return;
  //--ask confirmation (maybe a preference)
  if(confirm_action_dialog_box("Delete sketch?","Delete")){
    delete_current_sketch();
  }
}


void on_button_file_prev_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_first || is_sketch_list_empty) return;
  if(is_current_sketch_new) current_sketch = SKETCH_LAST;
  else current_sketch--;
  open_indexed_sketch(current_sketch);
  //FIXME: the following selects the row BUT does NOT turn it in darkblue
  gtk_clist_select_row(selector_clist, current_sketch, 1);
}


void on_button_file_next_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_new) return;
  if(is_current_sketch_last){
    current_sketch = SKETCH_NEW;
    sketchpad_new_sketch();
    return;
  }
  //else
  current_sketch++;
  open_indexed_sketch(current_sketch);
  gtk_clist_select_row(selector_clist, current_sketch, 1);
}

//---------------------------------------------------------
//--------------------- DRAWING TOOLBAR -------------------
void on_radiobutton_tool_clicked (GtkButton *button, gpointer user_data){
  //**/g_printerr("selected: %s\n", (gchar *) user_data);
  sketchpad_set_tool_s((gchar *) user_data);
}


void on_radiobutton_brush_clicked (GtkButton *button, gpointer user_data){
  //**/g_printerr("selected: %s\n", (gchar *) user_data);
  sketchpad_set_brush_s((gchar *) user_data);
}


void on_radiobutton_color_clicked (GtkButton *button, gpointer user_data){
  //**/g_printerr("selected: %s\n", (gchar *) user_data);
  sketchpad_set_color_s((gchar *) user_data);
  //NOTE: if(tool == ERASER) sketchpad_set_tool_s("pen"); (need toggle buttons)
}

//---------------------------------------------------------
//--------------------- DRAWING AREA ----------------------

gboolean
on_drawing_area_configure_event (GtkWidget        *widget,
                                 GdkEventConfigure *event,
                                 gpointer          user_data){
  configure_event_handler(widget, event);
  return FALSE;
}


gboolean
on_drawing_area_expose_event (GtkWidget       *widget,
                              GdkEventExpose  *event,
                              gpointer         user_data){
  expose_event_handler(widget, event);
  return FALSE;
}


gboolean
on_drawing_area_motion_notify_event(GtkWidget       *widget,
                                    GdkEventMotion  *event,
                                    gpointer         user_data){
  motion_notify_event_handler(widget, event);
  return FALSE;
}


gboolean
on_drawing_area_button_press_event(GtkWidget       *widget,
                                   GdkEventButton  *event,
                                   gpointer         user_data){
  button_press_event_handler(widget, event);
  return FALSE;
}


gboolean
on_drawing_area_button_release_event(GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     gpointer         user_data){
  button_release_event_handler(widget, event);
  return FALSE;
}

