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

#include "gpe/about.h"
#include "gpe/question.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

void switch_windows(GtkWidget * window_to_hide, GtkWidget * window_to_show){
  gtk_widget_hide (window_to_hide);
  gtk_widget_show (window_to_show);
}


void on_window_selector_destroy (GtkObject *object, gpointer user_data){
  app_quit();
}

void on_button_selector_new_clicked (GtkButton *button, gpointer user_data){
  //FIXME: maybe save the previous one if unsaved
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();
  switch_windows(window_selector, window_sketchpad);
}

void on_button_selector_open_clicked (GtkButton *button, gpointer user_data){
  if(!is_current_sketch_selected || is_current_sketch_new) return;
  open_indexed_sketch(current_sketch);
  switch_windows(window_selector, window_sketchpad);
}

void on_button_selector_delete_clicked (GtkButton *button, gpointer user_data){
  int ret;
  if(!is_current_sketch_selected || is_current_sketch_new) return;
  //--ask confirmation (maybe a preference)
  ret = gpe_question_ask (_("Delete current sketch?"), _("Question"), "question", 
                          _("Cancel"), "!gtk-no", _("Delete"), "!gtk-yes", NULL);
  if(ret == 1) delete_current_sketch();

}

void on_button_selector_change_view_clicked (GtkButton *button, gpointer user_data){
  if(icons_mode){
    //**/g_printerr("switch to LIST view\n");
    gtk_widget_hide(scrolledwindow_selector_icons);
    gtk_widget_show(scrolledwindow_selector_clist);
    icons_mode = FALSE;
  }
  else {
    //**/g_printerr("switch to ICON view\n");
    gtk_widget_hide(scrolledwindow_selector_clist);
    gtk_widget_show(scrolledwindow_selector_icons);
    icons_mode = TRUE;
  }
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
    switch_windows(window_selector, window_sketchpad);
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
  switch_windows(window_selector, window_sketchpad);
}

void on_button_selector_about_clicked (GtkButton *button, gpointer user_data){
#ifdef NOT_DEF
  gpe_about(PACKAGE,
            VERSION,
            //icon
            "this_app_icon",
            //short description
            _("a notebook to sketch your notes"),
            //minihelp
            _("The application provides two main windows: "
            "the Selector, and the Sketchpad.\n"
            "You select your sketchs with the Selector.\n"
            "You sketch your notes with the Sketchpad."),
            //legal
            _("(c) 2002 Luc Pionchon\n" //FIXME: use build variables
            "Distributed under GPL"));
#endif
}

