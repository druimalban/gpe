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
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/pim-categories.h>

#include "todo.h"

#define _(_x) gettext(_x)

static GtkWidget *g_option;
static gint selected_category = -1;
static gboolean show_completed_tasks = TRUE;

GtkListStore *list_store;
GtkWidget *item_menu;

GdkPixbuf *tick_icon, *no_tick_icon, *bar_icon, *dot_icon;

static struct todo_item *current_menu_item;
extern GtkWidget *window;

static void
item_do_edit (void)
{
  gtk_widget_show_all (edit_item (current_menu_item, -1, GTK_WINDOW(window)));

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

static void
toggle_completed_items (GtkWidget *w, gpointer user_data)
{
   show_completed_tasks = 
       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM (w));
   refresh_items ();
}


void
categories_menu (void)
{
  GtkWidget *menu = gtk_menu_new ();
  GSList *l;
  GtkWidget *i;
  gboolean found;

  found = FALSE;

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

      if (t->id == selected_category)
        found = TRUE;
    }

  if (!found && selected_category != -1)
    {
      selected_category = -1;
      refresh_items ();
    }

  i = gtk_separator_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), i);
  gtk_widget_show (i);

  i = gtk_check_menu_item_new_with_label (_("Show completed"));
  gtk_check_menu_item_set_active
     (GTK_CHECK_MENU_ITEM (i), show_completed_tasks);
  g_signal_connect (G_OBJECT (i), "toggled", 
                    G_CALLBACK (toggle_completed_items), NULL);
  gtk_menu_append (GTK_MENU (menu), i);
  gtk_widget_show (i);

  gtk_widget_show (menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (g_option), menu);
}

static void
new_todo_item (GtkWidget *w, gpointer user_data)
{
  GtkWidget *todo = edit_item (NULL, selected_category, GTK_WINDOW(window));

  gtk_widget_show_all (todo);
}


static void
delete_completed_items (GtkWidget *w, gpointer user_data)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                   GTK_DIALOG_MODAL 
                                   | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                   "Delete completed items?");

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
    {
      GSList *list, *iter;
    
      list = g_slist_copy (todo_db_get_items_list ());
    
      for (iter = list; iter; iter = iter->next)
        {
          struct todo_item *i = iter->data;
          gboolean complete;
        
          complete = (i->state == COMPLETED) ? TRUE : FALSE;
        
          if (complete && (selected_category == -1 ||
            g_slist_find (i->categories, (gpointer)selected_category)))
            {
              todo_db_delete_item (i);
            }
        }
    
      g_slist_free (list);
      refresh_items ();
    }

  gtk_widget_destroy (dialog);
}

void
open_editing_window (GtkTreePath *path)
{
  GtkTreeIter iter;
  struct todo_item *i;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 3, &i, -1);

  gtk_widget_show_all (edit_item (i, -1, GTK_WINDOW(window)));
}

void
toggle_completed (GtkTreePath *path)
{
  GtkTreeIter iter;
  struct todo_item *i;
  gboolean complete;
  gchar *priority, time[20] = {0};
  struct tm *ti;
  GdkPixbuf *sicon;
	  
  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 3, &i, -1);

  i->state = (i->state + 1) % 4;
  
  switch (i->state)
    {
    case IN_PROGRESS:
      sicon = dot_icon;
    break;
    case COMPLETED:
      sicon = tick_icon;
    break;
    case ABANDONED:
      sicon = bar_icon;
    break;
    default:
      sicon = no_tick_icon;
    break;
    }
  complete = ((i->state == COMPLETED) || (i->state == ABANDONED)) ? TRUE : FALSE;

  switch (i->priority)
    {
    case PRIORITY_STANDARD:
		priority = _("standard");
	break;
    case PRIORITY_HIGH:
		priority = _("high");
	break;
    case PRIORITY_LOW:
		priority = _("low");
	break;
    default:
		priority = _("none");
	break;
    }
	if (i->time)
      {		
        ti = localtime(&i->time);
        strftime(time, 20, "%x", ti);
	  }

    if (complete && !show_completed_tasks) {
       gtk_list_store_remove (list_store, &iter);
    } else {
       gtk_list_store_set (list_store, &iter, 
                           0, sicon,
                           1, i->summary,  
                           2, complete, 
                           3, i,
                           4, priority,
                           5, time,
                           -1);
    }


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

int
sort_more_complex (gconstpointer a, gconstpointer b)
{
  struct todo_item *ia, *ib;

  ia = (struct todo_item *)a;
  ib = (struct todo_item *)b;

  /* status */
  if ((ia->state >= COMPLETED) && (ib->state < COMPLETED))
      return 1;
  if ((ib->state >= COMPLETED) && (ia->state < COMPLETED))
      return -1;
  
  /* both due date: sort by date */
  if (ia->time && ib->time)
    {
      if (ia->time != ib->time)
        return ((ia->time - ib->time ) > 0 ? 1 : -1);
    }
    
  /* only one due date */  
  if (ia->time != ib->time)
     return ((ib->time - ia->time) > 0 ? 1 : -1);

  /* no due date: sort by priority */  
    
  return ib->priority - ia->priority;
}

void
refresh_items (void)
{
  GSList *list, *iter;
  GdkPixbuf *sicon;

  gtk_list_store_clear (list_store);

  list = g_slist_copy (todo_db_get_items_list ());

  list = g_slist_sort (list, sort_more_complex);

  for (iter = list; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;
      if (selected_category == -1
	  || g_slist_find (i->categories, (gpointer)selected_category))
	{
	  GtkTreeIter iter;
	  gboolean complete;
	  gchar *priority, time[20] = {0};
	  struct tm *ti;

	  switch (i->priority)
	    {
	    case PRIORITY_STANDARD:
			priority = _("standard");
		break;
	    case PRIORITY_HIGH:
			priority = _("high");
		break;
	    case PRIORITY_LOW:
			priority = _("low");
		break;
	    default:
			priority = _("none");
		break;
	    }
		
	  if (i->time)
	    {		  
	      ti = localtime(&i->time);
	      strftime(time, 20, "%x", ti);
		}
		
	  complete = ((i->state == COMPLETED) || (i->state == ABANDONED)) ? TRUE : FALSE;

      if (complete && !show_completed_tasks)
         continue;

      switch (i->state)
        {
        case IN_PROGRESS:
          sicon = dot_icon;
        break;
        case COMPLETED:
          sicon = tick_icon;
        break;
        case ABANDONED:
          sicon = bar_icon;
        break;
        default:
          sicon = no_tick_icon;
        break;
        }
        
	  gtk_list_store_append (list_store, &iter);

	  gtk_list_store_set (list_store, &iter, 
			      0, sicon,
			      1, i->summary,  
			      2, complete, 
			      3, i,
			      4, priority,
			      5, time,
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
  bar_icon = gpe_find_icon ("bar-box");
  dot_icon = gpe_find_icon ("dot-box");

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

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("Tap here to add a new item."), NULL,
			    G_CALLBACK (new_todo_item), NULL, -1);
                            
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
                            _("Delete completed items."), NULL,
                            G_CALLBACK (delete_completed_items), NULL, -1);
  
  if (mode_landscape || large_screen)
    gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_REFRESH,
                              _("Refresh list sorting"), NULL,
                              G_CALLBACK (refresh_items), NULL, -1);
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

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    _("Quit"), _("Tap here to quit the program."),
			    G_CALLBACK (gtk_main_quit), NULL, -1);

  /* Exit */
  gtk_box_pack_end (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  list_store = gtk_list_store_new (6, 
                                   GDK_TYPE_PIXBUF, 
                                   G_TYPE_STRING, 
 								   G_TYPE_BOOLEAN, 
								   G_TYPE_POINTER,
                                   G_TYPE_STRING, 
                                   G_TYPE_STRING);
 list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_pixbuf_new ();
    col = gtk_tree_view_column_new_with_attributes (_("Status"), renderer,
						    "pixbuf", 0,
						    NULL);

    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);

    g_object_set_data (G_OBJECT (list_view), "pixmap-column", col);

    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes (_("Summary"), renderer,
						    "text", 1,
						    "strikethrough", 2,
						    NULL);
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);

    if (gdk_screen_width() > 400)
      {
        renderer = gtk_cell_renderer_text_new ();
        col = gtk_tree_view_column_new_with_attributes (_("Priority"), renderer,
	                                                    "text", 4,
	                                                    "strikethrough", 2,
                                                          NULL);
        gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);
      }
	
    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes (_("Due Date"), renderer,
						    "text", 5,
						    "strikethrough", 2,
						    NULL);

    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);
	
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), TRUE);
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
