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

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/gtkminifilesel.h>
#include <gpe/render.h>
#include <gpe/smallbox.h>

#include "playlist_db.h"

#define _(x) gettext(x)

#if GTK_MAJOR_VERSION < 2
#error Eat flaming death, GTK 1 users!
#endif

typedef void (*double_click_func)(struct playlist *, void *);

static double_click_func double_click_hook;
static void *double_click_data;

enum
  {
    TITLE_COLUMN,
    DATA_COLUMN,
    N_COLUMNS
  };

void
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

static void
select_file_done (GtkWidget *fs, gpointer d)
{
  struct playlist *p = playlist_new_track ();
  p->data.track.url = gtk_mini_file_selection_get_filename (GTK_MINI_FILE_SELECTION (fs));

  gtk_widget_destroy (fs);
}

static void
new_entry (GtkWidget *w, gpointer d)
{
  GtkWidget *fs = gtk_mini_file_selection_new (_("Select file"));
  gtk_signal_connect (GTK_OBJECT (GTK_MINI_FILE_SELECTION (fs)->cancel_button), "clicked", close_file_sel, fs);
  gtk_signal_connect (GTK_OBJECT (fs), "completed", select_file_done, d);
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
delete (GtkWidget *w, gpointer p)
{
  GtkWidget *treeview = GTK_WIDGET (p);
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
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

void
playlist_edit_double_click_hook (void *p, void *d)
{
  double_click_hook = (double_click_func)p;
  double_click_data = d;
}

static void
row_signal (GtkTreeView *treeview, GtkTreePath *path,
	    GtkTreeViewColumn *col, gpointer d)
{
  GtkTreeModel *model = (GtkTreeModel *)d;
  GtkTreeIter iter;
  
  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      struct playlist *item;
      gtk_tree_model_get (model, &iter, DATA_COLUMN, &item, -1);
      if (double_click_hook)
	double_click_hook (double_click_data, item);
    }
}

GtkWidget *
playlist_edit (struct playlist *p)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *toolbar = gtk_toolbar_new ();
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkTreeStore *store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  GtkWidget *treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (_("Title"), 
									renderer, 
									"text", 
									TITLE_COLUMN, 
									NULL);
  GtkWidget *pw;

  pw = gpe_render_icon (NULL, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Track"), 
			   _("Track"), _("Track"), pw, 
			   new_entry, treeview);

  pw = gpe_render_icon (NULL, gpe_find_icon ("dir-closed"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Folder"), 
			   _("Folder"), _("Folder"), pw, 
			   new_folder, treeview);

  pw = gpe_render_icon (NULL, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete"), 
			   _("Delete"), _("Delete"), pw, 
			   delete, treeview);
  
  gtk_object_set_data (GTK_OBJECT (window), "store", store);
  store_playlist (store, p, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), treeview, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_WIDGET (treeview), "row-activated",
		      GTK_SIGNAL_FUNC (row_signal), store);

  gtk_widget_show (toolbar);
  gtk_widget_show (treeview);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  return window;
}
