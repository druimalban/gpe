/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "networks_config_sql.h"

enum
{
  COLUMN_NETWORK,
  COLUMN_EDITABLE,
  COLUMN_SQL_NETWORK,
  NUM_COLUMNS
};

GtkTreeView *networks_config_tree_view;
GtkListStore *networks_config_list_store;

static void
kill_widget (GtkWidget *parent, GtkWidget *widget)
{
  gtk_widget_destroy (widget);
}

void
networks_config_add_from_sql ()
{
  GSList *iter;
  GtkTreeIter tree_iter;
 
  iter = sql_networks; 
  while (iter)
  {
    gtk_list_store_append (networks_config_list_store, &tree_iter);
    gtk_list_store_set (networks_config_list_store, &tree_iter, COLUMN_NETWORK, ((struct sql_network *) iter->data)->name, COLUMN_EDITABLE, TRUE, COLUMN_SQL_NETWORK, (gpointer) (struct sql_network *) iter->data, -1);
    iter = iter->next;
  }
}

void
networks_config_network_cell_edited (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data)
{
  GtkTreeModel *model = (GtkTreeModel *) data;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gchar *old_text;
  struct sql_network *network;
  
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter, COLUMN_NETWORK, &old_text, -1);
  g_free (old_text);

  gtk_tree_model_get (model, &iter, COLUMN_SQL_NETWORK, &network, -1);
  network->name = g_strdup (new_text);
  edit_sql_network (network);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NETWORK, new_text, -1);
}

void
networks_config_new ()
{
  struct sql_network *network;
  GtkTreeIter iter;

  network = new_sql_network ("Untitled", "", "", "");

  gtk_list_store_append (networks_config_list_store, &iter);
  gtk_list_store_set (networks_config_list_store, &iter, COLUMN_NETWORK, "Untitled", COLUMN_EDITABLE, TRUE, COLUMN_SQL_NETWORK, (gpointer) network, -1);
}

void
networks_config_delete ()
{
  GtkTreeSelection *selec;
  GtkTreeIter iter;
  struct sql_network *network;
  
  selec = gtk_tree_view_get_selection (networks_config_tree_view);
  if (gtk_tree_selection_get_selected (selec, NULL, &iter) == TRUE)
  {
    gtk_tree_model_get (GTK_TREE_MODEL (networks_config_list_store), &iter, COLUMN_SQL_NETWORK, &network, -1);
    del_sql_network (network);

    gtk_list_store_remove (networks_config_list_store, &iter);
  }
}

void
networks_config_edit ()
{
  GtkTreeSelection *selec;
  GtkTreeIter iter;
  struct sql_network *network;
  
  selec = gtk_tree_view_get_selection (networks_config_tree_view);
  if (gtk_tree_selection_get_selected (selec, NULL, &iter) == TRUE)
  {
    gtk_tree_model_get (GTK_TREE_MODEL (networks_config_list_store), &iter, COLUMN_SQL_NETWORK, &network, -1);
    networks_config_edit_window (network);
  }
}

void
networks_config_edit_window (struct sql_network *network)
{
  GtkWidget *window, *table, *vbox, *button_hbox, *label, *hsep;
  GtkWidget *nick_entry, *real_name_entry, *password_entry;
  GtkWidget *connect_button, *close_button, *network_properties_button;
  GtkTreeView *tree_view;
  GtkTreeViewColumn* column;
  GtkCellRenderer *renderer;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  guint window_x = 240, window_y = 310;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), g_strdup_printf ("IRC Client - Modify %s", network->name));
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (kill_widget), window);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (hbox), 3);
  button_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (button_hbox), 6);

  table = gtk_table_new (2, 5, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  label = gtk_label_new ("Network");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Server");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Nickname");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Real Name");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Password");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  hsep = gtk_hseparator_new ();

  network_combo = gtk_combo_new ();
  server_combo = gtk_combo_new ();

  nick_entry = gtk_entry_new ();
  real_name_entry = gtk_entry_new ();
  password_entry = gtk_entry_new ();

  connect_button = gpe_picture_button (button_hbox->style, "Connect", "globe");
  close_button = gpe_picture_button (button_hbox->style, "Close", "close");
  network_properties_button = gpe_picture_button (button_hbox->style, NULL, "properties");

  gtk_object_set_data (GTK_OBJECT (connect_button), "server_combo_entry", (gpointer) GTK_COMBO (server_combo)->entry);
  gtk_object_set_data (GTK_OBJECT (connect_button), "nick_entry", (gpointer) nick_entry);
  gtk_object_set_data (GTK_OBJECT (connect_button), "real_name_entry", (gpointer) real_name_entry);
  gtk_object_set_data (GTK_OBJECT (connect_button), "password_entry", (gpointer) password_entry);

/*
  gtk_signal_connect (GTK_OBJECT (connect_button), "clicked",
    		      GTK_SIGNAL_FUNC (new_connection), window);
  gtk_signal_connect (GTK_OBJECT (close_button), "clicked",
    		      GTK_SIGNAL_FUNC (kill_widget), window);
  gtk_signal_connect (GTK_OBJECT (network_properties_button), "clicked",
    		      GTK_SIGNAL_FUNC (networks_config_window), NULL);
*/

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), close_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), connect_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), network_combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), network_properties_button, FALSE, FALSE, 0);

  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), server_combo, 1, 2, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), nick_entry, 1, 2, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), real_name_entry, 1, 2, 3, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), password_entry, 1, 2, 4, 5);

  if (gpe_find_icon_pixmap ("globe", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show_all (window);
}

void
networks_config_window ()
{
  GtkWidget *window, *vbox, *button_hbox, *hsep, *scroll;
  GtkWidget *new_button, *edit_button, *delete_button;
  GtkTreeViewColumn* column;
  GtkCellRenderer *renderer;
  GdkPixmap *pmap;
  GdkBitmap *bmap;
  
  guint window_x = 240, window_y = 310;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "IRC Client - Configuration");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  button_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (button_hbox), 6);
  
  networks_config_list_store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (renderer, "edited", G_CALLBACK (networks_config_network_cell_edited), GTK_TREE_MODEL (networks_config_list_store));
  networks_config_tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (networks_config_list_store));
  gtk_tree_view_insert_column_with_attributes (networks_config_tree_view, -1, "Network", renderer, "text", COLUMN_NETWORK, "editable", COLUMN_EDITABLE, NULL);
  gtk_tree_view_set_headers_visible (networks_config_tree_view, TRUE);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  
  hsep = gtk_hseparator_new ();

  new_button = gpe_picture_button (button_hbox->style, "New", "new");
  edit_button = gpe_picture_button (button_hbox->style, "Modify", "preferences");
  delete_button = gpe_picture_button (button_hbox->style, "Delete", "delete");
  
  gtk_signal_connect (GTK_OBJECT (new_button), "clicked",
    		      GTK_SIGNAL_FUNC (networks_config_new), NULL);
  gtk_signal_connect (GTK_OBJECT (edit_button), "clicked",
    		      GTK_SIGNAL_FUNC (networks_config_edit), NULL);
  gtk_signal_connect (GTK_OBJECT (delete_button), "clicked",
    		      GTK_SIGNAL_FUNC (networks_config_delete), NULL);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (networks_config_tree_view));
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), delete_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), edit_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), new_button, FALSE, FALSE, 0);

  if (gpe_find_icon_pixmap ("properties", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);
    
  networks_config_add_from_sql ();

  gtk_widget_show_all (window);
}
