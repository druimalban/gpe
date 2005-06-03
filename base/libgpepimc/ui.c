/*
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

#include "internal.h"

#define _(x) (x)

static void
do_new_category (GtkWidget *widget, GtkWidget *d)
{
  GtkWidget *entry = gtk_object_get_data (GTK_OBJECT (d), "entry");
  char *title = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  GSList *l, *list;
  GtkTreeIter iter;
  GtkListStore *list_store;

  list_store = g_object_get_data (G_OBJECT (d), "list-store");

  if (title[0] == 0)
    {
      gpe_error_box (_("Category name must not be blank"));
      gtk_widget_destroy (d);
      return;
    }

  list = gpe_pim_categories_list ();
  for (l = list; l; l = l->next)
    {
      struct gpe_pim_category *c = l->data;
      if (!strcasecmp (title, c->name))
	{
	  gpe_error_box (_("A category by that name already exists"));
	  gtk_widget_destroy (d);
	  return;
	}
    }
  g_slist_free (list);

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, FALSE, 1, title, 2, -1, -1);

  gtk_widget_destroy (d);
}

static void
new_category (GtkWidget *w, GtkListStore *list_store)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *ok;
  GtkWidget *cancel;
  GtkWidget *label;
  GtkWidget *name;
  GtkWidget *hbox;
  guint spacing;

  spacing = gpe_get_boxspacing ();

  window = gtk_dialog_new ();

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (w)));

  vbox = GTK_DIALOG (window)->vbox;

  hbox = gtk_hbox_new (FALSE, 0);

  label = gtk_label_new (_("Name:"));
  name = gtk_entry_new ();
  
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), name, TRUE, TRUE, 2);

#ifdef IS_HILDON
  ok = gtk_button_new_with_label (_("OK"));
  cancel = gtk_button_new_with_label (_("Cancel"));

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, 
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, 
		      FALSE, FALSE, 0);
#else
  ok = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, 
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, 
		      FALSE, FALSE, 0);
#endif
  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok);

  g_signal_connect (G_OBJECT (ok), "clicked", 
		    G_CALLBACK (do_new_category), window);
  
  g_signal_connect (G_OBJECT (name), "activate", 
		    G_CALLBACK (do_new_category), window);

  g_signal_connect_swapped (G_OBJECT (cancel), "clicked", 
			    G_CALLBACK (gtk_widget_destroy), window);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, spacing);

  gtk_object_set_data (GTK_OBJECT (window), "entry", name);
  
  gtk_window_set_title (GTK_WINDOW (window), _("New category"));

  gpe_set_window_icon (window, "icon");

  gtk_container_set_border_width (GTK_CONTAINER (window), gpe_get_border ());

  g_object_set_data (G_OBJECT (window), "list-store", list_store);

  gtk_widget_show_all (window);
  gtk_widget_grab_focus (name);
}

static void
delete_category (GtkWidget *w, GtkWidget *tree_view)
{
  GtkTreeSelection *sel;
  GList *list, *iter;
  GtkTreeModel *model;
  GSList *refs = NULL, *riter;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

  list = gtk_tree_selection_get_selected_rows (sel, &model);

  for (iter = list; iter; iter = iter->next)
    {
      GtkTreePath *path;
      GtkTreeRowReference *ref;

      path = list->data;
      ref = gtk_tree_row_reference_new (model, path);
      gtk_tree_path_free (path);

      refs = g_slist_prepend (refs, ref);
    }

  g_list_free (list);

  for (riter = refs; riter; riter = riter->next)
    {
      GtkTreeRowReference *ref;
      GtkTreePath *path;

      ref = riter->data;
      path = gtk_tree_row_reference_get_path (ref);
      if (path)
	{
	  GtkTreeIter it;
	  
	  if (gtk_tree_model_get_iter (model, &it, path))
	    gtk_list_store_remove (GTK_LIST_STORE (model), &it);

	  gtk_tree_path_free (path);
	}

      gtk_tree_row_reference_free (ref);
    }

  g_slist_free (refs);
}

static gboolean
categories_button_press_event (GtkWidget *widget, GdkEventButton *b, GtkListStore *list_store)
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
					 x, y,
					 &path, &col,
					 NULL, NULL))
	{
	  GtkTreeViewColumn *toggle_col;
	  GtkTreeIter iter;

	  toggle_col = g_object_get_data (G_OBJECT (widget), "toggle-col");

	  if (col == toggle_col)
	    {
	      gboolean active;

	      gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

	      gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &active, -1);

	      active = !active;

	      gtk_list_store_set (GTK_LIST_STORE (list_store), &iter, 0, active, -1);
	      
	      ret = TRUE;
	    }

	  gtk_tree_path_free (path);
	}
    }

  return ret;
}

static void
categories_dialog_cancel (GtkWidget *w, gpointer p)
{
  GtkWidget *window = GTK_WIDGET (p);

  gtk_widget_destroy (window);
}

static void
categories_dialog_ok (GtkWidget *w, gpointer p)
{
  GtkWidget *window;
  GtkTreeIter iter;
  GtkListStore *list_store;
  GSList *old_categories, *i;
  GSList *selected_categories = NULL;
  void (*callback) (GtkWidget *, GSList *, gpointer);
  gpointer data;

  window = GTK_WIDGET (p);
  list_store = g_object_get_data (G_OBJECT (window), "list_store");

  old_categories = gpe_pim_categories_list ();

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      do 
	{
	  gint id;
	  gchar *title;
	  gboolean selected;
	  
	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &selected, 1, &title, 2, &id, -1);

	  if (id == -1)
	    gpe_pim_category_new (title, &id);
	  else
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
		  
		  if (strcmp (c->name, title))
		    {
		      g_free ((void *)c->name);
		      c->name = g_strdup (title);
		      gpe_pim_category_rename (id, title);
		    }
		}
	      else
		selected = FALSE;		/* category was deleted by second party */
	    }

	  if (selected)
	    selected_categories = g_slist_prepend (selected_categories, (gpointer)id);

	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
    }

  for (i = old_categories; i; i = i->next)
    {
      struct gpe_pim_category *c = i->data;

      gpe_pim_category_delete (c);
    }

  g_slist_free (old_categories);

  callback = g_object_get_data (G_OBJECT (window), "callback");
  data = g_object_get_data (G_OBJECT (window), "callback-data");

  if (callback)
    (*callback) (window, selected_categories, data);

  g_slist_free (selected_categories);

  gtk_widget_destroy (window);
}

static void
change_category_name (GtkCellRendererText *cell,
                      gchar               *path_string,
                      gchar               *new_text,
                      gpointer             user_data)
{
  GtkListStore *list_store;
  GtkTreeIter iter;
  struct todo_item *i;
  gint id;

  list_store = GTK_LIST_STORE (user_data);

  if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (list_store),
                                            &iter, path_string))
    {
      gpe_error_box ("Error getting changed item");
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 2, &id, -1);

  /* Update the name in the list, it will be updated in the db on ok. */
  gtk_list_store_set (list_store, &iter, 1, new_text, -1);
}

#ifdef IS_HILDON
GtkWidget *
gpe_pim_categories_dialog (GSList *selected_categories, gboolean select, 
                           GCallback callback, gpointer data)
#else
GtkWidget *
gpe_pim_categories_dialog (GSList *selected_categories, GCallback callback, gpointer data)
#endif
{
  GtkWidget *toolbar;
  GtkWidget *window;
  GtkWidget *sw;
  GSList *iter;
  GtkWidget *okbutton = NULL, *cancelbutton = NULL;
  GtkListStore *list_store;
  GtkWidget *tree_view;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GSList *list;
#ifdef IS_HILDON
  GtkWidget *newbutton, *deletebutton;
#endif
    
  window = gtk_dialog_new ();

  list_store = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

#ifndef IS_HILDON
    toolbar = gtk_toolbar_new ();
    gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
    
    gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
                  _("New category"), 
                  _("Tap here to add a new category."),
                  G_CALLBACK (new_category), list_store, -1);
    
    gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
                  _("Delete category"), 
                  _("Tap here to delete the selected category."),
                  G_CALLBACK (delete_category), tree_view, -1);
    
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
                toolbar, FALSE, FALSE, 0);
#endif      
  sw = gtk_scrolled_window_new (NULL, NULL);

  list = gpe_pim_categories_list ();
  for (iter = list; iter; iter = iter->next)
    {
      struct gpe_pim_category *c = iter->data;
      GtkTreeIter titer;

      gtk_list_store_append (list_store, &titer);

      gtk_list_store_set (list_store, &titer, 
			  0, g_slist_find (selected_categories, (gpointer)c->id) ? TRUE : FALSE,
			  1, c->name, 
			  2, c->id, 
			  -1);
    }
  g_slist_free (list);

#ifdef IS_HILDON
  if (select)
#endif
    {  
      renderer = gtk_cell_renderer_toggle_new ();
      g_object_set (G_OBJECT (renderer), "activatable", TRUE, NULL);
      col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
                              "active", 0,
                              NULL);
    
      gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);
    
      g_object_set_data (G_OBJECT (tree_view), "toggle-col", col);
    }
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						  "text", 1,
						  NULL);

  g_signal_connect (G_OBJECT (renderer), "edited", 
                    G_CALLBACK(change_category_name),
                    list_store);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);

  g_signal_connect (G_OBJECT (tree_view), "button_press_event", 
		    G_CALLBACK (categories_button_press_event), list_store);

  gtk_container_add (GTK_CONTAINER (sw), tree_view);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), sw, TRUE, TRUE, 0);

  gpe_set_window_icon (window, "icon");

#ifdef IS_HILDON
  if (select)
  {
    okbutton = gtk_button_new_with_label (_("OK"));
    cancelbutton = gtk_button_new_with_label (_("Cancel"));
  }
  else
  {
    okbutton = gtk_button_new_with_label (_("Close"));
    newbutton = gtk_button_new_with_label (_("New"));
    deletebutton = gtk_button_new_with_label (_("Delete"));
  }
#else
  okbutton = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancelbutton = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif

  g_object_set_data (G_OBJECT (window), "list_store", list_store);

  if (okbutton) 
    g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (categories_dialog_ok), window);
  if (cancelbutton)
    g_signal_connect (G_OBJECT (cancelbutton), "clicked", G_CALLBACK (categories_dialog_cancel), window);

#ifdef IS_HILDON
if (select)
  {
    gtk_window_set_title (GTK_WINDOW (window), _("Select categories"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancelbutton, TRUE, TRUE, 0);
  }
else
  {
    gtk_window_set_title (GTK_WINDOW (window), _("Edit categories"));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), newbutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), deletebutton, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (newbutton), "clicked", G_CALLBACK (new_category), list_store);
    g_signal_connect (G_OBJECT (deletebutton), "clicked", G_CALLBACK (delete_category), tree_view);
  }
#else
  gtk_window_set_title (GTK_WINDOW (window), _("Select categories"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancelbutton, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
#endif
  
  GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (okbutton);
  
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);

  g_signal_connect_swapped (G_OBJECT (window), "destroy", G_CALLBACK (g_object_unref), list_store);

  g_object_set_data (G_OBJECT (window), "callback", callback);
  g_object_set_data (G_OBJECT (window), "callback-data", data);

  gtk_widget_show_all (window);

  return window;
}
