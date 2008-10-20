/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2005 - 2006 Florian Boor <florian@kernelconcepts.de>
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
#include <gdk/gdkkeysyms.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/pim-categories.h>
#include <gpe/colorrenderer.h>

/* Hildon includes */
#ifdef IS_HILDON

#if HILDON_VER > 0
#include <hildon/hildon-program.h>
#else
#include <hildon-widgets/hildon-program.h>
#endif /* HILDON_VER */

#include <gpe/pim-categories-ui.h>

#define ICON_PATH "/usr/share/icons/hicolor/26x26/hildon"

#endif

#include "todo.h"

#define _(_x) gettext(_x)

#define CONF_FILE_ "/.gpe-todo"
#define CONF_FILE() g_strdup_printf ("%s" CONF_FILE_, g_get_home_dir ())

static GtkWidget *g_option;
static gint selected_category = -1;
static gboolean show_completed_tasks = TRUE;

GtkListStore *list_store;
GtkWidget *item_menu;

GdkPixbuf *tick_icon, *no_tick_icon, *bar_icon, *dot_icon, *high_icon;

static struct todo_item *current_menu_item;
extern GtkWidget *window;
static GtkWidget *toolbar;
static GtkWidget *fullscreen_control;

static gchar *
build_categories_string (struct todo_item *item)
{
  gchar *s = NULL;
  GSList *iter;

  for (iter = item->categories; iter; iter = iter->next)
    {
      const gchar *cat;
      cat = gpe_pim_category_name ((gint)iter->data);

      if (cat)
        {
          if (s)
            {
              gchar *ns = g_strdup_printf ("%s, %s", s, cat);
              g_free (s);
              s = ns;
            }
          else
            s = g_strdup (cat);
	    }
    }

  return s;
}

static GArray *
build_categories_color_array (struct todo_item *item)
{
  GArray *result = NULL;
  GSList *iter;

  for (iter = item->categories; iter; iter = iter->next)
    {
      const gchar *col;
      col = gpe_pim_category_colour ((gint)iter->data);

      if (col)
        {
			GdkColor color;
			if (!result)
				result = g_array_new (TRUE, TRUE, sizeof (GdkColor));
			if (gdk_color_parse (col, &color) 
				&& gdk_colormap_alloc_color (gdk_colormap_get_system (),
                                             &color, FALSE, TRUE ))
			  {
				  g_array_append_val (result, color);
			  }
	    }
    }

  return result;
}

static void
show_no_item_selected_dialog(GtkWidget *w)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (GTK_WINDOW(gtk_widget_get_toplevel(w)),
                                   GTK_DIALOG_MODAL 
                                   | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                   _("You must select an item to edit."));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
item_do_edit (GtkWidget *w)
{
  if(current_menu_item)
    {
      gtk_widget_show_all (edit_item (current_menu_item, -1, GTK_WINDOW(window)));

      current_menu_item = NULL;
    }
  else
    {
      show_no_item_selected_dialog(w);
    }
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
  GSList *l = gpe_pim_categories_list (), *iter;
  GtkWidget *i;
  gboolean found;

  found = FALSE;

  i = gtk_menu_item_new_with_label (_("All items"));
  gtk_menu_append (GTK_MENU (menu), i);
  g_signal_connect (G_OBJECT (i), "activate", G_CALLBACK (set_category), (gpointer)-1);
  gtk_widget_show (i);

  for (iter = l; iter; iter = iter->next)
    {
      gint id = (gint) iter->data;
      i = gtk_menu_item_new_with_label (gpe_pim_category_name(id));
      gtk_menu_append (GTK_MENU (menu), i);
      g_signal_connect (G_OBJECT (i), "activate", G_CALLBACK (set_category), (gpointer)id);
      gtk_widget_show (i);

      if (id == selected_category)
        found = TRUE;
    }
  g_slist_free (l);

  if (!found && selected_category != -1)
    {
      selected_category = -1;
      refresh_items ();
    }

  i = gtk_separator_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), i);
  gtk_widget_show (i);

  i = gtk_check_menu_item_new_with_label (_("Show completed"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (i), 
                                  show_completed_tasks);
  g_signal_connect (G_OBJECT (i), "toggled", 
                    G_CALLBACK (toggle_completed_items), NULL);
  gtk_menu_append (GTK_MENU (menu), i);
  gtk_widget_show (i);

  gtk_widget_show (menu);
  if (gtk_option_menu_get_menu(GTK_OPTION_MENU (g_option)))
    {
      gtk_widget_destroy(gtk_option_menu_get_menu(GTK_OPTION_MENU (g_option)));
    }
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

#ifdef IS_HILDON
  dialog = gtk_message_dialog_new (GTK_WINDOW(gtk_widget_get_toplevel(w)),
                                   GTK_DIALOG_MODAL 
                                   | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                   "Are you sure you want to delete all " \
                                   "completed items permanently?");
  gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                         _("OK"), GTK_RESPONSE_YES,
                         _("Cancel"), GTK_RESPONSE_NO, NULL);
#else
  dialog = gtk_message_dialog_new (GTK_WINDOW(gtk_widget_get_toplevel(w)),
                                   GTK_DIALOG_MODAL 
                                   | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                   "Delete completed items?");
#endif  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
    {
      GSList *list, *iter;
    
      list = g_slist_copy (todo_db_get_items_list ());
    
      for (iter = list; iter; iter = iter->next)
        {
          struct todo_item *i = iter->data;
          gboolean complete;
        
          complete = ((i->state == COMPLETED) || (i->state == ABANDONED)) ? TRUE : FALSE;
        
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

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, COL_DATA, &i, -1);

  gtk_widget_show_all (edit_item (i, -1, 
                                  GTK_WINDOW(gtk_widget_get_toplevel(window))));
}

void
toggle_completed (GtkTreePath *path)
{
  GtkTreeIter iter;
  struct todo_item *i;
  gboolean complete;
  gchar time[20] = {0};
  struct tm *ti;
  GdkPixbuf *sicon;
  GdkPixbuf *priority;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path);

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, COL_DATA, &i, -1);

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
    case PRIORITY_HIGH:
        priority = high_icon;
    break;
    default:
        priority = NULL;
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
                           COL_ICON, sicon,
                           COL_SUMMARY, i->summary,  
                           COL_STRIKETHROUGH, complete, 
                           COL_DATA, i,
                           COL_PRIORITY, priority,
                           COL_PRIORITY_TEXT, _( state_map[i->state].string ),
                           COL_DUE, time,
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
  gchar *categories;
  GArray *colors;

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
          gchar time[20] = {0};
          GdkPixbuf *priority;
          struct tm *ti;
              
          switch (i->priority)
            {
            case PRIORITY_HIGH:
                priority = high_icon;
            break;
            default:
                priority = NULL;
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
            
          categories = build_categories_string(i);
          colors = build_categories_color_array(i);
            
          gtk_list_store_append (list_store, &iter);
    
          gtk_list_store_set (list_store, &iter, 
                              COL_ICON, sicon,
                              COL_SUMMARY, i->summary,  
                              COL_STRIKETHROUGH, complete, 
                              COL_DATA, i,
                              COL_PRIORITY, priority,
                              COL_PRIORITY_TEXT, _( state_map[i->state].string ),
                              COL_DUE, time,
                              COL_CATEGORY, categories,
							  COL_COLORS, colors,
                              -1);
          if (categories)
              g_free(categories);
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
          gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, COL_DATA, &i, -1);
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

#ifdef IS_HILDON
static void
toggle_toolbar(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  if (gtk_check_menu_item_get_active(menuitem))
    gtk_widget_show(toolbar);
  else
    gtk_widget_hide(toolbar);
}

static void
update_categories (GtkWidget *w, GSList *new, gpointer t)
{
  categories_menu ();
  if (new)
      g_slist_free(new);
}

static void
edit_categories (GtkWidget *w)
{
  GtkWidget *dialog;

  dialog = gpe_pim_categories_dialog (NULL, FALSE, 
                                      G_CALLBACK(update_categories), NULL);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), 
                               GTK_WINDOW(gtk_widget_get_toplevel(w)));
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
}

static void
toggle_fullscreen (GtkCheckMenuItem *menuitem, gpointer user_data)
{
  if (gtk_check_menu_item_get_active (menuitem))
    gtk_window_fullscreen (GTK_WINDOW (window));
  else
    gtk_window_unfullscreen (GTK_WINDOW (window));
}

/* create hildon application main menu */
static void
create_app_menu(HildonWindow *window)
{
  GtkMenu *menu_main = GTK_MENU (gtk_menu_new());
  GtkWidget *menu_items = gtk_menu_new();
  GtkWidget *menu_categories = gtk_menu_new();
  GtkWidget *menu_view = gtk_menu_new();
  GtkWidget *menu_tools = gtk_menu_new();
    
  GtkWidget *item_items = gtk_menu_item_new_with_label(_("Item"));
  GtkWidget *item_categories = gtk_menu_item_new_with_label(_("Categories"));
  GtkWidget *item_view = gtk_menu_item_new_with_label(_("View"));
  GtkWidget *item_tools = gtk_menu_item_new_with_label(_("Tools"));
  GtkWidget *item_close = gtk_menu_item_new_with_label(_("Close"));
  GtkWidget *item_open = gtk_menu_item_new_with_label(_("Open..."));
  GtkWidget *item_add = gtk_menu_item_new_with_label(_("Add new"));
  GtkWidget *item_catedit = gtk_menu_item_new_with_label(_("Edit categories"));
  GtkWidget *item_toolbar = gtk_check_menu_item_new_with_label(_("Show toolbar"));
  GtkWidget *item_fullscreen = gtk_check_menu_item_new_with_label(_("Fullscreen"));
  GtkWidget *item_delete = gtk_menu_item_new_with_label(_("Delete completed items"));
  GtkWidget *item_move = gtk_menu_item_new_with_label(_("Move completed items to the end"));

  gtk_menu_append (GTK_MENU(menu_items), item_open);
  gtk_menu_append (GTK_MENU(menu_items), item_add);
  gtk_menu_append (GTK_MENU(menu_categories), item_catedit);
  gtk_menu_append (GTK_MENU(menu_view), item_fullscreen);
  gtk_menu_append (GTK_MENU(menu_view), item_toolbar);
  gtk_menu_append (GTK_MENU(menu_tools), item_delete);
  gtk_menu_append (GTK_MENU(menu_tools), item_move);
  gtk_menu_append (menu_main, item_items);
  gtk_menu_append (menu_main, item_categories);
  gtk_menu_append (menu_main, item_view);
  gtk_menu_append (menu_main, item_tools);
  gtk_menu_append (menu_main, item_close);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_items), menu_items);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_categories), menu_categories);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_view), menu_view);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_tools), menu_tools);

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_toolbar), TRUE);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_fullscreen), FALSE);

  g_signal_connect(G_OBJECT(item_add), "activate", G_CALLBACK(new_todo_item), NULL);
  g_signal_connect(G_OBJECT(item_open), "activate", G_CALLBACK(item_do_edit), NULL);
  g_signal_connect(G_OBJECT(item_catedit), "activate", G_CALLBACK(edit_categories), NULL);
  g_signal_connect(G_OBJECT(item_fullscreen), "activate", G_CALLBACK(toggle_fullscreen), NULL);
  g_signal_connect(G_OBJECT(item_toolbar), "activate", G_CALLBACK(toggle_toolbar), NULL);
  g_signal_connect(G_OBJECT(item_delete), "activate", G_CALLBACK(delete_completed_items), NULL);
  g_signal_connect(G_OBJECT(item_move), "activate", G_CALLBACK(refresh_items), NULL);
  g_signal_connect(G_OBJECT(item_close), "activate", G_CALLBACK(gpe_todo_exit), NULL);

  gtk_widget_show_all (GTK_WIDGET(menu_main));
  fullscreen_control = item_fullscreen;
  hildon_window_set_menu (HILDON_WINDOW (window), menu_main);
}
#endif

static gboolean
window_key_press_event (GtkWidget *window, GdkEventKey *k, GtkWidget *data)
{
    switch (k->keyval)
      {
#ifdef IS_HILDON
      case GDK_F6:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (fullscreen_control), 
             !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (fullscreen_control)));
        return TRUE;
      break;
#endif
      }
    return FALSE;
}

void
selection_changed_event(GtkTreeSelection *treeselection,
			 gpointer user_data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
    
  if(gtk_tree_selection_get_selected(treeselection, &model, &iter))
    {
      struct todo_item *i;
      gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, COL_DATA, &i, -1);
      current_menu_item = i;
    }
  else
    {
      current_menu_item = NULL;
    }    
} 

void                
row_activated_event(GtkTreeView *tree_view,
		    GtkTreePath *path,
                    GtkTreeViewColumn *column,
                    gpointer user_data)
{
  open_editing_window(path);
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
  GtkWidget *option = gtk_option_menu_new ();
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *list_view;
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkToolItem *item;
  GtkWidget *w;

  no_tick_icon = gpe_find_icon ("notick-box");
  tick_icon = gpe_find_icon ("tick-box");
  bar_icon = gpe_find_icon ("bar-box");
  dot_icon = gpe_find_icon ("dot-box");
  high_icon = gpe_find_icon ("high");

  g_option = option;
  categories_menu ();

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", accel_group);
  g_object_set_data_full (G_OBJECT (window), "<main>", item_factory,
                          (GDestroyNotify) g_object_unref);
  gtk_item_factory_create_items (item_factory, 
                                 sizeof (menu_items) / sizeof (menu_items[0]), 
                                 menu_items, NULL);

  item_menu = gtk_item_factory_get_widget (item_factory, "<main>");

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  /* New button */
#ifdef IS_HILDON  
  w = gtk_image_new_from_file(ICON_PATH "/qgn_indi_gene_plus.png");
  item = gtk_tool_button_new(w, _("New"));
#else
  item = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
#endif
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK (new_todo_item), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  /* Delete button */
#ifdef IS_HILDON  
  w = gtk_image_new_from_file(ICON_PATH "/qgn_toolb_gene_deletebutton.png");
  item = gtk_tool_button_new(w, _("Delete"));
#else
  item = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
#endif
  g_signal_connect(G_OBJECT(item), "clicked", 
                   G_CALLBACK (delete_completed_items), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
#ifdef IS_HILDON
  create_app_menu (HILDON_WINDOW (window));
  
  w = gtk_image_new_from_file(ICON_PATH "/qgn_toolb_gene_refresh.png");
  item = gtk_tool_button_new(w, _("Refresh"));
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK (refresh_items), 
                   NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	  
  if (mode_landscape || large_screen)
    {
      w = gtk_image_new_from_file(ICON_PATH "/qgn_list_gene_bullets.png");
      item = gtk_tool_button_new(w, _("Categories"));
      g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK (edit_categories), 
                       NULL);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
    
  hildon_window_add_toolbar(HILDON_WINDOW(window), GTK_TOOLBAR(toolbar));
#else
  /* Insert refresh button if we have enough space */
  if (mode_landscape || large_screen)
    {
      item = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
      g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK (refresh_items), 
                       NULL);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);        
    }
	
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
#endif
    
  item = gtk_separator_tool_item_new();
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  if (mode_landscape || large_screen)
    {
      GtkWidget *label = gtk_label_new(_("Show:"));
      item = gtk_tool_item_new();
      gtk_container_add(GTK_CONTAINER(item), label);
      gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
    
  /* Category menu */    
  item = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(item), option);
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

#ifndef IS_HILDON    
  /* Some space and quit button */
  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), TRUE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  item = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK (gpe_todo_exit), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
#endif
    
  list_store = gtk_list_store_new (NUM_COLUMNS, 
                                   GDK_TYPE_PIXBUF, 
                                   G_TYPE_STRING, 
 								   G_TYPE_BOOLEAN, 
								   G_TYPE_POINTER,
                                   G_TYPE_STRING, 
                                   G_TYPE_STRING, 
                                   G_TYPE_STRING, 
                                   GDK_TYPE_PIXBUF,
                                   G_TYPE_STRING, 
								   G_TYPE_POINTER);
  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_pixbuf_new ();
    col = gtk_tree_view_column_new_with_attributes ("", renderer,
                                                    "pixbuf", COL_PRIORITY,
                                                    NULL);
    gtk_tree_view_column_set_min_width(col, 20);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);

    renderer = gtk_cell_renderer_pixbuf_new ();
    col = gtk_tree_view_column_new_with_attributes ("", renderer,
                                                    "pixbuf", COL_ICON, 
                                                    NULL);
    gtk_tree_view_column_set_min_width(col, 20);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);

    g_object_set_data (G_OBJECT (list_view), "pixmap-column", col);

   if (large_screen)
      {
        renderer = gtk_cell_renderer_text_new ();
        col = gtk_tree_view_column_new_with_attributes (_("Status"), renderer,
                                                        "text", COL_PRIORITY_TEXT,
                                                        "strikethrough", COL_STRIKETHROUGH, NULL);
        gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);
      }
    renderer = gtk_cell_renderer_text_new ();
    col = gtk_tree_view_column_new_with_attributes (_("Summary"), renderer,
                                                    "text", COL_SUMMARY,
                                                    "strikethrough", COL_STRIKETHROUGH, NULL);
    gtk_tree_view_column_set_expand(col, TRUE);
#ifdef IS_HILDON	  
	gtk_tree_view_column_set_max_width(col, 360);
#endif	  
    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);
	
    if (large_screen)
      {
        renderer = gtk_cell_renderer_text_new ();
        col = gtk_tree_view_column_new_with_attributes (_("Due Date"), renderer,
                                                        "text", COL_DUE,
                                                        "strikethrough", 
		                                                COL_STRIKETHROUGH, NULL);
        gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);
      }
      
    renderer = cell_renderer_color_new ();
    /* xgettext: TRANSLATORS: "C" in the next line is an abbreviation for "Colour", 
       please select an appropriate 1- or 2- character label for your own language.  */
    col = gtk_tree_view_column_new_with_attributes (_("C"), renderer,
                                                    "colorlist", COL_COLORS, NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);
          
    if (large_screen)
      {
        renderer = gtk_cell_renderer_text_new ();
        col = gtk_tree_view_column_new_with_attributes (_("Category"), renderer,
                                                        "text", COL_CATEGORY,
                                                        "strikethrough", COL_STRIKETHROUGH, NULL);
        gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);

      }
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), TRUE);
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (list_view), FALSE);
  }

  g_signal_connect (G_OBJECT (list_view), "button_press_event", 
                    G_CALLBACK (button_press_event), NULL);
  g_signal_connect (G_OBJECT (list_view), "button_release_event", 
                    G_CALLBACK (button_release_event), NULL);
  g_signal_connect (G_OBJECT (window), "key-press-event", 
                    G_CALLBACK (window_key_press_event), NULL);
  g_signal_connect (G_OBJECT (list_view), "row_activated",
		    G_CALLBACK (row_activated_event), NULL);
  GtkTreeSelection *selection =  gtk_tree_view_get_selection(GTK_TREE_VIEW(list_view));
  if(selection)
    g_signal_connect (G_OBJECT(selection), "changed",
		      G_CALLBACK(selection_changed_event), NULL);

  gtk_container_add (GTK_CONTAINER (scrolled), list_view);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  refresh_items ();

  return vbox;
}

void
conf_read (void)
{
  /* Read the configuration file.  */
  GKeyFile *conf = g_key_file_new ();
  char *filename = CONF_FILE ();
  if (g_key_file_load_from_file (conf, filename, 0, NULL))
    {
      gboolean b;
      GError *error = NULL;
      b = g_key_file_get_boolean (conf, "gpe-todo",
				 "show_completed_tasks", &error);
      if (error)
	{
	  g_error_free (error);
	  error = NULL;
	}
      else
	show_completed_tasks = b;

    }
  g_free (filename);
  g_key_file_free (conf);
}

/* Write the configuration file.  */
void
conf_write (void)
{
  GKeyFile *conf = g_key_file_new ();
  char *filename = CONF_FILE ();
  g_key_file_load_from_file (conf, filename, G_KEY_FILE_KEEP_COMMENTS, NULL);
  g_free (filename);

  g_key_file_set_boolean (conf, "gpe-todo", "show_completed_tasks", 
			  show_completed_tasks);

  gsize length;
  char *data = g_key_file_to_data (conf, &length, NULL);
  g_key_file_free (conf);
  if (data)
    {
      char *filename = CONF_FILE ();
      FILE *f = fopen (filename, "w");
      g_free (filename);
      if (f)
	{
	  fwrite (data, length, 1, f);
	  fclose (f);
	}
      g_free (data);
    }
}
