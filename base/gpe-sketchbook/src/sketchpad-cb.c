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
#include <gdk/gdkkeysyms.h> //up down right left keys

#include "sketchpad-cb.h"
#include "sketchpad-gui.h"
#include "sketchpad.h"

#include "gpe-sketchbook.h"
#include "files.h"
#include "note.h"
#include "selector.h"
#include "selector-cb.h"
#include "selector-gui.h"//build_thumbnail_widget

#include "dock.h"
#include "dialog.h"
#include "gpe/question.h"
#include "gpe/gtkgpepixmap.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

//---------------------------------------------------------
//--------------------- GENERAL ---------------------------
void on_window_sketchpad_destroy(GtkObject *object, gpointer user_data){
  if(is_current_sketch_modified){
    gtk_widget_show (GTK_WIDGET(object));
    //FIXME: use gpe_question
    if(confirm_action_dialog_box(_("Sketch modified;\nsave before exiting?"),
                                 _("Exit"),_("Save"))){
      on_button_file_save_clicked(NULL,NULL);
    }
  }
  app_quit();
}

void on_window_size_allocate (GtkWidget     * widget,
                              GtkAllocation * allocation,
                              gpointer        dock){
  ///**/g_printerr("Window size allocated: (%d,%d) (%d,%d)\n",
  ///**/           allocation->x, allocation->y,
  ///**/           allocation->width, allocation->height);

  if(dock == NULL) return;

  if(allocation->width > 260)//240)//FIXME: that's very ugly way to detect LANDSCAPE mode!
    gpe_dock_change_toolbar_orientation((GpeDock *)dock, GTK_ORIENTATION_VERTICAL);
  else//Portrait mode
    gpe_dock_change_toolbar_orientation((GpeDock *)dock, GTK_ORIENTATION_HORIZONTAL); 
}

void on_button_list_view_clicked(GtkButton *button, gpointer user_data){
  if(is_current_sketch_modified){
    //FIXME: use gpe_question
    if(confirm_action_dialog_box(_("Save last sketch?"),_("Cancel"),_("Save"))){
      on_button_file_save_clicked(NULL,NULL);
    }
  }
  switch_windows(window_sketchpad, window_selector);

  if(is_current_sketch_new){
    gtk_clist_unselect_all(selector_clist);
    GTK_CLIST(selector_clist)->focus_row = -1;
  }
  else{
    //set the current one selected
    set_current_sketch_selected();
    gtk_clist_select_row(selector_clist, current_sketch, 1);
    GTK_CLIST(selector_clist)->focus_row = current_sketch;
    gtk_signal_emit_by_name((GtkObject *)selector_clist, "select-row",
                            current_sketch, 1,
                            NULL, NULL);//highlight the selected item
  }
  gtk_widget_show_all(selector_icons_table);
}

//---------------------------------------------------------
//--------------------- FILE TOOLBAR ----------------------
void on_button_file_save_clicked(GtkButton *button, gpointer user_data){
  Note * note;

  if(!is_current_sketch_modified) return;

  if(is_current_sketch_new){
    note = note_new();
    note->fullpath_filename = file_new_fullpath_filename();
  }
  else{
    note =  gtk_clist_get_row_data(selector_clist, current_sketch);
  }

  file_save(note->fullpath_filename); //FIXME: should catch saving errors
  build_thumbnail_widget(note, window_selector->style);//FIXME: use buffer, do NOT read from file!
  if(is_current_sketch_new){
    gchar * name[1];

    sketch_list_size++;

    name[0] = make_label_from_filename(note->fullpath_filename);
    current_sketch = gtk_clist_append(selector_clist, name);
    sketchpad_set_title(name[0]);
    g_free(name[0]);
    gtk_clist_set_row_data_full(selector_clist, current_sketch, note, note_destroy);
    if(current_sketch%2) gtk_clist_set_background(selector_clist, current_sketch, &bg_color);

    selector_pack_icons(selector_icons_table);
  }
  else{
    //update icon_view
    selector_repack_icon(GTK_TABLE(selector_icons_table), note);

  }
  is_current_sketch_modified = FALSE;
}

void on_button_file_new_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_modified){
    //FIXME: use gpe_question
    if(confirm_action_dialog_box(_("Save last sketch?"),_("Cancel"),_("Save"))){
      on_button_file_save_clicked(NULL,NULL);
    }
  }
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();
}


void on_button_file_delete_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_new) return;
  //NOTE: moved back to my own dialog-box, gpe_question freezes.
  //if(gpe_question_ask_yn ("Delete sketch?") == 1){
  if(confirm_action_dialog_box(_("Delete sketch?"),_("Cancel"),_("Delete"))){
    delete_current_sketch();  
  }
}


void on_button_file_prev_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_first || is_sketch_list_empty) return;
  if(is_current_sketch_new) current_sketch = SKETCH_LAST;
  else current_sketch--;
  open_indexed_sketch(current_sketch);
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
}

//---------------------------------------------------------
//--------------------- DRAWING TOOLBAR -------------------
void on_radiobutton_tool_clicked (GtkButton *button, gpointer user_data){
  sketchpad_set_tool_s((gchar *) user_data);
}

void on_button_brushes_clicked(GtkButton * button, gpointer brushbox){
  gtk_widget_hide    (GTK_WIDGET (brushbox));
  gtk_widget_show_all(GTK_WIDGET (brushbox));
}

void on_button_colors_clicked(GtkButton * button, gpointer colorbox){
  gtk_widget_hide    (GTK_WIDGET (colorbox));
  gtk_widget_show_all(GTK_WIDGET (colorbox));
}


void on_radiobutton_brush_clicked (GtkButton *button, gpointer brush){
  GtkWidget * brushbox;
  GtkWidget * topbutton;
  GtkGpePixmap * topbutton_pixmap;
#ifndef GTK2 //GTK1
  GdkPixmap * button_image;
  GdkBitmap * button_mask;
#endif

  sketchpad_set_brush_s((gchar *) brush);

  brushbox  = gtk_widget_get_toplevel((GtkWidget *)button);
  topbutton = gtk_object_get_data((GtkObject *) brushbox, "calling_button");
  topbutton_pixmap = gtk_object_get_data((GtkObject *) topbutton, "pixmap");
#ifdef GTK2
  //FIXME: need to switch icon
#else
  gtk_gpe_pixmap_get(GTK_GPE_PIXMAP(GTK_BIN (button)->child), &button_image, &button_mask);
  gtk_gpe_pixmap_set(topbutton_pixmap, button_image, button_mask);
#endif

  gtk_widget_hide(GTK_WIDGET (brushbox));
}


void on_radiobutton_color_clicked (GtkButton *button, gpointer color){
  GtkWidget * colorbox;
  GtkWidget * topbutton;

  current_color = (GdkColor *)color;

  colorbox  = gtk_widget_get_toplevel((GtkWidget *)button);
  topbutton = gtk_object_get_data((GtkObject *) colorbox, "calling_button");
  colorbox_button_set_color(GTK_WIDGET (topbutton), current_color);
  //NOTE: if(tool == ERASER) sketchpad_set_tool_s("pencil"); (need toggle buttons)

  gtk_widget_hide(GTK_WIDGET (colorbox));
}

//---------------------------------------------------------
//--------------------- DRAWING AREA ----------------------

gboolean
on_drawing_area_configure_event (GtkWidget        * widget,
                                 GdkEventConfigure *event,
                                 gpointer          user_data){
  //when window is created or resized
  if(!drawing_area_pixmap_buffer){
    reset_drawing_area(widget);
  }
  return FALSE;
}


gboolean
on_drawing_area_expose_event (GtkWidget       *widget,
                              GdkEventExpose  *event,
                              gpointer         user_data){
  //refresh the part outdated by the event
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  drawing_area_pixmap_buffer,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);
  return FALSE;
}


gboolean
on_drawing_area_motion_notify_event(GtkWidget       *widget,
                                    GdkEventMotion  *event,
                                    gpointer         user_data){
  int x, y;
  GdkModifierType state;

  if (event->is_hint){
    gdk_window_get_pointer (event->window, &x, &y, &state);
  }
  else{
    x = event->x;
    y = event->y;
    state = event->state;
  }    
  if (state & GDK_BUTTON1_MASK){// && drawing_area_pixmap_buffer != NULL){
    if(prev_pos_x != NO_PREV_POS){
      draw_line(prev_pos_x, prev_pos_y, x, y);
    }
    prev_pos_x = x;
    prev_pos_y = y;
  }
  return FALSE;
}


gboolean
on_drawing_area_button_press_event(GtkWidget       *widget,
                                   GdkEventButton  *event,
                                   gpointer         user_data){
  if (event->button == 1 && drawing_area_pixmap_buffer != NULL){
    draw_point (event->x, event->y);
    prev_pos_x = event->x;
    prev_pos_y = event->y;
  }
  return FALSE;
}


gboolean
on_drawing_area_button_release_event(GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     gpointer         user_data){
  prev_pos_x = prev_pos_y = NO_PREV_POS;
  return FALSE;
}

gboolean on_drawing_area_key_press_event (GtkWidget   * widget,
                                          GdkEventKey * event,
                                          gpointer      scrolledwindow){

  /*
   *    |<-----[ page_size ]------------>|
   *   lower   |                        upper
   *           `value
   */

  GtkAdjustment * adj;
  GtkOrientation orientation;
  gdouble new_value;

  /**/if (event->type != GDK_KEY_PRESS) return FALSE;

  switch(event->keyval){
    case GDK_Left:  case GDK_KP_4: 
    case GDK_Right: case GDK_KP_6:
      orientation = GTK_ORIENTATION_HORIZONTAL;
      break;
    case GDK_Up:    case GDK_KP_8:
    case GDK_Down:  case GDK_KP_2:
      orientation = GTK_ORIENTATION_VERTICAL;
      break;

#ifdef DESKTOP
    case GDK_q:
      gtk_main_quit();
#endif

    default:
      return FALSE;
  }

  if(orientation == GTK_ORIENTATION_VERTICAL){
    adj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow *) scrolledwindow);
  }
  else{//GTK_ORIENTATION_HORIZONTAL
    adj = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow *) scrolledwindow);
  }
  
  switch(event->keyval){
    case GDK_Left:  case GDK_KP_4:
    case GDK_Up:    case GDK_KP_8:
      new_value = adj->value - adj->step_increment;
      if(new_value < adj->lower) new_value = adj->lower;
      gtk_adjustment_set_value(adj, new_value);
      break;
    case GDK_Right: case GDK_KP_6:
    case GDK_Down:  case GDK_KP_2:
      new_value = adj->value + adj->step_increment;
      if( (new_value + adj->page_size) > adj->upper) new_value = adj->upper - adj->page_size;
      gtk_adjustment_set_value(adj, new_value);
      break;
  }

  return TRUE;//TRUE avoid to change the focus when using arrow keys
}
