/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002, 2003, 2004, 2005 Luc Pionchon
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
#include "sketchpad.h"

#include "gpe/question.h"
#include "gpe/popup_menu.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

void switch_to_page(guint page){
  gtk_notebook_set_page(sketchbook.notebook, page);
}

void on_window_selector_destroy (GtkObject *object, gpointer user_data){
  app_quit();
}

void on_button_selector_exit_clicked (GtkToolButton *button, gpointer user_data){
  app_quit();
}

void on_button_selector_new_clicked (GtkToolButton *button, gpointer user_data){
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();
  switch_to_page(PAGE_SKETCHPAD);
}

void on_button_selector_open_clicked (GtkToolButton *button, gpointer user_data){
  if(!is_current_sketch_selected || is_current_sketch_new) return;
  open_indexed_sketch(current_sketch);
  switch_to_page(PAGE_SKETCHPAD);
}

void on_button_selector_delete_clicked (GtkToolButton *button, gpointer user_data){
  int ret;

  if(!is_current_sketch_selected || is_current_sketch_new) return;

  //--ask confirmation (maybe a preference)
  ret = gpe_question_ask (_("Delete current sketch?"), _("Question"), "question", 
                          _("Cancel"), "!gtk-no", _("Delete"), "!gtk-yes", NULL);
  if(ret == 1) delete_current_sketch();

}

void on_button_selector_import_clicked (GtkToolButton *button, gpointer user_data){
  sketchpad_import_image();
}


void _switch_icon(GtkToolButton * button){
  GtkWidget * old_icon;
  GtkWidget * new_icon;

  if(icons_mode){
    new_icon = g_object_get_data(G_OBJECT(button), "list_mode_icon");
    old_icon = g_object_get_data(G_OBJECT(button), "icon_mode_icon");
  }
  else{
    new_icon = g_object_get_data(G_OBJECT(button), "icon_mode_icon");
    old_icon = g_object_get_data(G_OBJECT(button), "list_mode_icon");
  }  
  gtk_tool_button_set_icon_widget(button, new_icon);
  gtk_widget_show(new_icon);
}

void on_button_selector_change_view_clicked (GtkToolButton *button, gpointer user_data){
  if(icons_mode){//--> switch to LIST view
    gtk_widget_hide(selector.iconlist);
    gtk_widget_show(selector.textlist);
    icons_mode = FALSE;
  }
  else {//--> switch to ICON view
    gtk_widget_hide(selector.textlist);
    gtk_widget_show(selector.iconlist);
    icons_mode = TRUE;
    if(selector.thumbnails_notloaded) load_thumbnails();
  }
  if(button) _switch_icon(button);
}

gboolean on_treeview_selection_change(GtkTreeSelection *selection,
                                      GtkTreeModel *model,
                                      GtkTreePath *path,
                                      gboolean path_currently_selected,
                                      gpointer data){
  // Used by gtk_tree_selection_set_select_function().
  // Called whenever a row's state might change.
  // Returns TRUE to indicates to selection that it is okay to change the selection.

  if(!path_currently_selected){
    gint * indices;
    indices = gtk_tree_path_get_indices(path);

    //FIXME: selection is done HERE - remove duplicated and dead code.

    //FIXME: update selection on IconView

    current_sketch = indices[0];
    gtk_widget_set_sensitive(selector.button_edit,   TRUE);
    gtk_widget_set_sensitive(selector.button_delete, TRUE);
    set_current_sketch_selected();
    return TRUE;
  }
  else {
    gtk_widget_set_sensitive(selector.button_edit,   FALSE);
    gtk_widget_set_sensitive(selector.button_delete, FALSE);
  }

  return TRUE;
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
        g_free(fullpath_filename);
        switch_to_page(PAGE_SKETCHPAD);
        return TRUE;
      }
    default: return FALSE;//FALSE to propagate the event further
  }
}

void on_iconlist_selection_changed(GtkIconView *iconview, gpointer user_data){
  GList * selected_item_list = NULL;

  //**/g_printerr("Icon list Selection changed.\n");

  selected_item_list = gtk_icon_view_get_selected_items(iconview);

  //**/g_printerr("selection: %d items\n", g_list_length(selected_item_list));

  if(selected_item_list && g_list_length(selected_item_list) == 1)
  {
    GtkTreePath * tree_path;
    gint * indices;

    tree_path = (GtkTreePath *) g_list_nth_data(selected_item_list, 0);

    indices = gtk_tree_path_get_indices(tree_path);
    current_sketch = indices[0];
    set_current_sketch_selected();
  }

  //FIXME: update selection on TreeView

  g_list_foreach (selected_item_list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free    (selected_item_list);
}


void on_iconlist_item_activated(GtkIconView *iconview, GtkTreePath *tree_path, gpointer user_data){
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar * fullpath_filename;
  gint * indices;

  /**/g_printerr("Icon list item activated.\n");
  
  model = selector.listmodel;

  gtk_tree_model_get_iter (model, &iter, tree_path);
  gtk_tree_model_get (model, &iter,
                      ENTRY_URL, &fullpath_filename,
                      -1);

  indices = gtk_tree_path_get_indices(tree_path);
  current_sketch = indices[0];
  set_current_sketch_selected();

  sketchpad_open_file(fullpath_filename);
  g_free(fullpath_filename);
  switch_to_page(PAGE_SKETCHPAD);
}


void on_button_selector_preferences_clicked (GtkToolButton *button, gpointer _unused){
  switch_to_page(PAGE_PREFERENCES);
}
