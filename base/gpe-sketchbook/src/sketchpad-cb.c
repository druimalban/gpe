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
#include "note.h"
#include "selector.h"
#include "selector-cb.h"

#include "dock.h"
#include "gpe/question.h"
#include "gpe/gpe-iconlist.h"
#include "gpe/popup_menu.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

//---------------------------------------------------------
//--------------------- GENERAL ---------------------------
void on_window_sketchpad_destroy(GtkObject *object, gpointer user_data){
  if(is_current_sketch_modified){
    int ret;
    gtk_widget_show (GTK_WIDGET(object));

    ret = gpe_question_ask (_("Sketch modified,\nsave before exiting?"), _("Question"), "question", 
                            _("Discard"), "!gtk-no", _("Save"), "!gtk-yes", NULL);
    if(ret != 0){
      on_button_file_save_clicked(NULL, NULL);
    }
  }
  app_quit();
}

void on_window_size_allocate (GtkWidget     * widget,
                              GtkAllocation * allocation,
                              gpointer        dock){
  //**/g_printerr("Window size allocated: (%d,%d) (%d,%d)\n",
  //**/           allocation->x, allocation->y,
  //**/           allocation->width, allocation->height);

  if(dock == NULL) return;

  if(allocation->width > 260)//240)//FIXME: that's very ugly way to detect LANDSCAPE mode!
    gpe_dock_change_toolbar_orientation((GpeDock *)dock, GTK_ORIENTATION_VERTICAL);
  else//Portrait mode
    gpe_dock_change_toolbar_orientation((GpeDock *)dock, GTK_ORIENTATION_HORIZONTAL); 
}


enum {ACTION_CANCELED, ACTION_DONE};

int _save_current_if_needed(){
  if(is_current_sketch_modified){
    int ret;
    ret = gpe_question_ask (_("Save current sketch?"), _("Question"), "question", 
                            _("Discard"), "!gtk-no", _("Save"), "!gtk-yes", NULL);
    //NOTE: could had a [Cancel] button
    if(ret == 1){
      on_button_file_save_clicked(NULL, NULL);
    }
    else if (ret == -1) return ACTION_CANCELED; //destroy window (ret == -1) cancel the action
  }
  return ACTION_DONE;
}

void on_button_list_view_clicked(GtkButton *button, gpointer user_data){

  //close popup, in case it was around
  popup_menu_close (sketchpad.files_popup_button);
  
  if (_save_current_if_needed() == ACTION_CANCELED) return;

  if(is_current_sketch_new){
    gtk_clist_unselect_all(selector_clist);
    GTK_CLIST(selector_clist)->focus_row = -1;
    gtk_widget_set_sensitive(selector.button_edit,   FALSE);
    gtk_widget_set_sensitive(selector.button_delete, FALSE);
  }
  else{
    //set the current one selected
    set_current_sketch_selected();
    gtk_clist_select_row(selector_clist, current_sketch, 1);
    GTK_CLIST(selector_clist)->focus_row = current_sketch;
    gtk_signal_emit_by_name((GtkObject *)selector_clist, "select-row",
                            current_sketch, 1,
                            NULL, NULL);//highlight the selected item
    gtk_widget_set_sensitive(selector.button_edit,   TRUE);
    gtk_widget_set_sensitive(selector.button_delete, TRUE);
  }
  switch_to_page(PAGE_SELECTOR);
}

//---------------------------------------------------------
//--------------------- FILE TOOLBAR ----------------------

void on_button_file_save_clicked(GtkButton *button, gpointer unused){
  Note * note;

  if(!is_current_sketch_modified){
    popup_menu_close (sketchpad.files_popup_button);
    return;
  }
  if(is_current_sketch_new){
    note = note_new();
    note->fullpath_filename = file_new_fullpath_filename();
  }
  else{
    note =  gtk_clist_get_row_data(selector_clist, current_sketch);
  }

  file_save(note->fullpath_filename); //FIXME: should catch saving errors
  {//--update thumbnail
    GdkPixbuf * pixbuf;
    GdkPixbuf * pixbuf_scaled;
    pixbuf = sketchpad_get_current_sketch_pixbuf();
    pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf,
                                             THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                             GDK_INTERP_BILINEAR);
    gdk_pixbuf_unref(pixbuf);
    if(note->thumbnail) gdk_pixbuf_unref(note->thumbnail);
    note->thumbnail = pixbuf_scaled;
  }

  if(is_current_sketch_new){
    gchar * name[1];

    sketch_list_size++;

    name[0] = make_label_from_filename(note->fullpath_filename);
    current_sketch = gtk_clist_append(selector_clist, name);
    g_free(name[0]);
    gtk_clist_set_row_data_full(selector_clist, current_sketch, note, note_destroy);
    if(current_sketch%2) gtk_clist_set_background(selector_clist, current_sketch, &bg_color);

    gpe_iconlist_add_item_pixbuf (GPE_ICONLIST(selector.iconlist),
                                  NULL,// "Sketch"
                                  note->thumbnail,
                                  note);
  }
  else{
    //update icon_view
    gpe_iconlist_update_icon_item_with_udata (GPE_ICONLIST(selector.iconlist),
                                              note->thumbnail, note);
  }
  is_current_sketch_modified = FALSE;
  sketchpad_reset_title();

  //in case of direct call, button == NULL, ie: from on_button_file_new_clicked()
  if(button) popup_menu_close (sketchpad.files_popup_button);
}

void on_button_file_new_clicked (GtkButton *button, gpointer user_data){
  if(_save_current_if_needed() == ACTION_CANCELED){
    popup_menu_close (sketchpad.files_popup_button);
    return;
  }
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();

  popup_menu_close (sketchpad.files_popup_button);
}

void on_button_file_import_clicked (GtkButton *button, gpointer user_data){
  if(_save_current_if_needed() != ACTION_CANCELED){
    sketchpad_import_image();
  }
  popup_menu_close (sketchpad.files_popup_button);
}

void on_button_file_properties_clicked (GtkButton *button, gpointer user_data){
  //does nothing yet
  popup_menu_close (sketchpad.files_popup_button);
}


void on_button_file_delete_clicked (GtkButton *button, gpointer user_data){
  int ret;

  //first close the popup, to avoid overlaping with the message box
  popup_menu_close (sketchpad.files_popup_button);

  if(is_current_sketch_new) return;
  ret = gpe_question_ask (_("Delete current sketch?"), _("Question"), "question", 
                          _("Cancel"), "!gtk-no", _("Delete"), "!gtk-yes", NULL);
  if(ret == 1) delete_current_sketch();

}


void on_button_file_prev_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_first || is_sketch_list_empty) return;
  if( _save_current_if_needed() == ACTION_CANCELED) return;
  if(is_current_sketch_new) current_sketch = SKETCH_LAST;
  else current_sketch--;
  open_indexed_sketch(current_sketch);
}


void on_button_file_next_clicked (GtkButton *button, gpointer user_data){
  if(is_current_sketch_new && !is_current_sketch_modified) return;
  if( _save_current_if_needed() == ACTION_CANCELED) return;
  if(is_current_sketch_last || is_current_sketch_new){
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
  GtkWidget * topbutton_image;
  GdkPixbuf * button_image;

  sketchpad_set_brush_s((gchar *) brush);

  brushbox  = gtk_widget_get_toplevel((GtkWidget *)button);
  topbutton = gtk_object_get_data((GtkObject *) brushbox, "calling_button");

  topbutton_image = gtk_object_get_data((GtkObject *) topbutton, "pixmap");
  button_image = gtk_image_get_pixbuf (GTK_IMAGE(GTK_BIN (button)->child));
  gtk_image_set_from_pixbuf(GTK_IMAGE(topbutton_image), button_image);

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
    reset_drawing_area();
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

