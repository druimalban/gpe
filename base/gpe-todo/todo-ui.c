/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#include <gtk/gtk.h>

#include "gtkdatecombo.h"
#include "todo.h"

#define _(_x) gettext(_x)

struct edit_todo
{
  GtkWidget *text;
  GtkWidget *state;
  GtkWidget *duetoggle;
  GtkWidget *duedate;

  struct todo_item *item;
  struct todo_list *list;
};

static void
destroy_user_data (gpointer p)
{
  struct edit_todo *t = (struct edit_todo *)p;

  g_free (p);
}

static void
click_cancel(GtkWidget *widget,
	     GtkWidget *window)
{
  gtk_widget_hide (window);
  gtk_widget_destroy (window);
}

static void
click_ok(GtkWidget *widget,
	 GtkWidget *window)
{
  struct edit_todo *t = gtk_object_get_data (GTK_OBJECT (window), "todo");

  time_t when;
  struct tm tm;
  const char *what;
  //  gboolean completed = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t->completed));
  item_state state = NOT_STARTED;

#if 0  
  memset(&tm, 0, sizeof(tm));
  tm.tm_year = t->year - 1900;
  tm.tm_mon = t->month;
  tm.tm_mday = t->day;
  when = mktime(&tm);
#endif

  what = gtk_editable_get_chars (GTK_EDITABLE (t->text), 0, -1);

  if (t->item)
    {
      g_free ((char *)t->item->what);
      t->item->what = what;
      t->item->time = when;
      t->item->state = state;
      gtk_widget_draw (t->list->widget, NULL);
    }
  else
    add_new_event (t->list, when, what, state);

  gtk_widget_hide (window);
  gtk_widget_destroy (window);
}

GtkWidget *
edit_todo(struct todo_list *list, struct todo_item *item)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *text = gtk_text_new (NULL, NULL);
  GtkWidget *duebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *donebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok = gtk_button_new_with_label (_("Save"));
  GtkWidget *buttoncancel = gtk_button_new_with_label (_("Cancel"));
  GtkWidget *buttondelete = gtk_button_new_with_label (_("Delete"));
  GtkWidget *state = gtk_option_menu_new ();
  GtkWidget *state_menu = gtk_menu_new ();
  struct edit_todo *t = g_malloc(sizeof(struct edit_todo));
  struct tm tm;
  time_t the_time;
  char buf[32];

  gtk_menu_append (GTK_MENU (state_menu), 
		   gtk_menu_item_new_with_label (_("Not started")));
  gtk_menu_append (GTK_MENU (state_menu), 
		   gtk_menu_item_new_with_label (_("In progress")));
  gtk_menu_append (GTK_MENU (state_menu), 
		   gtk_menu_item_new_with_label (_("Completed")));

  t->duetoggle = gtk_check_button_new_with_label (_("Due:"));
  t->duedate = gtk_date_combo_new ();

  t->list = list;
  t->item = item;
  t->state = state;
  
  gtk_widget_set_usize (window, 240, 320);

  gtk_widget_set_usize (buttonok, 60, -1);
  gtk_widget_set_usize (buttoncancel, 60, -1);

  gtk_widget_set_usize (state, -1, state->style->font->ascent * 2);

  gtk_box_pack_start (GTK_BOX (duebox), t->duetoggle, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (duebox), t->duedate, TRUE, TRUE, 4);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);

  gtk_box_pack_start (GTK_BOX (buttonbox), buttondelete, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttoncancel, TRUE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttonok, TRUE, FALSE, 4);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (state), state_menu);

  gtk_box_pack_start (GTK_BOX (vbox), state, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), duebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), text, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), buttonbox, FALSE, FALSE, 2);

  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  gtk_text_set_word_wrap (GTK_TEXT (text), TRUE);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  if (item)
    {
      gint p = 0;
      the_time = item->time;
      gtk_editable_insert_text (GTK_EDITABLE (text), item->what, 
				strlen (item->what), &p);
    }
  else
    {
      time (&the_time);
      gtk_widget_set_sensitive (buttondelete, FALSE);
    }

  t->text = text;
  
  gtk_object_set_data_full (GTK_OBJECT (window), "todo", t, destroy_user_data);

  return window;
}
