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

#include "selector-gui.h"
#include "selector-cb.h"
#include "selector.h"

//gpe libs
#include "gpe/pixmaps.h"
#include "gpe/render.h"
#include "gpe/gpeiconlistview.h"

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
  icons_mode = FALSE;
  if(icons_mode) gtk_widget_show_all(selector.iconlist);
  else           gtk_widget_show_all(selector.textlist);

  return vbox;
}

GtkWidget * build_selector_toolbar(){
  GtkWidget * toolbar;
  GtkWidget * button;
  GdkPixbuf * pixbuf;
  GtkWidget * pixmap;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  pixbuf = gpe_find_icon ("new");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("New sketch"), _("New sketch"),
                           pixmap, GTK_SIGNAL_FUNC(on_button_selector_new_clicked), NULL);
  pixbuf = gpe_find_icon ("open");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("Edit sketch"), _("Edit sketch"),
                           pixmap, GTK_SIGNAL_FUNC(on_button_selector_open_clicked), NULL);
  gtk_widget_set_sensitive(button, FALSE);
  selector.button_edit = button;
  pixbuf = gpe_find_icon ("delete");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("Delete selected sketch"), _("Delete selected sketch"),
                           pixmap, GTK_SIGNAL_FUNC(on_button_selector_delete_clicked), NULL);
  gtk_widget_set_sensitive(button, FALSE);
  selector.button_delete = button;

  pixbuf = gpe_find_icon ("import");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                                    _("Import from image"), _("Import from image"),
				    pixmap,
				    GTK_SIGNAL_FUNC(on_button_selector_import_clicked), NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

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
  gtk_tree_view_set_headers_visible(treeview, FALSE);

  gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(treeview),
                                         on_treeview_selection_change, //GtkTreeSelectionFunc func,
                                         NULL, //gpointer data,
                                         NULL  //GtkDestroyNotify destroy
                                         );

  g_signal_connect(G_OBJECT(treeview), "event", G_CALLBACK(on_treeview_event), model);

  g_object_unref(G_OBJECT(model));//treeview keep a ref on its model
  selector.textlistview = GTK_WIDGET(treeview);

  //--COLUMN: title
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("title", renderer,
                                                     "text", ENTRY_TITLE,
                                                     NULL);
  //g_object_set(renderer, "editable", TRUE, NULL);
  //g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_title_edited), model);
  gtk_tree_view_column_set_sort_column_id (column, ENTRY_TITLE);
  gtk_tree_view_append_column (treeview, column);

  //--Scrolled window and packing
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), GTK_WIDGET(treeview));

  return scrolledwindow;
}


void on_iconlist_clicked    (GtkWidget * iconlist, gpointer il_data, gpointer data);
//void on_iconlist_show_popup (GtkWidget * iconlist, gpointer il_data, gpointer data);

GtkWidget * build_scrollable_iconlist(GtkWidget * window){
  GtkWidget * iconlist;

  iconlist = gpe_icon_list_view_new();
  gpe_icon_list_view_set_icon_size (GPE_ICON_LIST_VIEW(iconlist), THUMBNAIL_SIZE);
  gpe_icon_list_view_set_bg_color  (GPE_ICON_LIST_VIEW(iconlist), 0xbabac444);//0xddddd444);//light grey 
  gpe_icon_list_view_set_show_title(GPE_ICON_LIST_VIEW(iconlist), FALSE);
  gpe_icon_list_view_set_icon_xmargin (GPE_ICON_LIST_VIEW(iconlist), 4);

  g_signal_connect (G_OBJECT (iconlist), "clicked",
                    G_CALLBACK (on_iconlist_clicked), NULL);
  //g_signal_connect (G_OBJECT (iconlist), "show_popup",
  //                  G_CALLBACK (on_iconlist_show_popup), NULL);

  return iconlist;
}

//FIXME: to move to selector-cb.c/h
#include "gpe-sketchbook.h"
#include "sketchpad.h"
void on_iconlist_clicked (GtkWidget * iconlist, gpointer iter, gpointer data) {
  //**/g_printerr("ICONLIST> %s\n", (char *)data);
  GtkTreeModel * model;
  gchar * fullpath_filename;
        
  model = GTK_TREE_MODEL(selector.listmodel);

  gtk_tree_model_get(model, (GtkTreeIter *)iter,
                     ENTRY_URL, &fullpath_filename, -1);

  {
    GtkTreePath * path;
    gint * indices;
    path = gtk_tree_model_get_path(selector.listmodel, iter);
    indices = gtk_tree_path_get_indices(path);

    current_sketch = indices[0];

    gtk_tree_path_free(path);
  }
  set_current_sketch_selected();

  sketchpad_open_file(fullpath_filename);
  switch_to_page(PAGE_SKETCHPAD);
}

//void on_iconlist_show_popup (GtkWidget *il, gpointer note, gpointer data) {
//  /**/g_printerr("ICONLIST> %s", data);
//}
