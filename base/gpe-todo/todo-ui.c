/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *               2005, 2006, 2008 Florian Boor <florian@kernelconcepts.de>
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
    { N_("Standard"),	PRIORITY_STANDARD },
    { N_("Low"),		PRIORITY_LOW },
  };

static void
set_item_state (GtkComboBox *w, gpointer p)
{
  struct edit_todo *t = p;
  int i;
  gchar *name;

  name = gtk_combo_box_get_active_text(w);

  for (i = 0; i < sizeof (state_map) / sizeof (state_map[0]); i++)
    {
      if (strcmp(name, state_map[i].string) == 0) {
        t->state = state_map[i].value;
	g_free(name);
	return;
      }
    }

  /* Name not found ??? */
  g_assert_not_reached();
}

static void
set_item_priority (GtkComboBox *w, gpointer p)
{
  struct edit_todo *t = p;
  int i;
  gchar *name;

  name = gtk_combo_box_get_active_text(w);

  for (i = 0; i < sizeof (priority_map) / sizeof (priority_map[0]); i++)
    {
      if (strcmp(name, priority_map[i].string) == 0) {
        t->priority = priority_map[i].value;
	g_free(name);
	return;
      }
    }

  /* Name not found ??? */
  g_assert_not_reached();
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
  GtkWidget *dialog;

#ifdef IS_HILDON
  dialog = gpe_pim_categories_dialog (t->selected_categories, TRUE,
                                      G_CALLBACK (update_categories), t);
#else
  dialog = gpe_pim_categories_dialog (t->selected_categories, 
                                      G_CALLBACK (update_categories), t);
#endif    
  gtk_window_set_transient_for(GTK_WINDOW(dialog), 
                               GTK_WINDOW(gtk_widget_get_toplevel(w)));
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
}

/*
 * Pass item=NULL to create a new item;
 * if item=NULL, you may pass the item's initial category.
 */
GtkWidget *
edit_item (struct todo_item *item, gint initial_category, GtkWindow *parent)
{
  GtkWidget *window;
  GtkWidget *table = gtk_table_new(6, 5, FALSE);
  GtkWidget *top_vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *text = gtk_text_view_new ();
  GtkWidget *buttonok;
  GtkWidget *buttoncancel;
#ifdef IS_HILDON    
  GtkWidget *buttoncategories;
  GtkWidget *buttonbox = gtk_hbutton_box_new();
#else
  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
#endif    
  GtkWidget *buttondelete;
  GtkWidget *label_priority = gtk_label_new (_("Priority:"));
  GtkWidget *priority_combo;
  GtkWidget *label_state = gtk_label_new (_("Status:"));
  GtkWidget *state_combo;
  GtkWidget *label_summary = gtk_label_new (_("Summary:"));
  GtkWidget *label_details = gtk_label_new (_("Details:"));
  GtkWidget *entry_summary = gtk_entry_new ();
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *viewport = gtk_viewport_new(NULL, NULL);
  GtkWidget *button_categories;
  GtkWidget *label_categories = gtk_label_new ("");
  gchar *s = NULL;
  struct edit_todo *t;
  guint i = 0, pos = 0;
  guint gpe_spacing = gpe_get_boxspacing();
  struct tm tm;
  time_t the_time;

  t = g_malloc (sizeof (struct edit_todo));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  /* make it a dialog if it is large enough */
  if (large_screen)
    {
      gtk_window_set_type_hint(GTK_WINDOW(window),
                               GDK_WINDOW_TYPE_HINT_DIALOG);
      gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
      gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    }
    
  gtk_container_set_border_width (GTK_CONTAINER (window),
                                  gpe_get_border ());
#ifdef IS_HILDON    
  button_categories = gtk_label_new (_("Categories:"));
  gtk_misc_set_alignment(GTK_MISC(button_categories), 0.0, 0.0);
#else
  button_categories = gtk_button_new_with_label (_("Categories:"));  
#endif
	
  if (large_screen)
    {
      if (mode_landscape)
        gtk_window_set_default_size (GTK_WINDOW (window), 480, 320);
      else
        gtk_window_set_default_size (GTK_WINDOW (window), 320, 480);
    }
  else
    gtk_window_set_default_size (GTK_WINDOW (window), 240, 320);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (viewport), table);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  
  gtk_container_set_border_width(GTK_CONTAINER(buttonbox), 0);

#ifdef IS_HILDON  
  buttonok = gtk_button_new_with_label (_("OK"));
  buttoncancel = gtk_button_new_with_label (_("Cancel"));
  buttoncategories = gtk_button_new_with_label (_("Categories..."));
  buttondelete = gtk_button_new_from_stock (_("Delete"));
#else  
  buttonok = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  buttondelete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
#endif  
  gtk_table_set_col_spacings (GTK_TABLE(table), gpe_spacing);
  gtk_table_set_row_spacings (GTK_TABLE(table), gpe_spacing);
  gtk_box_set_spacing (GTK_BOX(top_vbox), gpe_spacing);
    
  state_combo = gtk_combo_box_new_text();
  for (i = 0; i < sizeof (state_map) / sizeof (state_map[0]); i++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX(state_combo), gettext (state_map[i].string));
    }
  g_signal_connect (G_OBJECT (state_combo), "changed", G_CALLBACK (set_item_state), t);
		   
  priority_combo = gtk_combo_box_new_text();
  for (i = 0; i < sizeof (priority_map) / sizeof (priority_map[0]); i++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX(priority_combo), gettext (priority_map[i].string));
    }
  g_signal_connect (G_OBJECT (priority_combo), "changed", G_CALLBACK (set_item_priority), t);

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

  s = build_categories_string (t);
  if (!s)
     s = g_strdup(_("(no category selected)"));
  gtk_label_set_text (GTK_LABEL (label_categories), s);
  g_free (s);
     
  gtk_misc_set_alignment (GTK_MISC (label_categories), 0.0, 0.5);
  gtk_label_set_line_wrap(GTK_LABEL (label_categories), TRUE);

#ifndef IS_HILDON  
  gtk_button_set_alignment(GTK_BUTTON(button_categories), 0.0, 0.5);    
  g_signal_connect (G_OBJECT (button_categories), "clicked", 
                    G_CALLBACK (change_categories), t);
#endif
  
  t->categories_label = label_categories;
  t->item = item;
  
  gtk_signal_connect (GTK_OBJECT (t->duetoggle), "clicked",
		      GTK_SIGNAL_FUNC (due_toggle_clicked), t);
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);
#ifdef IS_HILDON              
  gtk_signal_connect (GTK_OBJECT (buttoncategories), "clicked",
		      GTK_SIGNAL_FUNC (change_categories), t);
#endif
  gtk_signal_connect (GTK_OBJECT (buttondelete), "clicked",
		      GTK_SIGNAL_FUNC (click_delete), window);

#ifdef IS_HILDON
  gtk_box_pack_start (GTK_BOX (buttonbox), buttonok, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttoncategories, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttondelete, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttoncancel, TRUE, FALSE, 0);
#else
  gtk_box_pack_start (GTK_BOX (buttonbox), buttondelete, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttoncancel, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttonok, TRUE, FALSE, 0);
#endif

  gtk_misc_set_alignment (GTK_MISC (label_details), 0.0, 0.0);
  gtk_misc_set_alignment (GTK_MISC (label_summary), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (label_priority), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (label_state), 0.0, 0.5);
  
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), TRUE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text), GTK_WRAP_WORD_CHAR);
  
  /* Summary */
  gtk_table_attach(GTK_TABLE(table), label_summary, 0, 1, pos, pos+1, 
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry_summary, 1, 5, pos, pos+1,
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  pos++;
  /* Due date */                 
  gtk_table_attach(GTK_TABLE(table), t->duetoggle, 0, 1, pos, pos+1,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), t->duedate, 1, 5, pos, pos+1,
                   GTK_FILL, GTK_FILL, 0, 0);
  pos++;
  /* Priority */ 
  gtk_table_attach(GTK_TABLE(table), label_priority, 0, 1, pos, pos+1,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), priority_combo, 1, 
                   mode_landscape ? 3 : 5, pos, pos+1, 
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  /* State */                 
  if (mode_landscape)
    {    
      gtk_table_attach(GTK_TABLE(table), label_state, 3, 4, pos, pos+1,
                       GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_attach(GTK_TABLE(table), state_combo, 4, 5, pos, pos+1,
                       GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    }
  else
    {
      pos++;
      gtk_table_attach(GTK_TABLE(table), label_state, 0, 1, pos, pos+1,
                       GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_attach(GTK_TABLE(table), state_combo, 1, 5, pos, pos+1,
                       GTK_FILL, GTK_FILL, 0, 0);
    }
    pos++;
#ifdef IS_HILDON    
  /* Details */
  gtk_table_attach(GTK_TABLE(table), label_details, 0, 1, pos, pos+1,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), text, 1, 5, pos, pos+1,
                   GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
  pos++;
  /* Categories */
  gtk_table_attach(GTK_TABLE(table), button_categories, 0, 2, pos, pos+1,
                   GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), label_categories, 2, 5, pos, pos+1,
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  /*HACK: This is a workaround for a bug in the GtkTextView implementation 
    causing it to grow while entering text. */
  gtk_widget_set_size_request(text, 5, -1);
#else
  /* Categories */
  gtk_table_attach(GTK_TABLE(table), button_categories, 0, 2, pos, pos+1,
                   GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), label_categories, 2, 5, pos, pos+1,
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  pos++;
  /* Details */
  gtk_table_attach(GTK_TABLE(table), label_details, 0, 1, pos, pos+1,
                   GTK_FILL, GTK_FILL, 0, 0);
  pos++;
  gtk_table_attach(GTK_TABLE(table), text, 0, 5, pos, pos+1,
                   GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
                   
  /*HACK: This is a workaround for a bug in the GtkTextView implementation 
    causing it to grow while entering text. */
  gtk_widget_set_size_request(text, 5, -1);
#endif    
  gtk_box_pack_start (GTK_BOX (top_vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (top_vbox), buttonbox, FALSE, FALSE, 0);

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
      gtk_combo_box_set_active(GTK_COMBO_BOX(state_combo), item->state);
      if (item->priority > PRIORITY_STANDARD)
	prio_level = 0;
      else if (item->priority < PRIORITY_STANDARD)
	prio_level = 2;
      gtk_combo_box_set_active(GTK_COMBO_BOX(priority_combo), prio_level);
      t->state = item->state;
      t->priority = item->priority;

      if (item->time)
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (t->duetoggle), TRUE);
          the_time = item->time;
        }
      else
        gtk_widget_set_sensitive (t->duedate, FALSE);
      
      gtk_window_set_title (GTK_WINDOW (window), _("Edit Item"));
    }
  else
    {
      t->state = NOT_STARTED;
      gtk_combo_box_set_active(GTK_COMBO_BOX(state_combo), t->state);
      t->priority = PRIORITY_STANDARD;
      gtk_combo_box_set_active(GTK_COMBO_BOX(priority_combo), 1);
#ifdef IS_HILDON        
      gtk_widget_destroy (buttondelete);
#endif        
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
