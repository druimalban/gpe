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


GtkWidget * create_window_selector(){

  GtkWidget *window_selector;
  GtkWidget *vbox1;

  //toolbar
  GtkWidget *hbox_selector_toolbar;
  GtkWidget *button_selector_new;    
  GtkWidget *button_selector_open;   
  GtkWidget *button_selector_delete; 
  GtkWidget *button_sketchpad_view;
  GtkWidget *button_selector_about;
  GdkPixbuf *pixbuf;
  GtkWidget *pixmap;

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
  button_selector_new    = gtk_button_new ();
  button_selector_open   = gtk_button_new ();
  button_selector_delete = gtk_button_new ();
  button_sketchpad_view  = gtk_button_new ();
  button_selector_about  = gtk_button_new ();

  //no relief
  gtk_button_set_relief (GTK_BUTTON (button_selector_about),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_sketchpad_view),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_selector_delete), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_selector_open),   GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_selector_new),    GTK_RELIEF_NONE);

  //pixmaps
  pixbuf = gpe_find_icon ("new");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_selector_new), pixmap);
  pixbuf = gpe_find_icon ("open");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_selector_open), pixmap);
  pixbuf = gpe_find_icon ("delete");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_selector_delete), pixmap);
  pixbuf = gpe_find_icon ("sketchpad");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_sketchpad_view), pixmap);
  pixbuf = gpe_find_icon ("about");
  pixmap = gpe_render_icon (window_selector->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_selector_about), pixmap);

  //signals connection
  gtk_signal_connect (GTK_OBJECT (button_selector_new), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_new_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_open), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_open_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_delete), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_delete_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_sketchpad_view), "clicked",
                      GTK_SIGNAL_FUNC (on_button_sketchpad_view_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_selector_about), "clicked",
                      GTK_SIGNAL_FUNC (on_button_selector_about_clicked), NULL);

  //--packing
  hbox_selector_toolbar = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_new,    FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_open,   FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_selector_delete, FALSE, FALSE, 0);
  //FIXME: remove related code gtk_box_pack_start (GTK_BOX (hbox_selector_toolbar), button_sketchpad_view,  FALSE, FALSE, 0);
  gtk_box_pack_end   (GTK_BOX (hbox_selector_toolbar), button_selector_about,  FALSE, FALSE, 0);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox_selector_toolbar,         FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow_selector_clist, TRUE,  TRUE,  0);

  gtk_container_add (GTK_CONTAINER (window_selector), vbox1);

  //show all except the toplevel window
  gtk_widget_show_all(vbox1);

  return window_selector;
}

