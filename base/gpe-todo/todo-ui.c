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
#include <string.h>

#include <gtk/gtk.h>

#include <gpe/gtkdatecombo.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/spacing.h>
#include <gpe/errorbox.h>

#include <libdisplaymigration/displaymigration.h>

#include "todo.h"

#define _(_x) gettext(_x)
#define N_(_x) (_x)

struct category_map
{
  GtkWidget *w;
  struct todo_category *c;
};

struct edit_todo
{
  GtkWidget *text;
  GtkWidget *summary;
  GtkWidget *duetoggle;
  GtkWidget *duedate;

  struct todo_item *item;

  item_state state;
  guint priority;
  
  GSList *selected_categories;
  GtkWidget *categories_label;
};

static void
destroy_user_data (gpointer p)
{
  struct edit_todo *t = (struct edit_todo *)p;
  if (t->selected_categories)
    g_slist_free (t->selected_categories);
  g_free (p);
}

static void
due_toggle_clicked (GtkWidget *widget, struct edit_todo *t)
{
  gtk_widget_set_sensitive (t->duedate, 
		    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
						  (t->duetoggle)));
}

static void
click_cancel (GtkWidget *widget,
	      GtkWidget *window)
{
  gtk_widget_destroy (window);
}

static void
click_delete (GtkWidget *widget,
	      GtkWidget *window)
{
  struct edit_todo *t = gtk_object_get_data (GTK_OBJECT (window), "todo");
  
  if (t->item)
    todo_db_delete_item (t->item);

  refresh_items ();
  
  gtk_widget_destroy (window);
}

static void
click_ok (GtkWidget *widget,
	  GtkWidget *window)
{
  struct edit_todo *t = gtk_object_get_data (GTK_OBJECT (window), "todo");

  time_t when;
  struct tm tm;
  const char *what, *summary;
  GtkTextBuffer *buf;
  GtkTextIter start, end;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t->duetoggle)))
    {
      memset (&tm, 0, sizeof(tm));
      tm.tm_year = GTK_DATE_COMBO (t->duedate)->year - 1900;
      tm.tm_mon = GTK_DATE_COMBO (t->duedate)->month;
      tm.tm_mday = GTK_DATE_COMBO (t->duedate)->day;
      when = mktime (&tm);
    }
  else
    when = (time_t)0;

  buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (t->text));
  gtk_text_buffer_get_bounds (buf, &start, &end);
  what = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buf), &start, &end, FALSE);
  summary = gtk_editable_get_chars (GTK_EDITABLE (t->summary), 0, -1);

  if (t->item)
    {
      if (t->item->what)
	{
	  g_free ((char *)t->item->what);
	  t->item->what = NULL;
	}
      if (t->item->summary)
	{
	  g_free ((char *)t->item->summary);
	  t->item->summary = NULL;
	}
    }
  else
    t->item = todo_db_new_item ();

  t->item->categories = g_slist_copy (t->selected_categories);

  if (what[0])
    t->item->what = what;
  if (summary[0])
    t->item->summary = summary;
  t->item->time = when;
  t->item->state = t->state;
  todo_db_push_item (t->item);

  refresh_items ();

  gtk_widget_destroy (window);
}

static gchar *
build_categories_string (GSList *iter)
{
  gchar *s = NULL;

  for (; iter; iter = iter->next)
    {
      struct todo_category *c = iter->data;
      if (s)
	{
	  char *ns = g_strdup_printf ("%s, %s", s, c->title);
	  g_free (s);
	  s = ns;
	}
      else
	s = g_strdup (c->title);
    }

  return s;
}

static void
categories_cancel (GtkWidget *w, gpointer p)
{
  GtkWidget *window = GTK_WIDGET (p);

  gtk_widget_destroy (window);
}

static void
categories_ok (GtkWidget *w, gpointer p)
{
  GtkWidget *window;
  struct edit_todo *t;
  GtkTreeIter iter;
  GtkListStore *list_store;
  GSList *old_categories, *i;

  window = GTK_WIDGET (p);
  t = g_object_get_data (G_OBJECT (window), "edit_data");
  list_store = g_object_get_data (G_OBJECT (window), "list_store");

  g_slist_free (t->selected_categories);
  t->selected_categories = NULL;

  old_categories = g_slist_copy (todo_db_get_categories_list ());

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      do 
	{
	  struct todo_category *c;
	  gchar *title;
	  gboolean selected;
	  
	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &selected, 1, &title, 2, &c, -1);

	  if (c == NULL)
	    c = todo_db_new_category (title);
	  else
	    {
	      old_categories = g_slist_remove (old_categories, c);

	      if (strcmp (c->title, title))
		{
		  g_free ((void *)c->title);
		  c->title = g_strdup (title);
		}
	    }

	  if (selected)
	    t->selected_categories = g_slist_append (t->selected_categories, c);
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
    }

  for (i = old_categories; i; i = i->next)
    {
      struct todo_category *c = i->data;

      todo_db_del_category (c);
    }

  g_slist_free (old_categories);

  gtk_label_set_text (GTK_LABEL (t->categories_label),
				 build_categories_string (t->selected_categories));

  categories_menu ();

  gtk_widget_destroy (window);
}

void
do_new_category (GtkWidget *widget, GtkWidget *d)
{
  GtkWidget *entry = gtk_object_get_data (GTK_OBJECT (d), "entry");
  char *title = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  GSList *l;
  GtkTreeIter iter;
  GtkListStore *list_store;

  list_store = g_object_get_data (G_OBJECT (d), "list-store");

  if (title[0] == 0)
    {
      gpe_error_box (_("Category name must not be blank"));
      gtk_widget_destroy (d);
      return;
    }
  
  for (l = todo_db_get_categories_list (); l; l = l->next)
    {
      struct todo_category *t = l->data;
      if (!strcmp (title, t->title))
	{
	  gpe_error_box (_("A category by that name already exists"));
	  gtk_widget_destroy (d);
	  return;
	}
    }

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, FALSE, 1, title, 2, NULL, -1);

  gtk_widget_destroy (d);
}

void
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

  displaymigration_mark_window (window);

  vbox = GTK_DIALOG (window)->vbox;

  hbox = gtk_hbox_new (FALSE, 0);

  label = gtk_label_new (_("Name:"));
  name = gtk_entry_new ();
  
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), name, TRUE, TRUE, 2);

  ok = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, 
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, 
		      FALSE, FALSE, 0);

  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok);

  g_signal_connect (G_OBJECT (ok), "clicked", 
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

void
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

void
change_categories (GtkWidget *w, gpointer p)
{
  GtkWidget *toolbar;
  GtkWidget *window;
  GtkWidget *sw;
  GSList *iter;
  struct edit_todo *t = (struct edit_todo *)p;
  GtkWidget *okbutton, *cancelbutton;
  GtkListStore *list_store;
  GtkWidget *tree_view;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;

  window = gtk_dialog_new ();

  list_store = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

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

  sw = gtk_scrolled_window_new (NULL, NULL);

  for (iter = todo_db_get_categories_list(); iter; iter = iter->next)
    {
      struct todo_category *c = iter->data;
      GtkTreeIter iter;
      gboolean selected = FALSE;

      gtk_list_store_append (list_store, &iter);

      if (g_slist_find (t->selected_categories, c))
	selected = TRUE;
      
      gtk_list_store_set (list_store, &iter, 0, selected, 1, c->title, 2, c, -1);
    }

  renderer = gtk_cell_renderer_toggle_new ();
  g_object_set (G_OBJECT (renderer), "activatable", TRUE, NULL);
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						  "active", 0,
						  NULL);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);

  g_object_set_data (G_OBJECT (tree_view), "toggle-col", col);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						  "text", 1,
						  NULL);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  g_object_set_data (G_OBJECT (window), "edit_data", t);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);

  g_signal_connect (G_OBJECT (tree_view), "button_press_event", 
		    G_CALLBACK (categories_button_press_event), list_store);

  gtk_container_add (GTK_CONTAINER (sw), tree_view);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), sw, TRUE, TRUE, 0);

  gpe_set_window_icon (window, "icon");

  okbutton = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  cancelbutton = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

  g_object_set_data (G_OBJECT (window), "list_store", list_store);

  g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (categories_ok), window);
  g_signal_connect (G_OBJECT (cancelbutton), "clicked", G_CALLBACK (categories_cancel), window);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancelbutton, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (okbutton);
  
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);
  gtk_window_set_title (GTK_WINDOW (window), _("Select categories"));

  g_signal_connect_swapped (G_OBJECT (window), "destroy", G_CALLBACK (g_object_unref), list_store);

  gtk_widget_show_all (window);
}

struct menu_map
{
  gchar *string;
  gint value;
};

struct menu_map
state_map[] =
  {
    { N_("Not started"),	NOT_STARTED },
    { N_("In progress"),	IN_PROGRESS },
    { N_("Completed"),		COMPLETED },
    { N_("Abandoned"),		ABANDONED },
  };

struct menu_map
priority_map[] = 
  {
    { N_("High"),		PRIORITY_HIGH },
    { N_("Standard"),		PRIORITY_STANDARD },
    { N_("Low"),		PRIORITY_LOW },
  };

static void
set_item_state (GtkMenuItem *w, gpointer p)
{
  struct edit_todo *t = p;
  struct menu_map *m;

  m = g_object_get_data (G_OBJECT (w), "menu-map");
  t->state = m->value;
}

static void
set_item_priority (GtkMenuItem *w, gpointer p)
{
  struct edit_todo *t = p;
  struct menu_map *m;

  m = g_object_get_data (G_OBJECT (w), "menu-map");
  t->priority = m->value;
}

/*
 * Pass item=NULL to create a new item;
 * if item=NULL, you may pass the item's initial category.
 */
GtkWidget *
edit_item (struct todo_item *item, struct todo_category *initial_category)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *top_vbox;
  GtkWidget *vbox;
  GtkWidget *text = gtk_text_view_new ();
  GtkWidget *duebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok;
  GtkWidget *buttoncancel;
  GtkWidget *buttondelete;
  GtkWidget *priority_label, *priority_menu, *priority_optionmenu;
  GtkWidget *hbox_state;
  GtkWidget *state = gtk_option_menu_new ();
  GtkWidget *state_menu;
  GtkWidget *label_summary = gtk_label_new (_("Summary:"));
  GtkWidget *label_details = gtk_label_new (_("Details:"));
  GtkWidget *entry_summary = gtk_entry_new ();
  GtkWidget *hbox_summary = gtk_hbox_new (FALSE, 0);
  GtkWidget *hbox_categories = NULL;
  GtkWidget *scrolled_window;
  struct edit_todo *t;
  guint i;
  struct tm tm;
  time_t the_time;

  t = g_malloc (sizeof (struct edit_todo));

  displaymigration_mark_window (window);

  top_vbox = gtk_vbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  buttonok = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  buttondelete = gtk_button_new_from_stock (GTK_STOCK_DELETE);

  state_menu = gtk_menu_new ();
  for (i = 0; i < sizeof (state_map) / sizeof (state_map[0]); i++)
    {
      GtkWidget *l = gtk_menu_item_new_with_label (gettext (state_map[i].string));
      g_object_set_data (G_OBJECT (l), "menu-map", &state_map[i]);
      gtk_menu_append (GTK_MENU (state_menu), l);
      g_signal_connect (G_OBJECT (l), "activate", G_CALLBACK (set_item_state), t);
    }
		   
  priority_menu = gtk_menu_new ();
  for (i = 0; i < sizeof (priority_map) / sizeof (priority_map[0]); i++)
    {
      GtkWidget *l = gtk_menu_item_new_with_label (gettext (priority_map[i].string));
      g_object_set_data (G_OBJECT (l), "menu-map", &priority_map[i]);
      gtk_menu_append (GTK_MENU (priority_menu), l);
      g_signal_connect (G_OBJECT (l), "activate", G_CALLBACK (set_item_priority), t);
    }

  t->duetoggle = gtk_check_button_new_with_label (_("Due:"));
  t->duedate = gtk_date_combo_new ();

  if (item)
    t->selected_categories = g_slist_copy (item->categories);
  else
    {
      if (initial_category)
	t->selected_categories = g_slist_append (NULL, initial_category);
      else
	t->selected_categories = NULL;
    }

  {
    GtkWidget *button = gtk_button_new_with_label (_("Categories:"));
    GtkWidget *label = gtk_label_new ("");
    gchar *s = NULL;

    s = build_categories_string (t->selected_categories);
    
    if (s)
      {
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);
      }
    
    gtk_widget_show (label);
    gtk_widget_show (button);
    hbox_categories = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox_categories);
    gtk_box_pack_start (GTK_BOX (hbox_categories), button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox_categories), label, TRUE, TRUE, 4);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (change_categories), t);
    
    t->categories_label = label;
  }

  t->item = item;
  
  gtk_box_pack_start (GTK_BOX (duebox), t->duetoggle, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (duebox), t->duedate, TRUE, TRUE, 1);

  gtk_signal_connect (GTK_OBJECT (t->duetoggle), "clicked",
		      GTK_SIGNAL_FUNC (due_toggle_clicked), t);
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);
  gtk_signal_connect (GTK_OBJECT (buttondelete), "clicked",
		      GTK_SIGNAL_FUNC (click_delete), window);

  gtk_box_pack_start (GTK_BOX (buttonbox), buttondelete, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttoncancel, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttonok, TRUE, FALSE, 4);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (state), state_menu);

  gtk_box_pack_start (GTK_BOX (hbox_summary), label_summary, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox_summary), entry_summary, TRUE, TRUE, 2);

  hbox_state = gtk_hbox_new (FALSE, 0);

  priority_label = gtk_label_new (_("Priority:"));
  priority_optionmenu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (priority_optionmenu), priority_menu);

  gtk_box_pack_start (GTK_BOX (hbox_state), state, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox_state), priority_label, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox_state), priority_optionmenu, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox_summary, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), duebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox_state, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox_categories, FALSE, FALSE, 2);
  
  gtk_misc_set_alignment (GTK_MISC (label_details), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label_details, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), text, TRUE, TRUE, 2);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), TRUE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text), GTK_WRAP_WORD);
  gtk_widget_set_usize (text, -1, 88);

  gtk_container_set_border_width (GTK_CONTAINER (window),
				  gpe_get_border ());
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);

  gtk_box_pack_start (GTK_BOX (top_vbox), scrolled_window, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (top_vbox), buttonbox, FALSE, FALSE, 2);

  gtk_container_add (GTK_CONTAINER (window), top_vbox);

  gtk_widget_grab_focus (entry_summary);

  the_time = time (NULL);

  if (item)
    {
      if (item->what)
	gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (text)), 
				  item->what, -1);
      if (item->summary)
	gtk_entry_set_text (GTK_ENTRY (entry_summary), item->summary);
      gtk_option_menu_set_history (GTK_OPTION_MENU (state), item->state);
      t->state = item->state;

      if (item->time)
	{
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (t->duetoggle), 
					TRUE);
	  the_time = item->time;
	}
      else
	gtk_widget_set_sensitive (t->duedate, FALSE);

      gtk_window_set_title (GTK_WINDOW (window), _("Edit to-do item"));
    }
  else
    {
      t->state = NOT_STARTED;
      gtk_widget_set_sensitive (buttondelete, FALSE);
      gtk_widget_set_sensitive (t->duedate, FALSE);
      gtk_window_set_title (GTK_WINDOW (window), _("New item"));
    }

  t->text = text;
  t->summary = entry_summary;

  localtime_r (&the_time, &tm);
  gtk_date_combo_set_date (GTK_DATE_COMBO (t->duedate),
			   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
  
  gtk_object_set_data_full (GTK_OBJECT (window), "todo", t, destroy_user_data);

  gpe_set_window_icon (window, "icon");

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);

  return window;
}

