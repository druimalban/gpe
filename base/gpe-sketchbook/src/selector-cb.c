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

#include "selector.h"
#include "selector-cb.h"
#include "selector-gui.h"

#include "gpe-sketchbook.h"
#include "files.h"
#include "note.h"
#include "sketchpad.h"

#include "gpe/question.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

void switch_to_page(guint page){
  switch(page){
    case PAGE_SKETCHPAD:
      gtk_notebook_set_page(sketchbook.notebook, PAGE_SKETCHPAD);
      break;
    case PAGE_SELECTOR:
    default:
      gtk_notebook_set_page(sketchbook.notebook, PAGE_SELECTOR);
  }
}

void on_window_selector_destroy (GtkObject *object, gpointer user_data){
  app_quit();
}

void on_button_selector_new_clicked (GtkButton *button, gpointer user_data){
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();
  switch_to_page(PAGE_SKETCHPAD);
}

void on_button_selector_open_clicked (GtkButton *button, gpointer user_data){
  if(!is_current_sketch_selected || is_current_sketch_new) return;
  open_indexed_sketch(current_sketch);
  switch_to_page(PAGE_SKETCHPAD);
}

void on_button_selector_delete_clicked (GtkButton *button, gpointer user_data){
  int ret;
  if(!is_current_sketch_selected || is_current_sketch_new) return;
  //--ask confirmation (maybe a preference)
  ret = gpe_question_ask (_("Delete current sketch?"), _("Question"), "question", 
                          _("Cancel"), "!gtk-no", _("Delete"), "!gtk-yes", NULL);
  if(ret == 1) delete_current_sketch();

}


void _switch_icon(GtkButton * button){
  GtkWidget * old_icon;
  GtkWidget * new_icon;

  GtkWidget * vbox;//Button > VBox > ..., icon, ...
  vbox = gtk_bin_get_child(GTK_BIN (button));

  if(icons_mode){
    new_icon = gtk_object_get_data((GtkObject *) button, "list_mode_icon");
    old_icon = gtk_object_get_data((GtkObject *) button, "icon_mode_icon");
  }
  else{
    new_icon = gtk_object_get_data((GtkObject *) button, "icon_mode_icon");
    old_icon = gtk_object_get_data((GtkObject *) button, "list_mode_icon");
  }  
  g_object_ref(old_icon);
  gtk_container_remove (GTK_CONTAINER (vbox), old_icon );
  gtk_container_add    (GTK_CONTAINER (vbox), new_icon );
  gtk_widget_show_now(new_icon);
}

void on_button_selector_change_view_clicked (GtkButton *button, gpointer user_data){
  if(icons_mode){
    //**/g_printerr("switch to LIST view\n");
    gtk_widget_hide(selector.iconlist);
    gtk_widget_show(scrolledwindow_selector_clist);
    icons_mode = FALSE;
  }
  else {
    //**/g_printerr("switch to ICON view\n");
    gtk_widget_hide(scrolledwindow_selector_clist);
    gtk_widget_show(selector.iconlist);
    icons_mode = TRUE;
  }
  _switch_icon(button);
}



void on_clist_selector_select_row (GtkCList *clist, gint row, gint column,
                                   GdkEvent *event, gpointer user_data){
  if(event == NULL) return;//explicit call, user_func does nothing.
  current_sketch = row;
  set_current_sketch_selected();

  if(event->type == GDK_2BUTTON_PRESS){//--> double click = open related sketch
    Note * note;
    note = gtk_clist_get_row_data(clist, row);
    sketchpad_open_file(note->fullpath_filename);
    switch_to_page(PAGE_SKETCHPAD);
  }
}

void on_clist_selector_unselect_row (GtkCList *clist, gint row, gint column,
                                     GdkEvent *event, gpointer user_data){
  if(row == current_sketch) set_current_sketch_unselected();
}

void on_clist_selector_click_column (GtkCList *clist, gint column, gpointer user_data){
  //do nothing, as column is hidden!
}

void on_button_sketchpad_view_clicked (GtkButton *button, gpointer user_data){
  if(!is_current_sketch_selected) current_sketch = SKETCH_NEW;
  if(is_current_sketch_new) sketchpad_new_sketch();
  else open_indexed_sketch(current_sketch);
  switch_to_page(PAGE_SKETCHPAD);
}

