/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/pim-categories.h>

#include "todo.h"

#define _(_x) gettext(_x)

static GtkWidget *g_option;
static gint selected_category = -1;

GtkListStore *list_store;
GtkWidget *item_menu;

GdkPixbuf *tick_icon, *no_tick_icon;

static struct todo_item *current_menu_item;

static void
item_do_edit (void)
{
  gtk_widget_show_all (edit_item (current_menu_item, -1));

  current_menu_item = NULL;
}

static void
item_do_delete (void)
{
  todo_db_delete_item (current_menu_item);

  refresh_items ();

  current_menu_item = NULL;
}

static void
set_category (GtkWidget *w, gpointer user_data)
{
  selected_category = (gint)user_data;
  refresh_items ();
}

void
categories_menu (void)
{
  GtkWidget *menu = gtk_menu_new ();
  GSList *l;
  GtkWidget *i;

  i = gtk_menu_item_new_with_label (_("All items"));
  gtk_menu_append (GTK_MENU (menu), i);
  g_signal_connect (G_OBJECT (i), "activate", G_CALLBACK (set_category), (gpointer)-1);
  gtk_widget_show (i);

  for (l = gpe_pim_categories_list (); l; l = l->next)
    {
      struct gpe_pim_category *t = l->data;
      i = gtk_menu_item_new_with_label (t->name);
      gtk_menu_append (GTK_MENU (menu), i);
      g_signal_connect (G_OBJECT (i), "activate", G_CALLBACK (set_category), (gpointer)t->id);
      gtk_widget_show (i);
    }

  gtk_widget_show (menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (g_option), menu);
}

static void
new_todo_item (GtkWidget *w, gpointer user_data)
{
  GtkWidget *todo = edit_item (NULL, selected_category);

  gtk_widget_show_all (todo);
}

void
open_editing_window (GtkTreePath *path)
{
  GtkTreeIter iter;
  struct todo_item *i;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 3, &i, -1);

  gtk_widget_show_all (edit_item (i, -1));
}

void
toggle_completed (GtkTreePath *path)
{
  GtkTreeIter iter;
  struct todo_item *i;
  gboolean complete;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 3, &i, -1);
		      
  if (i->state == COMPLETED)
    i->state = NOT_STARTED;
  else
    i->state = COMPLETED;
  
  complete = (i->state == COMPLETED) ? TRUE : FALSE;

  gtk_list_store_set (list_store, &iter, 
		      0, complete ? tick_icon : no_tick_icon,
		      1, i->summary,  
		      2, complete, 
		      3, i,
		      -1);

  todo_db_push_item (i);
}

int
sort_by_priority (gconstpointer a, gconstpointer b)
{
  struct todo_item *ia, *ib;

  ia = (struct todo_item *)a;
  ib = (struct todo_item *)b;

  return ib->priority - ia->priority;
}

void
refresh_items (void)
{
  GSList *list, *iter;

  gtk_list_store_clear (list_store);

  list = g_slist_copy (todo_db_get_items_list ());

  list = g_slist_sort (list, sort_by_priority);

  for (iter = list; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;
      if (selected_category == -1
	  || g_slist_find (i->categories, (gpointer)selected_category))
	{
	  GtkTreeIter iter;
	  gboolean complete;

	  complete = (i->state == COMPLETED) ? TRUE : FALSE;

	  gtk_list_store_append (list_store, &iter);

	  gtk_list_store_set (list_store, &iter, 
			      0, complete ? tick_icon : no_tick_icon,
			      1, i->summary,  
			      2, complete, 
			      3, i,
			      -1);
	}
    }

  g_slist_free (list);
}

gboolean
button_press_event (GtkWidget *widget, GdkEventButton *b)
{
  if (b->button == 3)
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
	  GtkTreeIter iter;
	  struct todo_item *i;

	  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 3, &i, -1);

	  gtk_menu_popup (GTK_MENU (item_menu), NULL, NULL, NULL, NULL, b->button, b->time);

	  current_menu_item = i;

	  gtk_tree_path_free (path);
	}
    }

  return TRUE;
}

gboolean
button_release_event (GtkWidget *widget, GdkEventButton *b)
{
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
	  GtkTreeViewColumn *pix_col;
	      
	  pix_col = g_object_get_data (G_OBJECT (widget), "pixmap-column");
	  
	  if (pix_col == col)
	    toggle_completed (path);
	  else
	    open_editing_window (path);

	  gtk_tree_path_free (path);
	}
    }

  return TRUE;
}

static GtkItemFactoryEntry menu_items[] =
{
  { "/_Edit",   NULL, item_do_edit,     0, "<Item>" },
  { "/_Delete", NULL, item_do_delete,   0, "<StockItem>", GTK_STOCK_DELETE },
};

GtkWidget *
top_level (GtkWidget *window)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *toolbar;
  GtkWidget *option = gtk_option_menu_new ();
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *list_view;
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;

  no_tick_icon = gpe_find_icon ("notick-box");
  tick_icon = gpe_find_icon ("tick-box");

  g_option = option;
  categories_menu ();

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", accel_group);
  g_object_set_data_full (G_OBJECT (window), "<main>", item_factory, (GDestroyNotify) g_object_unref);
  gtk_item_factory_create_items (item_factory, sizeof (menu_items) / sizeof (menu_items[0]), 
				 menu_items, NULL);

  item_menu = gtk_item_factory_get_widget (item_factory, "<main>");

  /* Design the Tool Bar a little better, see Bug 733 */
  /* New | Conf | Purge Down | List | Exit */
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("New item"), _("Tap here to add a new item."),
			    G_CALLBACK (new_todo_item), NULL, -1);

  /* New */
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);

  /* | */
  gtk_box_pack_start (GTK_BOX (hbox), gtk_vseparator_new(), FALSE, FALSE, 0);

  /* List */
  gtk_box_pack_start (GTK_BOX (hbox), option, FALSE, FALSE, 0);

  /* | */
  gtk_box_pack_start (GTK_BOX (hbox), gtk_vseparator_new(), FALSE, FALSE, 0);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    _("Quit"), _("Tap here to quit the program."),
			    G_CALLBACK (gtk_main_quit), NULL, -1);

  /* Exit */
  gtk_box_pack_end (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  list_store = gtk_list_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_pixbuf_new ();
    col = gtk_tree_view_column_new_with_attributes ("...", renderer,
						    "pixbuf", 0,
						    NULL);

    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);

    g_object_set_data (G_OBJECT (list_view), "pixmap-column", col);

    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes ("...", renderer,
						    "text", 1,
						    "strikethrough", 2,
						    NULL);

    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);

    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (list_view), TRUE);
  }

  g_signal_connect (G_OBJECT (list_view), "button_press_event", G_CALLBACK (button_press_event), NULL);
  g_signal_connect (G_OBJECT (list_view), "button_release_event", G_CALLBACK (button_release_event), NULL);
  
  gtk_container_add (GTK_CONTAINER (scrolled), list_view);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  refresh_items ();

  return vbox;
}
