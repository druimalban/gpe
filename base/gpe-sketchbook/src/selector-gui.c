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
#include "gpe/gpe-iconlist.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

//list clist/icons views
GtkWidget * scrolledwindow_selector_clist;
GtkWidget * scrolledwindow_selector_icons;

gboolean icons_mode;

GtkWidget * build_selector_toolbar();
GtkWidget * build_scrollable_clist();
GtkWidget * build_scrollable_icons();

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

  //--Clist
  scrolledwindow_selector_clist = build_scrollable_clist();
  scrolledwindow_selector_icons = build_scrollable_icons();

  //--packing
  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox,                          FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow_selector_clist, TRUE,  TRUE,  0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow_selector_icons, TRUE,  TRUE,  0);

  //--show all except top level window AND one view
  gtk_widget_show(vbox);//top box
  gtk_widget_show_all(hbox);//toolbar
  icons_mode = FALSE;
  if(icons_mode) gtk_widget_show_all(scrolledwindow_selector_icons);
  else           gtk_widget_show_all(scrolledwindow_selector_clist);

  return vbox;
}

GtkWidget * build_selector_toolbar(){
  GtkWidget * toolbar;
  GtkWidget * button;
  GdkPixbuf * pixbuf;
  GtkWidget * pixmap;

#ifdef GTK2
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style      (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  //gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  //gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#endif

  pixbuf = gpe_find_icon ("new");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           NULL, NULL,
                           pixmap, GTK_SIGNAL_FUNC(on_button_selector_new_clicked), NULL);
  pixbuf = gpe_find_icon ("open");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           NULL, NULL,
                           pixmap, GTK_SIGNAL_FUNC(on_button_selector_open_clicked), NULL);
  pixbuf = gpe_find_icon ("delete");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           NULL, NULL,
                           //_("Delete selected sketch"), _("Delete selected sketch"),
                           pixmap, GTK_SIGNAL_FUNC(on_button_selector_delete_clicked), NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  pixbuf = gpe_find_icon ("icons");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                                    NULL, NULL,
                                    pixmap,
                                    GTK_SIGNAL_FUNC(on_button_selector_change_view_clicked), NULL);
  g_object_set_data((GObject *) button, "icon_mode_icon", pixmap);
  pixbuf = gpe_find_icon ("list");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  g_object_set_data((GObject *) button, "list_mode_icon", pixmap);
  
  return toolbar;
}

GtkWidget * build_scrollable_clist(){
  GtkWidget * scrolledwindow;//to return
  GtkWidget * clist_selector;
  GtkWidget * label_Clist_column1;

  clist_selector = gtk_clist_new (1);
  set_selector_clist(GTK_CLIST(clist_selector));//set a ref to the clist

  gtk_clist_set_column_width (GTK_CLIST (clist_selector), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist_selector));
  label_Clist_column1 = gtk_label_new ("column 1");
  gtk_clist_set_column_widget (GTK_CLIST (clist_selector), 0, label_Clist_column1);

  gtk_signal_connect (GTK_OBJECT (clist_selector), "select_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_select_row), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "click_column",
                      GTK_SIGNAL_FUNC (on_clist_selector_click_column), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "unselect_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_unselect_row), NULL);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), clist_selector);

  return scrolledwindow;
}


void on_iconlist_clicked    (GtkWidget * iconlist, gpointer il_data, gpointer data);
//void on_iconlist_show_popup (GtkWidget * iconlist, gpointer il_data, gpointer data);

GtkWidget * build_scrollable_icons(GtkWidget * window){
  GtkWidget * iconlist;

  iconlist = gpe_iconlist_new();
  gpe_iconlist_set_icon_size (GPE_ICONLIST(iconlist), THUMBNAIL_SIZE);
  gpe_iconlist_set_bg_color  (GPE_ICONLIST(iconlist), 0xddddd444);//light grey 
	gpe_iconlist_set_show_title(GPE_ICONLIST(iconlist), FALSE);

  g_signal_connect (G_OBJECT (iconlist), "clicked",
                    G_CALLBACK (on_iconlist_clicked), "clicked!!!");
  //g_signal_connect (G_OBJECT (iconlist), "show_popup",
  //                  G_CALLBACK (on_iconlist_show_popup), "popup!!!");

  return iconlist;
}

#include "note.h"
void on_iconlist_clicked (GtkWidget * iconlist, gpointer note, gpointer data) {
  //**/g_printerr("ICONLIST> %s\n", (char *)data);
  current_sketch = gtk_clist_find_row_from_data(selector_clist, note);
  set_current_sketch_selected();
  on_button_selector_open_clicked (NULL, NULL);
}

//void on_iconlist_show_popup (GtkWidget *il, gpointer note, gpointer data) {
//  /**/g_printerr("ICONLIST> %s", data);
//}

void build_thumbnail_widget(Note * note, GtkStyle * style){
  GtkWidget * button;
  GdkPixbuf * pixbuf;
  GdkPixbuf * pixbuf_scaled;
  GtkWidget * pixmap;

  button = gtk_button_new();
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  //li gtk_signal_connect (GTK_OBJECT (button), "clicked",
  //li                     GTK_SIGNAL_FUNC (on_iconsview_icon_clicked), note);

#ifdef GTK2
  pixbuf = gdk_pixbuf_new_from_file(note->fullpath_filename, NULL); //GError **error
#else
  pixbuf = gdk_pixbuf_new_from_file(note->fullpath_filename);
#endif
  pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf,
                                           THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                           //GDK_INTERP_NEAREST);
                                           //GDK_INTERP_TILES);
                                           GDK_INTERP_BILINEAR);
                                           //GDK_INTERP_HYPER);
  gdk_pixbuf_unref(pixbuf);
  pixmap = gtk_image_new_from_pixbuf (pixbuf_scaled);
  note->thumbnail = pixbuf_scaled;//li
  gdk_pixbuf_unref(pixbuf_scaled);
  gtk_container_add (GTK_CONTAINER (button), pixmap);
  if(note->icon_widget != NULL) gtk_widget_destroy(note->icon_widget);
  note->icon_widget = button;
}

