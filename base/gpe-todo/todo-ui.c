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
#include <gpe/pim-categories.h>
#include <gpe/pim-categories-ui.h>

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
  t->item->priority = t->priority;
  todo_db_push_item (t->item);

  refresh_items ();

  gtk_widget_destroy (window);
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


static gchar *
build_categories_string (struct edit_todo *t)
{
  gchar *s = NULL;
  GSList *iter;

  for (iter = t->selected_categories; iter; iter = iter->next)
    {
      const gchar *cat;
      cat = gpe_pim_category_name ((int)iter->data);

      if (cat)
	{
	  if (s)
	    {
	      char *ns = g_strdup_printf ("%s, %s", s, cat);
	      g_free (s);
	      s = ns;
	    }
	  else
	    s = g_strdup (cat);
	}
    }

  return s;
}

static void
update_categories (GtkWidget *w, GSList *new, struct edit_todo *t)
{
  gchar *str;

  g_slist_free (t->selected_categories);

  t->selected_categories = g_slist_copy (new);

  str = build_categories_string (t);
  gtk_label_set_text (GTK_LABEL (t->categories_label), str);
  g_free (str);

  categories_menu ();
}

static void
change_categories (GtkWidget *w, struct edit_todo *t)
{
  gpe_pim_categories_dialog (t->selected_categories, G_CALLBACK (update_categories), t);
}

/*
 * Pass item=NULL to create a new item;
 * if item=NULL, you may pass the item's initial category.
 */
GtkWidget *
edit_item (struct todo_item *item, gint initial_category)
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
      if (initial_category != -1)
	t->selected_categories = g_slist_append (NULL, (gpointer)initial_category);
      else
	t->selected_categories = NULL;
    }

  {
    GtkWidget *button = gtk_button_new_with_label (_("Categories:"));
    GtkWidget *label = gtk_label_new ("");
    gchar *s = NULL;

    s = build_categories_string (t);
    
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

  gtk_box_pack_start (GTK_BOX (hbox_state), state, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_state), priority_label, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox_state), priority_optionmenu, FALSE, FALSE, 0);

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
      int prio_level = 1;
      if (item->what)
	gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (text)), 
				  item->what, -1);
      if (item->summary)
	gtk_entry_set_text (GTK_ENTRY (entry_summary), item->summary);
      gtk_option_menu_set_history (GTK_OPTION_MENU (state), item->state);
      if (item->priority > PRIORITY_STANDARD)
	prio_level = 0;
      else if (item->priority < PRIORITY_STANDARD)
	prio_level = 2;
      gtk_option_menu_set_history (GTK_OPTION_MENU (priority_optionmenu), prio_level);
      t->state = item->state;
      t->priority = item->priority;

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
      t->priority = PRIORITY_STANDARD;
      gtk_option_menu_set_history (GTK_OPTION_MENU (priority_optionmenu), 1);
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
