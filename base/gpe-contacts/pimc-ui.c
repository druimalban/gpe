/*
 * Hildon UI category code, (c) 2005 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Based on libgpepimc/ui.c
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "gpe/pim-categories.h"

#define _(x) (x)


static gboolean
categories_button_press_event (GtkWidget * widget, GdkEventButton * b,
                               GtkListStore * list_store)
{
  gboolean ret = FALSE;

  if (b->button == 1)
    {
      gint x, y;
      GtkTreeViewColumn *col;
      GtkTreePath *path;

      x = b->x;
      y = b->y;

      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					 x, y, &path, &col, NULL, NULL))
        {
          GtkTreeViewColumn *toggle_col;
          GtkTreeIter iter;
    
          toggle_col = g_object_get_data (G_OBJECT (widget), "toggle-col");
    
          if (col == toggle_col)
            {
              gboolean active;
    
              gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter,
                                       path);
    
              gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0,
                                  &active, -1);
    
              active = !active;
    
              gtk_list_store_set (GTK_LIST_STORE (list_store), &iter, 0,
                                  active, -1);
    
              ret = TRUE;
            }
    
          gtk_tree_path_free (path);
        }
    }

  return ret;
}


GSList *
get_categories (GtkWidget *w)
{
  GtkTreeIter iter;
  GtkListStore *list_store;
  GSList *old_categories, *i;
  GSList *selected_categories = NULL;

  list_store = g_object_get_data (G_OBJECT (w), "list_store");

  old_categories = gpe_pim_categories_list ();

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      do
        {
          gint id;
          gchar *title;
          gboolean selected;
    
          gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0,
                              &selected, 1, &title, 2, &id, -1);
    
            {
              struct gpe_pim_category *c = NULL;
    
              for (i = old_categories; i; i = i->next)
                {
                  c = i->data;
        
                  if (c->id == id)
                    break;
                }
    
              if (i)
                {
                  old_categories = g_slist_remove_link (old_categories, i);
                  g_slist_free (i);
                }
              else
                selected = FALSE;	/* category was deleted by second party */
            }
    
          if (selected)
            selected_categories =
              g_slist_prepend (selected_categories, (gpointer) id);
    
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
    }

  g_slist_free (old_categories);

  return selected_categories;
}

void
populate_pim_categories_list (GtkWidget *w, GSList * selected_categories)
{
  GSList *iter;
  GSList *list;
  GtkListStore *list_store;
    
  list_store = g_object_get_data (G_OBJECT (w), "list_store");
  list = gpe_pim_categories_list ();

  gtk_list_store_clear(list_store);
  for (iter = list; iter; iter = iter->next)
    {
      struct gpe_pim_category *c = iter->data;
      GtkTreeIter titer;

      gtk_list_store_append (list_store, &titer);

      gtk_list_store_set (list_store, &titer,
                          0, g_slist_find (selected_categories, 
                                           (gpointer) c->id) ? TRUE : FALSE,
                          1, c->name, 2, c->id, -1);
    }
    
  g_slist_free (list);
}

GtkWidget *
build_pim_categories_list (void)
{
  GtkListStore *list_store;
  GtkWidget *tree_view;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkWidget *sw;

  list_store =
    gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
   
  sw = gtk_scrolled_window_new(NULL, NULL);

  renderer = gtk_cell_renderer_toggle_new ();
  g_object_set (G_OBJECT (renderer), "activatable", TRUE, NULL);
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
                                                  "active", 0, NULL);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);
  g_object_set_data (G_OBJECT (tree_view), "toggle-col", col);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", FALSE, NULL);
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer, "text", 1, NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  g_signal_connect (G_OBJECT (tree_view), "button_press_event",
                    G_CALLBACK (categories_button_press_event), list_store);

  gtk_container_add (GTK_CONTAINER (sw), tree_view);


  g_object_set_data (G_OBJECT (sw), "list_store", list_store);

  gtk_widget_show_all (sw);

  return sw;
}
