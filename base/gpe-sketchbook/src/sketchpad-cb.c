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
#include <gdk/gdkkeysyms.h>

#include "sketchpad-cb.h"
#include "sketchpad-gui.h"
#include "sketchpad.h"

#include "gpe-sketchbook.h"
#include "files.h"
#include "selector.h"
#include "selector-cb.h"
#include "db.h"

#include "dock.h"
#include "gpe/question.h"
#include "gpe/popup_menu.h"
#include "gpe/pixmaps.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

//---------------------------------------------------------
//--------------------- GENERAL ---------------------------
static void sketchpad_exit(GtkWidget *top_level){
  if(is_current_sketch_modified){
    int ret;
    if(top_level) gtk_widget_show (top_level);

    ret = gpe_question_ask (_("Sketch modified,\nsave before exiting?"), _("Question"), "question", 
                            _("Discard"), "!gtk-no", _("Save"), "!gtk-yes", NULL);
    if(ret != 0){
      on_button_file_save_clicked(NULL, NULL);
    }
  }
  app_quit();
}

void on_window_sketchpad_destroy(GtkObject *object, gpointer unused){
  sketchpad_exit(GTK_WIDGET(object));
}

void on_window_size_allocate (GtkWidget     * widget,
                              GtkAllocation * allocation,
                              gpointer        dock){
  //**/g_printerr("Window size allocated: (%d,%d) (%d,%d)\n",
  //**/           allocation->x, allocation->y,
  //**/           allocation->width, allocation->height);

  if(dock == NULL) return;
  
  /* Set vertical on small screens in landscape mode.
     On large screens stay horizontal like in portrait mode to look
     more like a common PC application. */
    
  if ((allocation->width > allocation->height+20) && (allocation->width <= 640))
    gpe_dock_change_toolbar_orientation((GpeDock *)dock, GTK_ORIENTATION_VERTICAL);
  else//Portrait mode
    gpe_dock_change_toolbar_orientation((GpeDock *)dock, GTK_ORIENTATION_HORIZONTAL); 
}


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

void on_button_list_view_clicked(GtkToolButton *button, gpointer user_data){

  //close popup, in case it was around
  
  if (_save_current_if_needed() == ACTION_CANCELED) return;

  if(is_current_sketch_new){
    gboolean ret;
    g_signal_emit_by_name(G_OBJECT(selector.textlistview), "unselect-all", &ret);
  }
  else{
    GtkTreeSelection * selection;
    GtkTreePath * path;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(selector.textlistview));
    /**/g_printerr("Selecting %d\n", current_sketch);
    path = gtk_tree_path_new_from_indices(current_sketch, -1);
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_path_free(path);

    //set the current one selected
    set_current_sketch_selected();
  }
  switch_to_page(PAGE_SELECTOR);
}


gint on_key_press(GtkWidget *widget, GdkEventKey *ev, gpointer data){
  GtkScrolledWindow *sw = GTK_SCROLLED_WINDOW(data);
  gboolean isHoriz;
  GtkScrollType scroll;

  // ignore key events if not showing a drawing area
  if (gtk_notebook_get_current_page(sketchbook.notebook) != PAGE_SKETCHPAD
      || sketchbook.prefs.joypad_scroll == FALSE)
    return FALSE;

  switch (ev->keyval){
    case GDK_Left:
      scroll = GTK_SCROLL_STEP_BACKWARD;
      isHoriz = TRUE;
      break;
    case GDK_Right:
      scroll = GTK_SCROLL_STEP_FORWARD;
      isHoriz = TRUE;
      break;
    case GDK_Up:
      scroll = GTK_SCROLL_STEP_BACKWARD;
      isHoriz = FALSE;
      break;
    case GDK_Down:
      scroll = GTK_SCROLL_STEP_FORWARD;
      isHoriz = FALSE;
      break;
    default:
      return FALSE;  // not handled here
  }

  // Pass the event off to the scrollbar
  gtk_signal_emit_by_name(GTK_OBJECT(isHoriz ? sw->hscrollbar : sw->vscrollbar),
                          "move-slider", scroll);
  return TRUE;       // we handled it
}

//---------------------------------------------------------
//--------------------- FILE TOOLBAR ----------------------
void on_button_file_exit_clicked(GtkToolButton *button, gpointer unused){
  sketchpad_exit(NULL);
}

void on_button_file_save_clicked(GtkToolButton *button, gpointer unused){
  gchar     * filename  = NULL;
  gchar     * title     = NULL;
  GdkPixbuf * thumbnail = NULL;
  GObject   * iconlist_item = NULL;
  GtkTreeIter iter;
  GTimeVal current_time; //struct GTimeVal{  glong tv_sec;  glong tv_usec;};
  gint note_id = -1;

  if(!is_current_sketch_modified){
    return;
  }

  if(is_current_sketch_new){
    filename = file_new_fullpath_filename();
    title    = g_strdup(_("New sketch"));
  }
  else{
    //access the existing row data
    gchar * path_string;
    path_string = g_strdup_printf("%d", current_sketch);

    gtk_tree_model_get_iter_from_string( selector.listmodel, &iter, path_string);
    g_free(path_string);
    gtk_tree_model_get(selector.listmodel, &iter,
                       ENTRY_ID, &note_id,
                       ENTRY_URL, &filename,
                       ENTRY_THUMBNAIL, &thumbnail,
                       ENTRY_ICONLISTITEM, &iconlist_item,
                       -1);
  }

  file_save(filename); //FIXME: should catch saving errors
  
  if(selector.thumbnails_notloaded == FALSE){//--make thumbnail
    GdkPixbuf * pixbuf;
    GdkPixbuf * pixbuf_scaled;
    pixbuf = sketchpad_get_current_sketch_pixbuf();
    pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf,
                                             THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                             GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    if(thumbnail) g_object_unref(thumbnail);
    thumbnail = pixbuf_scaled;
  }

  g_get_current_time(&current_time);

  if(is_current_sketch_new){
    current_sketch = sketch_list_size;
    sketch_list_size++;

    //--Add to DB
    {
      Note note;
      
      note.type    = DB_NOTE_TYPE_SKETCH;
      note.title   = title;
      note.created = current_time.tv_sec;
      note.updated = current_time.tv_sec;
      note.url     = filename;  
      
      note_id = db_insert_note(&note);
    }

    selector_add_note(note_id, title, filename, current_time.tv_sec, current_time.tv_sec, thumbnail);
  }
  else{

    db_update_timestamp(note_id, current_time.tv_sec);

    //update "update" field
    {
      gchar * path_string;
      gchar *updated_label;
      path_string = g_strdup_printf("%d", current_sketch);
      
      /*gboolean*/gtk_tree_model_get_iter_from_string( selector.listmodel, &iter, path_string);
      g_free(path_string);

      updated_label = get_time_label(current_time.tv_sec);  
      gtk_list_store_set (GTK_LIST_STORE(selector.listmodel), &iter,
                          ENTRY_UPDATED,     updated_label,
                          ENTRY_UPDATED_VAL, current_time.tv_sec,
                          -1);
      g_free(updated_label);
    }

  }

  is_current_sketch_modified = FALSE;
  sketchpad_reset_title();
}

void on_button_file_new_clicked (GtkToolButton *button, gpointer user_data){
  if(_save_current_if_needed() == ACTION_CANCELED){
    return;
  }
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();
}

void on_button_file_import_clicked (GtkToolButton *button, gpointer user_data){
  if(_save_current_if_needed() != ACTION_CANCELED){
    sketchpad_import_image();
  }
}

void on_button_file_properties_clicked (GtkToolButton *button, gpointer user_data){
  //does nothing yet
}


void on_button_file_delete_clicked (GtkToolButton *button, gpointer user_data){
  int ret;

  if(is_current_sketch_new) return;
  ret = gpe_question_ask (_("Delete current sketch?"), _("Question"), "question", 
                          _("Cancel"), "!gtk-no", _("Delete"), "!gtk-yes", NULL);
  if(ret == 1) delete_current_sketch();

}


void on_button_file_prev_clicked (GtkToolButton *button, gpointer user_data){
  if(is_current_sketch_first || is_sketch_list_empty) return;
  if( _save_current_if_needed() == ACTION_CANCELED) return;
  if(is_current_sketch_new) current_sketch = SKETCH_LAST;
  else current_sketch--;
  open_indexed_sketch(current_sketch);
}


void on_button_file_next_clicked (GtkToolButton *button, gpointer user_data){
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
void on_radiobutton_tool_clicked (GtkToolButton *button, gpointer user_data){
  sketchpad_set_tool_s((gchar *) user_data);
}

void on_button_brushes_clicked(GtkToolButton * button, gpointer brushbox){
  gtk_widget_hide    (GTK_WIDGET (brushbox));
  gtk_widget_show_all(GTK_WIDGET (brushbox));
}

void on_button_colors_clicked(GtkToolButton * button, gpointer colorbox){
  gtk_widget_hide    (GTK_WIDGET (colorbox));
  gtk_widget_show_all(GTK_WIDGET (colorbox));
}


void on_radiobutton_brush_clicked (GtkToolButton *button, gpointer brush){
  GtkWidget * topbutton;
  GtkWidget * topbutton_image;
  GdkPixbuf * button_image;
  gchar *bname = g_strdup_printf("brush_%s",(gchar*)brush);

  sketchpad_set_brush_s((gchar *) brush);

  topbutton = g_object_get_data((GObject *) button, "parent");
  topbutton_image = g_object_get_data((GObject *) topbutton, "pixmap");

  button_image = gpe_find_icon (bname);
  g_free(bname);
  gtk_image_set_from_pixbuf(GTK_IMAGE(topbutton_image), button_image);
}


void on_radiobutton_color_clicked (GtkToolButton *button, gpointer color){
  GtkWidget * topbutton, * cbutton;

  current_color = (GdkColor *)color;

  topbutton = gtk_object_get_data((GtkObject *) button, "parent");
  cbutton = gtk_object_get_data((GtkObject *) topbutton, "cbutton");

  colorbox_button_set_color(GTK_WIDGET (cbutton), current_color);
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
