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
    case PAGE_SELECTOR:
      if(icons_mode){//FIXME: to remove if iconlist can select
        gtk_widget_set_sensitive(selector.button_edit,   FALSE);//FIXME: to remove
        gtk_widget_set_sensitive(selector.button_delete, FALSE);//FIXME: to remove
      }//FIXME: to remove
  }
  gtk_notebook_set_page(sketchbook.notebook, page);
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

void on_button_selector_import_clicked (GtkButton *button, gpointer user_data){
  sketchpad_import_image();
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
  if(icons_mode){//--> switch to LIST view
    gtk_widget_hide(selector.iconlist);
    gtk_widget_show(selector.textlist);
    icons_mode = FALSE;
    if(is_current_sketch_selected){//FIXME: remove if iconlist can select
      gtk_widget_set_sensitive(selector.button_edit,   TRUE);//FIXME: remove
      gtk_widget_set_sensitive(selector.button_delete, TRUE);//FIXME: remove
    }//FIXME: remove if iconlist can select
  }
  else {//--> switch to ICON view
    gtk_widget_hide(selector.textlist);
    gtk_widget_show(selector.iconlist);
    icons_mode = TRUE;
    gtk_widget_set_sensitive(selector.button_edit,   FALSE);//FIXME: remove if iconlist can select
    gtk_widget_set_sensitive(selector.button_delete, FALSE);//FIXME: remove if iconlist can select
  }
  if(button) _switch_icon(button);
}

gboolean on_treeview_event(GtkWidget *treeview, GdkEvent *event, gpointer the_model){
  switch(event->type){
    case GDK_2BUTTON_PRESS://double click --> activate the item
      {
        GtkTreeSelection * selection;
        gboolean selected;
        GtkTreeModel * model;
        GtkTreeIter iter;
        gchar * fullpath_filename;
        
        model = GTK_TREE_MODEL(the_model);
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
        selected = gtk_tree_selection_get_selected (selection, &model, &iter);        
        if(!selected) return TRUE;

        //--Open the selected sketch
        gtk_tree_model_get(model, &iter,
                           ENTRY_URL, &fullpath_filename, -1);
        sketchpad_open_file(fullpath_filename);//NOTE: keep an index and open_indexed()
        switch_to_page(PAGE_SKETCHPAD);
        return TRUE;
      }
    case GDK_BUTTON_PRESS:



      //FIXME: cannot use the index of the CList.
      current_sketch = 0;



      set_current_sketch_selected();
      gtk_widget_set_sensitive(selector.button_edit,   TRUE);
      gtk_widget_set_sensitive(selector.button_delete, TRUE);
      return FALSE;

    default: return FALSE;//FALSE to propagate the event further
  }
}

void on_button_selector_preferences_clicked (GtkButton *button, gpointer _unused){
  switch_to_page(PAGE_PREFERENCES);
}
