/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "dirbrowser.h"
#include "picturebutton.h"
#include "pixmaps.h"
#include "errorbox.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(x) dgettext(PACKAGE, x)
#else
#define _(x) (x)
#endif

enum
  {
    NAME_COLUMN,
    PATH_COLUMN,
    PIX_COLUMN,
    SCANNED_COLUMN,
    N_COLUMNS
  };

static GdkPixbuf *pix_closed, *pix_open;

static void
add_dir (GtkTreeStore *store, GtkTreeIter *iter, gchar *path, GtkTreeIter *result)
{
  gchar *leafname;
  GtkTreeIter new;

  gtk_tree_store_append (store, &new, iter);
  leafname = g_path_get_basename (path);
  gtk_tree_store_set (store, &new, NAME_COLUMN, leafname, PATH_COLUMN, path, PIX_COLUMN, pix_closed, -1);
  g_free (leafname);

  if (result)
    memcpy (result, &new, sizeof (new));
}

static void
scan_dir (GtkTreeStore *store, GtkTreeIter *iter, gchar *path)
{
  DIR *dir;
  struct dirent *de;
  GList *entries = NULL, *i;

  dir = opendir (path);
  if (dir == NULL)
    return;

  while (de = readdir (dir), de != NULL)
    {
      gchar *name;
      if (de->d_name[0] == '.')
	continue;
      if (de->d_type != DT_DIR)
	continue;
      name = g_strdup_printf ("%s%s/", path, de->d_name);
      entries = g_list_insert_sorted (entries, name, (GCompareFunc)strcoll);
    }
  closedir (dir);
  
  for (i = entries; i; i = i->next)
    {
      gchar *name;
      name = i->data;
      add_dir (store, iter, name, NULL);
      g_free (name);
    }
  g_list_free (entries);
}

static void
row_expanded (GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *t_path, void *p)
{
  GtkTreeStore *store = GTK_TREE_STORE (p);
  gchar *path;
  GtkTreeIter child;

  if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, iter))
    {
      do
	{
	  gboolean scanned;
	  gtk_tree_model_get (GTK_TREE_MODEL (store), &child, SCANNED_COLUMN, &scanned, -1);
	  if (scanned == FALSE)
	    {
	      gtk_tree_model_get (GTK_TREE_MODEL (store), &child, PATH_COLUMN, &path, -1);
	      scan_dir (store, &child, path);
	      g_free (path);
	      gtk_tree_store_set (store, iter, SCANNED_COLUMN, TRUE, -1);
	    }
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &child));
    }

  gtk_tree_store_set (store, iter, PIX_COLUMN, pix_open, -1);
}

static void
row_collapsed (GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *path, void *p)
{
  GtkTreeStore *store = GTK_TREE_STORE (p);

  gtk_tree_store_set (store, iter, PIX_COLUMN, pix_closed, -1);
}

static void
build_parent_directories (GtkTreeStore *store, GtkTreeIter *iter, gchar *path)
{
  gchar *leafname;
  GtkTreeIter new;
  
  if (strcmp (path, ".") && strcmp (path, "/"))
    {
      gchar *parent = g_path_get_dirname (path);
      build_parent_directories (store, iter, parent);
      g_free (parent);
      gtk_tree_store_append (store, &new, iter);
    }
  else
    gtk_tree_store_append (store, &new, NULL);

  leafname = g_path_get_basename (path);
  gtk_tree_store_set (store, &new, NAME_COLUMN, leafname, PATH_COLUMN, path, PIX_COLUMN, pix_closed, -1);
  g_free (leafname);

  memcpy (iter, &new, sizeof (new));
}

static void
ok_clicked (GObject *obj, GObject *window)
{
  GtkTreeView *view = g_object_get_data (window, "view");
  void (*handler) (gchar *) = g_object_get_data (window, "handler");
  GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
  GtkTreeModel *model;
  GtkTreeIter iter;
 
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *path;
      gtk_tree_model_get (model, &iter, PATH_COLUMN, &path, -1);   
      gtk_widget_hide (GTK_WIDGET (window));   
      handler (path);
      g_free (path);
      
      gtk_widget_destroy (GTK_WIDGET (window));
      return;
    }

  gpe_error_box (_("No directory is selected."));
}

static void
cancel_clicked (GObject *obj, GObject *window)
{
  gtk_widget_destroy (GTK_WIDGET (window));
}

GtkWidget *
gpe_create_dir_browser (gchar * title, gchar *current_path, 
			GtkSelectionMode mode, void (*handler) (gchar *))
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *ok_button = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  GtkWidget *cancel_button = gpe_button_new_from_stock (GTK_STOCK_CANCEL, 
							GPE_BUTTON_TYPE_BOTH);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  GtkTreeStore *store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_BOOLEAN);
  GtkWidget *tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  GtkTreeIter iter;
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkCellRenderer *pix_renderer = gtk_cell_renderer_pixbuf_new ();
  GtkTreeViewColumn *column = gtk_tree_view_column_new ();
  GtkTreePath *path;
  gchar *p;

  if (!pix_closed)
    pix_closed = gpe_try_find_icon ("dir-closed", NULL);
  if (!pix_open)
    pix_open = gpe_try_find_icon ("dir-open", NULL);

  gtk_tree_view_column_pack_start (column, pix_renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, pix_renderer, "pixbuf", PIX_COLUMN, NULL);

  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer, "text", NAME_COLUMN, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  g_signal_connect (G_OBJECT (tree_view), "row-expanded", G_CALLBACK (row_expanded), store);
  g_signal_connect (G_OBJECT (tree_view), "row-collapsed", G_CALLBACK (row_collapsed), store);

  add_dir (store, NULL, "/", &iter);
  scan_dir (store, &iter, "/");

  gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);

  path = gtk_tree_path_new_first ();
  gtk_tree_view_expand_row (GTK_TREE_VIEW (tree_view), path, FALSE);
  gtk_tree_path_free (path);

  p = current_path;
  while (p)
    {
      gchar *next;
      GtkTreeIter new_iter;
      gboolean found = FALSE;

      while (*p == '/')
	p++;
      if (*p == 0)
	break;

      next = strchr (p, '/');

      if (next)
	*(next++) = 0;

      if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &new_iter, &iter))
	break;

      do
	{
	  gchar *name;
	  gtk_tree_model_get (GTK_TREE_MODEL (store), &new_iter, NAME_COLUMN, &name, -1);
	  if (!strcmp (name, p))
	    {
	      path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &new_iter);
	      gtk_tree_view_expand_row (GTK_TREE_VIEW (tree_view), path, FALSE);	
	      gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree_view), path, NULL, FALSE);
	      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree_view), path, NULL, FALSE, 0, 0);
	      gtk_tree_path_free (path);
	      found = TRUE;
	      break;
	    }
	  g_free (name);
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &new_iter));

      if (!found)
	break;

      iter = new_iter;

      p = next;
    }

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_widget_show (ok_button);
  gtk_widget_show (cancel_button);
  gtk_widget_show (scrolled_window);
  gtk_widget_show (tree_view);
  
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);

  gtk_window_set_title (GTK_WINDOW (window), title);

  g_object_set_data (G_OBJECT (window), "view", tree_view);
  g_object_set_data (G_OBJECT (window), "handler", handler);

  g_signal_connect (G_OBJECT (ok_button), "clicked", G_CALLBACK (ok_clicked), window);
  g_signal_connect (G_OBJECT (cancel_button), "clicked", G_CALLBACK (cancel_clicked), window);

  return window;
}
