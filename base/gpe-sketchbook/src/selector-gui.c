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

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

GtkWidget * create_window_selector(){

  GtkWidget *window_selector;
  GtkWidget *vbox; //hbox + clist
  GtkWidget *hbox; //toolbar + help button

  //toolbar
  GtkWidget *toolbar;
  GdkPixbuf *pixbuf;
  GtkWidget *pixmap;

  GtkWidget * button_help;

  //clist
  GtkWidget *scrolledwindow_selector_clist;
  GtkWidget *clist_selector;
  GtkWidget *label_Clist_column1;

  //--Main window
  window_selector = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window_selector), 240, 280);
#endif
  gtk_signal_connect (GTK_OBJECT (window_selector), "destroy",
                      GTK_SIGNAL_FUNC (on_window_selector_destroy),
                      NULL);

  //--Clist
  clist_selector = gtk_clist_new (1);
  set_selector_clist(GTK_CLIST(clist_selector));//set a ref to the clist
  gtk_clist_set_column_width (GTK_CLIST (clist_selector), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist_selector));
  label_Clist_column1 = gtk_label_new ("column 1");
  gtk_clist_set_column_widget (GTK_CLIST (clist_selector), 0, label_Clist_column1);

  scrolledwindow_selector_clist = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_selector_clist),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_selector_clist), clist_selector);

  gtk_signal_connect (GTK_OBJECT (clist_selector), "select_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_select_row), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "click_column",
                      GTK_SIGNAL_FUNC (on_clist_selector_click_column), NULL);
  gtk_signal_connect (GTK_OBJECT (clist_selector), "unselect_row",
                      GTK_SIGNAL_FUNC (on_clist_selector_unselect_row), NULL);

  //--Toolbar
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  pixbuf = gpe_find_icon ("new");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("New sketch"), _("New sketch"),
                           pixmap, on_button_selector_new_clicked, NULL);
  pixbuf = gpe_find_icon ("open");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("Open selected sketch"), _("Open selected sketch"),
                           pixmap, on_button_selector_open_clicked, NULL);
  pixbuf = gpe_find_icon ("delete");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("Delete selected sketch"), _("Delete selected sketch"),
                           pixmap, on_button_selector_delete_clicked, NULL);
  //pixbuf = gpe_find_icon ("sketchpad");
  //pixmap = gpe_render_icon (window_selector->style, pixbuf);
  //gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
  //                         _("Switch to the sketchpad view"), _("Switch to the sketchpad view"),
  //                         pixmap, on_button_selector_sketchpad_view_clicked, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  //--help button
  button_help = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button_help),  GTK_RELIEF_NONE);

  pixbuf = gpe_find_icon ("about");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_help), pixmap);
  gtk_signal_connect (GTK_OBJECT (button_help), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_about_clicked), NULL);

  //--packing
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar,      FALSE, FALSE, 0);
  gtk_box_pack_end   (GTK_BOX (hbox), button_help,  FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox,                          FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow_selector_clist, TRUE,  TRUE,  0);

  gtk_container_add (GTK_CONTAINER (window_selector), vbox);

  //show all except the toplevel window
  gtk_widget_show_all(vbox);

  return window_selector;
}

