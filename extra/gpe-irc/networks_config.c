/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "main.h"
#include "networks_config_sql.h"

enum
{
  COLUMN_NETWORK,
  COLUMN_EDITABLE,
  COLUMN_SQL_NETWORK,
  NUM_COLUMNS
};

enum
{
  COLUMN_SERVER,
  COLUMN_PORT,
  COLUMN_EDITABLE2,
  COLUMN_SQL_SERVER,
  NUM_COLUMNS2
};

GtkTreeView *networks_config_tree_view;
GtkListStore *networks_config_list_store;

static void
kill_widget (GtkWidget * parent, GtkWidget * widget)
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
      gtk_list_store_set (networks_config_list_store, &tree_iter,
                          COLUMN_NETWORK,
                          ((struct sql_network *) iter->data)->name,
                          COLUMN_EDITABLE, TRUE, COLUMN_SQL_NETWORK,
                          (gpointer) (struct sql_network *) iter->data, -1);
      iter = iter->next;
    }
}

void
networks_config_add_servers_from_sql (GtkListStore * list_store,
                                      struct sql_network *network)
{
  GSList *iter;
  GtkTreeIter tree_iter;

  iter = network->servers;
  while (iter)
    {
      gtk_list_store_append (list_store, &tree_iter);
      gtk_list_store_set (list_store, &tree_iter, COLUMN_SERVER,
                          ((struct sql_network_server *) iter->data)->name,
                          COLUMN_PORT,
                          ((struct sql_network_server *) iter->data)->port,
                          COLUMN_EDITABLE2, TRUE, COLUMN_SQL_SERVER,
                          (gpointer) (struct sql_network *) iter->data, -1);
      iter = iter->next;
    }
}

void
networks_config_network_cell_edited (GtkCellRendererText * cell,
                                     const gchar * path_string,
                                     const gchar * new_text, gpointer data)
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

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NETWORK, new_text,
                      -1);
}

void
networks_config_edit_server_cell_edited (GtkCellRendererText * cell,
                                         const gchar * path_string,
                                         const gchar * new_text,
                                         gpointer data)
{
  GtkTreeModel *model = (GtkTreeModel *) data;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gchar *old_text;
  gint *column;
  struct sql_network_server *server;
  struct sql_network *network;

  column = g_object_get_data (G_OBJECT (cell), "column");
  network = g_object_get_data (G_OBJECT (cell), "irc_network");

  gtk_tree_model_get_iter (model, &iter, path);

  if (GPOINTER_TO_INT (column) == COLUMN_SERVER)
    {
      gtk_tree_model_get (model, &iter, COLUMN_SERVER, &old_text, -1);
      g_free (old_text);

      gtk_tree_model_get (model, &iter, COLUMN_SQL_SERVER, &server, -1);
      server->name = g_strdup (new_text);
      edit_sql_network_server (server, network);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SERVER,
                          new_text, -1);
    }
  else if (GPOINTER_TO_INT (column) == COLUMN_PORT)
    {
      gtk_tree_model_get (model, &iter, COLUMN_SQL_SERVER, &server, -1);
      server->port = atoi (new_text);
      edit_sql_network_server (server, network);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_PORT,
                          atoi (new_text), -1);
    }
}

void
networks_config_new ()
{
  struct sql_network *network;
  GtkTreeIter iter;

  network = new_sql_network ("Untitled", "", "", "");

  gtk_list_store_append (networks_config_list_store, &iter);
  gtk_list_store_set (networks_config_list_store, &iter, COLUMN_NETWORK,
                      "Untitled", COLUMN_EDITABLE, TRUE, COLUMN_SQL_NETWORK,
                      (gpointer) network, -1);
}

void
networks_config_new_server (GtkWidget * widget, struct sql_network *network)
{
  GtkListStore *list_store;
  struct sql_network_server *server;
  GtkTreeIter iter;

  list_store = g_object_get_data (G_OBJECT (widget), "list_store");
  server = new_sql_network_server ("Untitled", 6667, network);

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter, COLUMN_SERVER, "Untitled",
                      COLUMN_PORT, 6667, COLUMN_EDITABLE2, TRUE,
                      COLUMN_SQL_SERVER, (gpointer) server, -1);
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
      gtk_tree_model_get (GTK_TREE_MODEL (networks_config_list_store), &iter,
                          COLUMN_SQL_NETWORK, &network, -1);
      del_sql_network (network);

      gtk_list_store_remove (networks_config_list_store, &iter);
    }
}

void
networks_config_delete_server (GtkWidget * widget,
                               struct sql_network *network)
{
  GtkListStore *list_store;
  GtkTreeView *tree_view;
  GtkTreeSelection *selec;
  GtkTreeIter iter;
  struct sql_network_server *server;

  list_store = g_object_get_data (G_OBJECT (widget), "list_store");
  tree_view = g_object_get_data (G_OBJECT (widget), "tree_view");

  selec = gtk_tree_view_get_selection (tree_view);
  if (gtk_tree_selection_get_selected (selec, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter,
                          COLUMN_SQL_SERVER, &server, -1);
      del_sql_network_server (network, server);

      gtk_list_store_remove (list_store, &iter);
    }
}

void
networks_config_edit_save (GtkWidget * widget, struct sql_network *network)
{
  GtkWidget *window, *nick_entry, *real_name_entry, *password_entry;

  nick_entry = g_object_get_data (G_OBJECT (widget), "nick_entry");
  real_name_entry = g_object_get_data (G_OBJECT (widget), "real_name_entry");
  password_entry = g_object_get_data (G_OBJECT (widget), "password_entry");
  window = g_object_get_data (G_OBJECT (widget), "window");

  network->nick = g_strdup (gtk_entry_get_text (GTK_ENTRY (nick_entry)));
  network->real_name =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (real_name_entry)));
  network->password =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (password_entry)));

  edit_sql_network (network);
  gtk_widget_destroy (window);
}

void
networks_config_edit_window (struct sql_network *network)
{
  GtkWidget *window, *table, *frame, *frame2, *vbox, *vbox2, *button_hbox,
    *button_hbox2, *label, *hsep, *scroll;
  GtkWidget *nick_entry, *real_name_entry, *password_entry;
  GtkWidget *save_button, *close_button, *new_server_button,
    *delete_server_button;
  GtkCellRenderer *renderer;
  GtkTreeView *tree_view;
  GtkListStore *list_store;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  guint window_x = 240, window_y = 310;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window),
                        g_strdup_printf ("IRC Client - Modify %s",
                                         network->name));
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      G_CALLBACK (kill_widget), window);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (vbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  vbox2 = gtk_vbox_new (FALSE, 0);
  button_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (button_hbox), 6);
  button_hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox2), 6);

  frame = gtk_frame_new ("User Infomation");
  frame2 = gtk_frame_new ("Servers");

  table = gtk_table_new (2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  label = gtk_label_new ("Nickname");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Real Name");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Password");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  hsep = gtk_hseparator_new ();

  nick_entry = gtk_entry_new ();
  real_name_entry = gtk_entry_new ();
  password_entry = gtk_entry_new ();

  gtk_entry_set_text (GTK_ENTRY (nick_entry), network->nick);
  gtk_entry_set_text (GTK_ENTRY (real_name_entry), network->real_name);
  gtk_entry_set_text (GTK_ENTRY (password_entry), network->password);

  close_button =
    gpe_button_new_from_stock (GTK_STOCK_CLOSE, GPE_BUTTON_TYPE_BOTH);
  save_button =
    gpe_button_new_from_stock (GTK_STOCK_SAVE, GPE_BUTTON_TYPE_BOTH);
  new_server_button =
    gpe_button_new_from_stock (GTK_STOCK_ADD, GPE_BUTTON_TYPE_ICON);
  delete_server_button =
    gpe_button_new_from_stock (GTK_STOCK_REMOVE, GPE_BUTTON_TYPE_ICON);

  list_store =
    gtk_list_store_new (NUM_COLUMNS2, G_TYPE_STRING, G_TYPE_INT,
                        G_TYPE_BOOLEAN, G_TYPE_POINTER);
  tree_view =
    GTK_TREE_VIEW (gtk_tree_view_new_with_model
                   (GTK_TREE_MODEL (list_store)));

  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (networks_config_edit_server_cell_edited),
                    GTK_TREE_MODEL (list_store));
  g_object_set_data (G_OBJECT (renderer), "column", (gint *) COLUMN_SERVER);
  g_object_set_data (G_OBJECT (renderer), "irc_network", network);
  gtk_tree_view_insert_column_with_attributes (tree_view, -1, "Server",
                                               renderer, "text",
                                               COLUMN_SERVER, "editable",
                                               COLUMN_EDITABLE2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (networks_config_edit_server_cell_edited),
                    GTK_TREE_MODEL (list_store));
  g_object_set_data (G_OBJECT (renderer), "column", (gint *) COLUMN_PORT);
  g_object_set_data (G_OBJECT (renderer), "irc_network", network);
  gtk_tree_view_insert_column_with_attributes (tree_view, -1, "Port",
                                               renderer, "text", COLUMN_PORT,
                                               "editable", COLUMN_EDITABLE2,
                                               NULL);

  gtk_tree_view_set_headers_visible (tree_view, FALSE);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  g_signal_connect (G_OBJECT (close_button), "clicked",
                    G_CALLBACK (kill_widget), window);
  g_signal_connect (G_OBJECT (save_button), "clicked",
                    G_CALLBACK (networks_config_edit_save), network);
  g_signal_connect (G_OBJECT (new_server_button), "clicked",
                    G_CALLBACK (networks_config_new_server), network);
  g_signal_connect (G_OBJECT (delete_server_button), "clicked",
                    G_CALLBACK (networks_config_delete_server), network);

  g_object_set_data (G_OBJECT (save_button), "window", (gpointer) window);
  g_object_set_data (G_OBJECT (save_button), "nick_entry",
                     (gpointer) nick_entry);
  g_object_set_data (G_OBJECT (save_button), "real_name_entry",
                     (gpointer) real_name_entry);
  g_object_set_data (G_OBJECT (save_button), "password_entry",
                     (gpointer) password_entry);
  g_object_set_data (G_OBJECT (new_server_button), "list_store",
                     (gpointer) list_store);
  g_object_set_data (G_OBJECT (delete_server_button), "tree_view",
                     (gpointer) tree_view);
  g_object_set_data (G_OBJECT (delete_server_button), "list_store",
                     (gpointer) list_store);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (tree_view));
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (table));
  gtk_container_add (GTK_CONTAINER (frame2), GTK_WIDGET (vbox2));
  gtk_box_pack_start (GTK_BOX (vbox2), scroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame2, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_hbox2, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox2), close_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox2), save_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), delete_server_button, FALSE, FALSE,
                    0);
  gtk_box_pack_end (GTK_BOX (button_hbox), new_server_button, FALSE, FALSE,
                    0);

  gtk_table_attach_defaults (GTK_TABLE (table), nick_entry, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), real_name_entry, 1, 2, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), password_entry, 1, 2, 2, 3);

  if (gpe_find_icon_pixmap ("preferences", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  networks_config_add_servers_from_sql (list_store, network);

  gtk_widget_show_all (window);
  gtk_tree_view_columns_autosize (tree_view);
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
      gtk_tree_model_get (GTK_TREE_MODEL (networks_config_list_store), &iter,
                          COLUMN_SQL_NETWORK, &network, -1);
      networks_config_edit_window (network);
    }
}

void
networks_config_window_close (GtkWidget * parent, GtkWidget * widget)
{
  GtkWidget *window = NULL;

  get_networks (g_object_get_data (G_OBJECT (widget), "network_combo"),
                g_object_get_data (G_OBJECT (widget), "network_hash"));

  window = g_object_get_data (G_OBJECT (parent), "window");
  if (window)
    gtk_widget_destroy (window);
}

void
networks_config_window (GtkWidget * widget)
{
  GtkWidget *window, *frame, *vbox, *vbox2, *button_hbox, *button_hbox2,
    *hsep, *scroll;
  GtkWidget *new_button, *edit_button, *delete_button, *save_button,
    *close_button;
  GtkCellRenderer *renderer;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  guint window_x = 240, window_y = 310;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "IRC Client - Configuration");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      G_CALLBACK (networks_config_window_close), widget);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_set_spacing (GTK_BOX (vbox), 6);
  vbox2 = gtk_vbox_new (FALSE, 0);
  button_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (button_hbox), 6);
  button_hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox2), 6);

  frame = gtk_frame_new ("Networks");

  networks_config_list_store =
    gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN,
                        G_TYPE_POINTER);
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (networks_config_network_cell_edited),
                    GTK_TREE_MODEL (networks_config_list_store));
  networks_config_tree_view =
    GTK_TREE_VIEW (gtk_tree_view_new_with_model
                   (GTK_TREE_MODEL (networks_config_list_store)));
  gtk_tree_view_insert_column_with_attributes (networks_config_tree_view, -1,
                                               "Network", renderer, "text",
                                               COLUMN_NETWORK, "editable",
                                               COLUMN_EDITABLE, NULL);
  gtk_tree_view_set_headers_visible (networks_config_tree_view, FALSE);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  hsep = gtk_hseparator_new ();

  new_button =
    gpe_button_new_from_stock (GTK_STOCK_ADD, GPE_BUTTON_TYPE_ICON);
  edit_button =
    gpe_button_new_from_stock (GTK_STOCK_PREFERENCES, GPE_BUTTON_TYPE_ICON);
  delete_button =
    gpe_button_new_from_stock (GTK_STOCK_REMOVE, GPE_BUTTON_TYPE_ICON);
  close_button =
    gpe_button_new_from_stock (GTK_STOCK_CLOSE, GPE_BUTTON_TYPE_BOTH);
  save_button =
    gpe_button_new_from_stock (GTK_STOCK_SAVE, GPE_BUTTON_TYPE_BOTH);

  g_signal_connect (G_OBJECT (new_button), "clicked",
                    G_CALLBACK (networks_config_new), NULL);
  g_signal_connect (G_OBJECT (edit_button), "clicked",
                    G_CALLBACK (networks_config_edit), NULL);
  g_signal_connect (G_OBJECT (delete_button), "clicked",
                    G_CALLBACK (networks_config_delete), NULL);
  g_signal_connect (G_OBJECT (close_button), "clicked",
                    G_CALLBACK (networks_config_window_close), widget);
  g_signal_connect (G_OBJECT (save_button), "clicked",
                    G_CALLBACK (networks_config_window_close), widget);

  g_object_set_data (G_OBJECT (close_button), "window", (gpointer) window);
  g_object_set_data (G_OBJECT (save_button), "window", (gpointer) window);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_container_add (GTK_CONTAINER (scroll),
                     GTK_WIDGET (networks_config_tree_view));
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (vbox2));
  gtk_box_pack_start (GTK_BOX (vbox2), scroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_hbox2, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), delete_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), edit_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), new_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox2), close_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox2), save_button, FALSE, FALSE, 0);

  if (gpe_find_icon_pixmap ("properties", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  networks_config_add_from_sql ();

  gtk_widget_show_all (window);
}
