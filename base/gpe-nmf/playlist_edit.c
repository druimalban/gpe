/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <libintl.h>
#include <string.h>

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/gtkminifilesel.h>
#include <gpe/render.h>
#include <gpe/smallbox.h>

#include "frontend.h"
#include "decoder.h"

#define _(x) gettext(x)

#if GTK_MAJOR_VERSION < 2
#error Eat flaming death, GTK 1 users!
#endif

enum
  {
    TITLE_COLUMN,
    DATA_COLUMN,
    N_COLUMNS
  };

static void
store_playlist (GtkTreeStore *store, struct playlist *p, GtkTreeIter *parent)
{
  GSList *i;
  GtkTreeIter iter;
  if (p)
    {
      assert (p->type == ITEM_TYPE_LIST);
      for (i = p->data.list; i; i = i->next)
	{
	  struct playlist *item = i->data;
	  gtk_tree_store_append (store, &iter, parent);
	  gtk_tree_store_set (store, &iter, TITLE_COLUMN, item->title, -1);
	  gtk_tree_store_set (store, &iter, DATA_COLUMN, item, -1);
	  switch (item->type)
	    {
	    case ITEM_TYPE_TRACK:
	      break;
	    case ITEM_TYPE_LIST:
	      store_playlist (store, item, &iter);
	      break;
	    }
	}
    }
}

static void
close_file_sel (GtkWidget *w, gpointer d)
{
  gtk_widget_destroy (GTK_WIDGET (d));
}

void
load_file (struct nmf_frontend *fe, gchar *s)
{
  struct playlist *p = NULL;

  if (strstr (s, ".npl") || strstr (s, ".xml"))
    {
      p = playlist_xml_load (s);
    }
  else if (strstr (s, ".m3u"))
    {
      p = playlist_m3u_load (s);
    }
  else
    {
      p = playlist_new_track ();
      p->data.track.url = s;
      decoder_fill_in_playlist (p);

      if (p->title == NULL)
	p->title = p->data.track.url;
    }

  if (p)
    {
      p->parent = fe->playlist;
      fe->playlist->data.list = g_slist_append (fe->playlist->data.list, p);
      player_set_playlist (fe->player, fe->playlist);
      playlist_edit_push (fe->playlist_widget, fe->playlist);
    }
}

static void
select_file_done (GtkWidget *fs, struct nmf_frontend *fe)
{
  char *s = gtk_mini_file_selection_get_filename (GTK_MINI_FILE_SELECTION (fs));
  
  load_file (fe, s);
  
  gtk_widget_destroy (fs);
}

static void
new_entry (GtkWidget *w, struct nmf_frontend *fe)
{
  GtkWidget *fs = gtk_mini_file_selection_new (_("Select file"));
  gtk_signal_connect (GTK_OBJECT (GTK_MINI_FILE_SELECTION (fs)->cancel_button), 
		      "clicked", GTK_SIGNAL_FUNC (close_file_sel), fs);
  gtk_signal_connect (GTK_OBJECT (fs), "completed", 
		      GTK_SIGNAL_FUNC (select_file_done), fe);
  gtk_widget_show (fs);
}

void
new_folder (GtkWidget *w, gpointer d)
{
  char *name = smallbox (_("New folder"), _("Title"), "");
  if (name)
    {
    }
}

void
delete (GtkWidget *w, struct nmf_frontend *fe)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (fe->view));
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      struct playlist *item;
      gtk_tree_model_get (model, &iter, DATA_COLUMN, &item, -1);
    }
}

void
playlist_edit_push (GtkWidget *w, struct playlist *p)
{
  GtkTreeStore *store = gtk_object_get_data (GTK_OBJECT (w), "store");

  gtk_tree_store_clear (store);
  store_playlist (store, p, NULL);
  gtk_widget_draw (w, NULL);
}

static void
row_signal (GtkTreeView *treeview, GtkTreePath *path,
	    GtkTreeViewColumn *col, struct nmf_frontend *fe)
{
  GtkTreeIter iter;
  
  if (gtk_tree_model_get_iter (fe->model, &iter, path))
    {
      struct playlist *item;
      struct player_status ps;
      gtk_tree_model_get (fe->model, &iter, DATA_COLUMN, &item, -1);
      if (item->type == ITEM_TYPE_LIST || item->parent == NULL)
	{
	  player_set_playlist (fe->player, item);
	}
      else
	{
	  player_set_playlist (fe->player, item->parent);
	  player_set_index (fe->player, g_slist_index (item->parent->data.list, item));
	}
      player_play (fe->player);
      player_status (fe->player, &ps);
      update_track_info (fe, ps.item);
      fe->playing = TRUE;
    }
}

static gboolean
hide_window (GtkWidget *w)
{
  gtk_widget_hide (w);
  return TRUE;
}

GtkWidget *
playlist_edit (struct nmf_frontend *fe, struct playlist *p)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *toolbar = gtk_toolbar_new ();
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
  GtkTreeStore *store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  GtkWidget *treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (_("Title"), 
									renderer, 
									"text", 
									TITLE_COLUMN, 
									NULL);
  GtkWidget *pw;

  pw = gpe_render_icon (NULL, gpe_find_icon ("open"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"), 
			   _("Open"), _("Open"), pw, 
			   GTK_SIGNAL_FUNC (new_entry), fe);

  pw = gpe_render_icon (NULL, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete"), 
			   _("Delete"), _("Delete"), pw, 
			   GTK_SIGNAL_FUNC (delete), fe);
  
  gtk_object_set_data (GTK_OBJECT (window), "store", store);
  store_playlist (store, p, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  gtk_container_add (GTK_CONTAINER (sw), treeview);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

  fe->model = GTK_TREE_MODEL (store);
  fe->view = GTK_TREE_VIEW (treeview);

  gtk_signal_connect (GTK_OBJECT (treeview), "row-activated",
		      GTK_SIGNAL_FUNC (row_signal), fe);

  gtk_widget_show (sw);
  gtk_widget_show (toolbar);
  gtk_widget_show (treeview);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_signal_connect (GTK_OBJECT (window), "delete-event",
		      GTK_SIGNAL_FUNC (hide_window), NULL);

  return window;
}
