/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
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

#include <libdm.h>

#include "todo.h"

#define _(_x) gettext(_x)

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

static void
state_func_0(GtkMenuItem *w, gpointer p)
{
  struct edit_todo *t = p;
  t->state = NOT_STARTED;
}

static void
state_func_1(GtkMenuItem *w, gpointer p)
{
  struct edit_todo *t = p;
  t->state = IN_PROGRESS;
}

static void
state_func_2(GtkMenuItem *w, gpointer p)
{
  struct edit_todo *t = p;
  t->state = COMPLETED;
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
  GtkWidget *window = GTK_WIDGET (p);
  GSList *list = g_object_get_data (G_OBJECT (window), "widgets");
  struct edit_todo *t = g_object_get_data (G_OBJECT (window), "edit_data");

  g_slist_free (t->selected_categories);
  t->selected_categories = NULL;

  for (; list; list = list->next)
    {
      GtkWidget *w = list->data;
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
	{
	  struct todo_category *c = g_object_get_data (G_OBJECT (w), "category");
	  t->selected_categories = g_slist_append (t->selected_categories, c);
	}
    }

  gtk_label_set_text (GTK_LABEL (t->categories_label),
				 build_categories_string (t->selected_categories));

  gtk_widget_destroy (window);
}

static void
change_categories (GtkWidget *w, gpointer p)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
  GSList *iter;
  struct edit_todo *t = (struct edit_todo *)p;
  GSList *widgets = NULL;
  GtkWidget *okbutton, *cancelbutton;

  for (iter = todo_db_get_categories_list(); iter; iter = iter->next)
    {
      struct todo_category *c = iter->data;
      GtkWidget *w = gtk_check_button_new_with_label (c->title);

      if (g_slist_find (t->selected_categories, c))
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);

      gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

      g_object_set_data (G_OBJECT (w), "category", c);
      widgets = g_slist_append (widgets, w);
    }

  g_object_set_data (G_OBJECT (window), "widgets", widgets);
  g_object_set_data (G_OBJECT (window), "edit_data", t);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), vbox);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), sw, TRUE, TRUE, 0);

  gpe_set_window_icon (window, "icon");

  okbutton = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  cancelbutton = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

  g_signal_connect (G_OBJECT (okbutton), "clicked", G_CALLBACK (categories_ok), window);
  g_signal_connect (G_OBJECT (cancelbutton), "clicked", G_CALLBACK (categories_cancel), window);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancelbutton, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), okbutton, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (okbutton);
  
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);
  gtk_window_set_title (GTK_WINDOW (window), _("Select categories"));

  gtk_widget_show_all (window);
}

/*
 * Pass item=NULL to create a new item;
 * if item=NULL, you may pass the item's initial category.
 */
GtkWidget *
edit_item (struct todo_item *item, struct todo_category *initial_category)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *text = gtk_text_view_new ();
  GtkWidget *duebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok;
  GtkWidget *buttoncancel;
    GtkWidget *buttondelete;
  GtkWidget *state = gtk_option_menu_new ();
  GtkWidget *state_menu = gtk_menu_new ();
  GtkWidget *label_summary = gtk_label_new (_("Summary:"));
  GtkWidget *label_details = gtk_label_new (_("Details:"));
  GtkWidget *entry_summary = gtk_entry_new ();
  GtkWidget *hbox_summary = gtk_hbox_new (FALSE, 0);
  GtkWidget *hbox_categories;
  struct edit_todo *t = g_malloc (sizeof (struct edit_todo));

  const char *state_strings[] = { _("Not started"), _("In progress"),
				  _("Completed") };
  void (*state_funcs[3])(GtkMenuItem *, gpointer) = 
    { state_func_0, state_func_1, state_func_2 };
  guint i;
  struct tm tm;
  time_t the_time;

  libdm_mark_window (window);

  buttonok = gpe_button_new_from_stock (GTK_STOCK_SAVE, GPE_BUTTON_TYPE_BOTH);
  buttoncancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);
  buttondelete = gpe_button_new_from_stock (GTK_STOCK_DELETE, GPE_BUTTON_TYPE_BOTH);
 
  for (i = 0; i < 3; i++)
    {
      GtkWidget *l = gtk_menu_item_new_with_label (state_strings[i]);
      gtk_menu_append (GTK_MENU (state_menu), l);
      gtk_signal_connect (GTK_OBJECT (l), "activate", (GtkSignalFunc)state_funcs[i], t);
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

  if (todo_db_get_categories_list())
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

  gtk_box_pack_start (GTK_BOX (vbox), hbox_summary, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), duebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), state, FALSE, FALSE, 2);

  if (todo_db_get_categories_list())
    gtk_box_pack_start (GTK_BOX (vbox), hbox_categories, FALSE, FALSE, 2);
  
  gtk_misc_set_alignment (GTK_MISC (label_details), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label_details, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), text, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), buttonbox, FALSE, FALSE, 2);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), TRUE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text), GTK_WRAP_WORD);
  gtk_widget_set_usize (text, -1, 88);

  gtk_container_set_border_width (GTK_CONTAINER (window),
				  gpe_get_border ());
  gtk_container_add (GTK_CONTAINER (window), vbox);

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

  return window;
}
