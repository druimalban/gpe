/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Some additions by Florian Boor <florian.boor@kernelconcepts.de> 2005
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
#include <sys/stat.h>
#include <dirent.h>

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/smallbox.h>

#include "frontend.h"

#define _(x) gettext(x)

enum
  {
    TITLE_COLUMN,
    DATA_COLUMN,
    N_COLUMNS
  };

gboolean add_playlist_item (struct nmf_frontend *fe, char *fn);

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
  struct nmf_frontend *fe = g_object_get_data (G_OBJECT (d), "frontend");
  fe->fs_open = FALSE;
  gtk_widget_destroy (GTK_WIDGET (d));
}

gboolean
add_playlist_file (struct nmf_frontend *fe, char *s)
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
      p->data.track.url = g_strdup (s);

      player_fill_in_playlist (p);

      if (p->title == NULL)
        p->title = g_path_get_basename (p->data.track.url);
    }

  if (p)
    {
      p->parent = fe->playlist;
      fe->playlist->data.list = g_slist_append (fe->playlist->data.list, p);
      player_set_playlist (fe->player, fe->playlist);
      playlist_edit_push (fe->playlist_widget, fe->playlist);
    }

  return TRUE;
}

gboolean
add_playlist_directory (struct nmf_frontend *fe, char *dname)
{
  DIR *dir;
  struct dirent *entry;
  
  dir = opendir (dname);
  if (! dir)
    {
      gpe_perror_box (dname);
      return FALSE;
    }
 
  while (entry = readdir (dir), entry != NULL)
    {
      gchar *fn;
      
      if (!strcmp (entry->d_name, ".") || !strcmp (entry->d_name, ".."))
        continue;
      
      fn = g_strdup_printf ("%s/%s", dname, entry->d_name);
      
      add_playlist_item (fe, fn);
      
      g_free (fn);
    }

  closedir (dir);

  return TRUE;
}

int 
isdir (const char *fn)
{
  struct stat buf;
  
  if (stat (fn, &buf))
    return 0;
  
  if (S_ISDIR (buf.st_mode))
    return 1;
  
  return 0;
}

gboolean
add_playlist_item (struct nmf_frontend *fe, char *fn)
{
  return isdir (fn) ? add_playlist_directory (fe, fn) : add_playlist_file (fe, fn);
}

static void
select_file_done (GtkWidget *w, GtkWidget *fs)
{
  int i = 0;
  struct nmf_frontend *fe = g_object_get_data (G_OBJECT (fs), "frontend");
  gchar **files = gtk_file_selection_get_selections(GTK_FILE_SELECTION(fs));

  const char *s = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

  while (files[i])
    {
      add_playlist_item (fe, files[i]);
      i++;
    }

  if (fe->current_path)
    g_free(fe->current_path);
  if (isdir (s))
    {
      fe->current_path = g_strdup_printf ("%s/", s);
    }
  else
    {
      gchar *dir = g_path_get_dirname (s);
      fe->current_path = g_strdup_printf ("%s/", dir);
      g_free (dir);
    }
  
  fe->fs_open = FALSE;

  gtk_widget_destroy (fs);
}

static void
apply_list (GtkWidget *w, struct nmf_frontend *fe)
{  
 if (!fe->playing)
    {
      struct player_status ps;
      if (player_play (fe->player))
        {
          player_status (fe->player, &ps);
          update_track_info (fe, ps.item);
          fe->playing = TRUE;
        }
      else
        {
          fe->player->state = PLAYER_STATE_NULL;
          fe->playing = FALSE;
        }
    }
    else
    	if( fe->player->state == PLAYER_STATE_PAUSED )
    		player_play (fe->player);
    gtk_widget_hide(gtk_widget_get_toplevel(w));
}

static void
new_entry (GtkWidget *w, struct nmf_frontend *fe)
{  
  GtkWidget *fs;
	
  /* check is selector is already open */
  if (fe->fs_open) return; 
  fe->fs_open = TRUE;
  
  fs = gtk_file_selection_new (_("Select file"));
  
  if (fe->current_path)
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), fe->current_path);
  gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(fs), TRUE);
  
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (fs)->cancel_button), 
		    "clicked", G_CALLBACK (close_file_sel), fs);
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (fs)->ok_button), 
		    "clicked", G_CALLBACK (select_file_done), fs);
  
  g_object_set_data (G_OBJECT (fs), "frontend", fe);

  gtk_widget_show (fs);
}

void
delete (GtkWidget *w, struct nmf_frontend *fe)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (fe->view));
  GtkTreeStore *store = g_object_get_data (G_OBJECT (fe->view), "store");
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
    struct playlist *item;
    gtk_tree_model_get (model, &iter, DATA_COLUMN, &item, -1);

    if (item) {
      struct playlist *parent = item->parent;
      if (parent) {
          parent->data.list = g_slist_remove (parent->data.list, item);
      }
      // now free the subtree
      playlist_free (item);
    }
    gtk_tree_store_remove (store, &iter);
  }
}

void
playlist_edit_push (GtkWidget *w, struct playlist *p)
{
  GtkTreeStore *store = g_object_get_data (G_OBJECT (w), "store");

  gtk_tree_store_clear (store);
  store_playlist (store, p, NULL);
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

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 300);
  gtk_window_set_title(GTK_WINDOW (window), _("Playlist"));
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_ADD,
			    _("Tap here to close playlist and start playing."), NULL,
			    G_CALLBACK (new_entry), fe, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_REMOVE,
			    _("Tap here to remove the selected item."), NULL,
			    G_CALLBACK (delete), fe, -1);
  
  gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), 2);
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OK,
			    _("Tap here to remove the selected item."), NULL,
			    G_CALLBACK (apply_list), fe, -1);
                
  g_object_set_data (G_OBJECT (window), "store", store);
  g_object_set_data (G_OBJECT (treeview), "store", store);
  store_playlist (store, p, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  gtk_container_add (GTK_CONTAINER (sw), treeview);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

  fe->model = GTK_TREE_MODEL (store);
  fe->view = GTK_TREE_VIEW (treeview);
  fe->fs_open = FALSE;
  
  g_signal_connect (G_OBJECT (treeview), "row-activated",
		    G_CALLBACK (row_signal), fe);

  gtk_widget_show (sw);
  gtk_widget_show (toolbar);
  gtk_widget_show (treeview);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (hide_window), NULL);

  return window;
}
