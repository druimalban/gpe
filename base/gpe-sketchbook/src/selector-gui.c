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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>

#include "selector-gui.h"
#include "selector-cb.h"
#include "selector.h"
#include "db.h"

//gpe libs
#include "gpe/pixmaps.h"
#include "gpe/render.h"
#include "gpe/popup_menu.h"
#include "gpe/picturebutton.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

gboolean icons_mode;

GtkWidget * build_selector_toolbar();
GtkWidget * build_scrollable_textlist();
GtkWidget * build_scrollable_iconlist();

GtkWidget * create_window_selector(){
  GtkWidget *window_selector;
  GtkWidget * selector;

  window_selector = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window_selector), 240, 280);
  gtk_window_set_position     (GTK_WINDOW (window_selector), GTK_WIN_POS_CENTER);
#endif
  gtk_signal_connect (GTK_OBJECT (window_selector), "destroy",
                      GTK_SIGNAL_FUNC (on_window_selector_destroy), NULL);

  selector = selector_gui();
  gtk_container_add (GTK_CONTAINER (window_selector), selector);

  return window_selector;
}

GtkWidget * selector_gui(){
  GtkWidget *vbox; //hbox + list_view + (alt) icon_view
  GtkWidget *hbox; //toolbar + help button

  //--Toolbar
  hbox = build_selector_toolbar();

  //-- text/icon lists
  selector.textlist = build_scrollable_textlist();
  selector.iconlist = build_scrollable_iconlist();

  //--packing
  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox,              FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), selector.textlist, TRUE,  TRUE,  0);
  gtk_box_pack_start (GTK_BOX (vbox), selector.iconlist, TRUE,  TRUE,  0);

  //--show all except top level window AND one view
  gtk_widget_show(vbox);//top box
  gtk_widget_show_all(hbox);//toolbar

  //icons_mode = FALSE;
  //if(icons_mode) gtk_widget_show_all(selector.iconlist);
  //else           gtk_widget_show_all(selector.textlist);
  gtk_widget_show_all(selector.textlist);
  gtk_widget_show_all(selector.iconlist);

  return vbox;
}

GtkWidget * _selector_popup_new (GtkWidget *parent_button){
  GtkWidget *vbox;//to return

  GtkWidget * button_new;
  GtkWidget * button_open;
  GtkWidget * button_import;
  GtkWidget * button_delete;
  //GtkWidget * button_properties;
  GtkWidget * button_exit;

  GtkStyle * style ;//FIXME !!! = sketchpad.window->style;

  button_new        = gpe_picture_button_aligned (style, _("New"),    "new",    GPE_POS_LEFT);
  button_open       = gpe_picture_button_aligned (style, _("Edit"),   "edit",   GPE_POS_LEFT);
  button_import     = gpe_picture_button_aligned (style, _("Import"), "import", GPE_POS_LEFT);
  button_delete     = gpe_picture_button_aligned (style, _("Delete"), "delete", GPE_POS_LEFT);
  //button_properties = gpe_picture_button_aligned (style, _("Properties"), "properties", GPE_POS_LEFT);
  button_exit       = gpe_picture_button_aligned (style, _("Exit"),   "exit",   GPE_POS_LEFT);

#define _BUTTON_SETUP(action) \
              gtk_button_set_relief (GTK_BUTTON (button_ ##action), GTK_RELIEF_NONE);\
              g_signal_connect (G_OBJECT (button_ ##action), "clicked",\
              G_CALLBACK (on_button_selector_ ##action ##_clicked), NULL);

  _BUTTON_SETUP(new);
  _BUTTON_SETUP(import);
  _BUTTON_SETUP(open);
  _BUTTON_SETUP(delete);
  //_BUTTON_SETUP(properties);
  _BUTTON_SETUP(exit);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_new,        FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_open,       FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_delete,     FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_import,     FALSE, FALSE, 0);
  //gtk_box_pack_start (GTK_BOX (vbox), button_properties, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_exit,     FALSE, FALSE, 0);

  if(is_current_sketch_selected){//selector.sketch_is_selected){
    gtk_widget_set_sensitive(button_open,   TRUE);
    gtk_widget_set_sensitive(button_delete, TRUE);
  }
  else {
    gtk_widget_set_sensitive(button_open,   FALSE);
    gtk_widget_set_sensitive(button_delete, FALSE);
  }
  return vbox;
}

GtkWidget * build_selector_toolbar(){
  GtkWidget * toolbar;
  GtkWidget * button;
  GdkPixbuf * pixbuf;
  GtkWidget * pixmap;

  GtkWidget * files_popup_button;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  pixbuf = gpe_find_icon ("new");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  files_popup_button = popup_menu_button_new_from_stock (GTK_STOCK_NEW, _selector_popup_new, NULL);
  gtk_button_set_relief (GTK_BUTTON (files_popup_button), GTK_RELIEF_NONE);
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar),
                            files_popup_button, _("Sketch menu"), _("Sketch menu"));

  selector.files_popup_button = files_popup_button;  //keep a ref

  pixbuf = gpe_find_icon ("icons");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                                    _("Toggle view"), _("Toggle view"),
                                    pixmap,
                                    GTK_SIGNAL_FUNC(on_button_selector_change_view_clicked), NULL);
  g_object_set_data((GObject *) button, "icon_mode_icon", pixmap);
  pixbuf = gpe_find_icon ("list");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  g_object_set_data((GObject *) button, "list_mode_icon", pixmap);
  selector.button_view = button;

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  pixbuf = gpe_find_icon ("prefs");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                                    _("Preferences"), _("Preferences"),
				    pixmap,
				    GTK_SIGNAL_FUNC(on_button_selector_preferences_clicked), NULL);

  return toolbar;
}

static void on_title_edited (GtkCellRendererText *cell,
                             const gchar         *path_string,
                             const gchar         *new_title,
                             gpointer             unused_userdata)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  gchar * old_title;

  model = selector.listmodel;
  path  = gtk_tree_path_new_from_string (path_string);
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get(model, &iter, ENTRY_TITLE, &old_title, -1);

  if(strcmp(old_title, new_title) != 0){
    gtk_list_store_set (GTK_LIST_STORE(model), &iter, ENTRY_TITLE, new_title, -1);   
    
    {//update DB
      int id;
      gtk_tree_model_get(model, &iter, ENTRY_ID, &id, -1);
      db_update_title(id, new_title);
    }
  }

  g_free(old_title);
  gtk_tree_path_free(path);
}

GtkWidget * build_scrollable_textlist(){
  GtkWidget * scrolledwindow;//to return

  GtkTreeView  * treeview;
  GtkTreeModel * model;

  GtkTreeViewColumn * column;
  GtkCellRenderer   * renderer;

  model = selector.listmodel;
  treeview = (GtkTreeView *) gtk_tree_view_new_with_model (model);

  gtk_tree_view_set_rules_hint(treeview, TRUE);//request alternate color
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(treeview), GTK_SELECTION_SINGLE);
  gtk_tree_view_set_headers_visible(treeview,TRUE);

  gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(treeview),
                                         on_treeview_selection_change, //GtkTreeSelectionFunc func,
                                         NULL, //gpointer data,
                                         NULL  //GtkDestroyNotify destroy
                                         );

  g_signal_connect(G_OBJECT(treeview), "event", G_CALLBACK(on_treeview_event), model);

  g_object_unref(G_OBJECT(model));//treeview keep a ref on its model
  selector.textlistview = GTK_WIDGET(treeview);

//  //--COLUMN: Created
//  renderer = gtk_cell_renderer_text_new ();
//  column = gtk_tree_view_column_new_with_attributes (_("C"), renderer,
//                                                     "text", ENTRY_CREATED,
//                                                     NULL);
//  gtk_tree_view_column_set_sort_column_id (column, ENTRY_CREATED_VAL);
//  gtk_tree_view_append_column (treeview, column);

  //--COLUMN: Updated
  renderer = gtk_cell_renderer_text_new ();

  /* TRANSLATORS: "U" stands for "Updated"
   *  It is the title for the column that displays last update timestamp.
   *  The data in this column is no more than 5 characters.
   *  Please, try to make this title 5 characters max.
   */
  column = gtk_tree_view_column_new_with_attributes (_("U"), renderer,
                                                     "text", ENTRY_UPDATED,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, ENTRY_UPDATED_VAL);
  gtk_tree_view_append_column (treeview, column);

  //--COLUMN: title
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer,
                                                     "text", ENTRY_TITLE,
                                                     NULL);
  g_object_set(renderer, "editable", TRUE, NULL);
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_title_edited), model);
  gtk_tree_view_column_set_sort_column_id (column, ENTRY_TITLE);
  gtk_tree_view_append_column (treeview, column);

  //--Scrolled window and packing
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), GTK_WIDGET(treeview));

  return scrolledwindow;
}


void on_iconlist_item_activated(GtkIconView *iconview, GtkTreePath *treepath, gpointer user_data);

GtkWidget * build_scrollable_iconlist(GtkWidget * window){
  GtkWidget * scrolledwindow;//to return
  GtkWidget * icon_view;
  GtkTreeModel * model;

  model = selector.listmodel;

  icon_view = gtk_icon_view_new_with_model (model);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view), GTK_SELECTION_SINGLE);
  g_object_unref (model);
  
  selector.iconlistview = icon_view;

  gtk_icon_view_set_margin(GTK_ICON_VIEW(selector.iconlistview), 3);
  //g_printerr("margin : %d\n"
  //           "spacing: %d\n"
  //           "column : %d\n"
  //           "row    : %d\n"
  //           , gtk_icon_view_get_margin         (GTK_ICON_VIEW(selector.iconlistview))
  //           , gtk_icon_view_get_spacing        (GTK_ICON_VIEW(selector.iconlistview))
  //           , gtk_icon_view_get_column_spacing (GTK_ICON_VIEW(selector.iconlistview))
  //           , gtk_icon_view_get_row_spacing    (GTK_ICON_VIEW(selector.iconlistview))
  //           );


  /* We now set which model columns that correspont to the text
   * and pixbuf of each item
   */
  //gtk_icon_view_set_text_column   (GTK_ICON_VIEW (icon_view), ENTRY_TITLE);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icon_view), ENTRY_THUMBNAIL);
  
  /* Connect to the "item_activated" signal */
  g_signal_connect (icon_view, "item_activated", G_CALLBACK (on_iconlist_item_activated), model);

  //--Scrolled window and packing
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), GTK_WIDGET(icon_view));

  return scrolledwindow;

}

//FIXME: to move to selector-cb.c/h
#include "gpe-sketchbook.h"
#include "sketchpad.h"

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
