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

#include <gtk/gtk.h>

#include "todo.h"

struct edit_todo
{
  int year, month, day;
  GtkWidget *cal;
  GtkWidget *due_e;
  GtkWidget *text;
  GtkWidget *state;

  struct todo_item *item;
  struct todo_list *list;
};

static void
click_calendar(GtkWidget *widget,
	       struct edit_todo *t)
{
  struct tm tm;
  char buf[256];
  
  gtk_calendar_get_date (GTK_CALENDAR (widget), &t->year, &t->month, &t->day);
  memset(&tm, 0, sizeof(tm));
  tm.tm_year = t->year - 1900;
  tm.tm_mon = t->month;
  tm.tm_mday = t->day;
  strftime (buf, sizeof(buf), "%a, %d %b %Y", &tm);
  gtk_entry_set_text (GTK_ENTRY (t->due_e), buf);

  gtk_widget_hide (widget);
  gtk_widget_destroy (widget);
}

static void
drop_calendar(GtkWidget *widget,
	      GtkWidget *window)
{
  struct edit_todo *t = gtk_object_get_data (GTK_OBJECT (window), "todo");

  if (t->cal)
    {
      gtk_widget_hide (t->cal);
      gtk_widget_destroy (t->cal);

      t->cal = NULL;
    }
  else
    {
      GtkWidget *cal = gtk_calendar_new ();
      GtkWidget *calw = gtk_window_new (GTK_WINDOW_POPUP);
      GtkRequisition requisition;
      gint x, y;
      gint screen_width;
      gint screen_height;

      gtk_calendar_select_month (GTK_CALENDAR (cal), t->month, t->year);
      gtk_calendar_select_day (GTK_CALENDAR (cal), t->day);
 
      gtk_container_add (GTK_CONTAINER (calw), cal);
      gtk_window_set_policy (GTK_WINDOW (calw),
			     FALSE, FALSE, TRUE);
      
      gdk_window_get_pointer (NULL, &x, &y, NULL);
      gtk_widget_size_request (cal, &requisition);
      
      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();

      x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
      y = CLAMP (y + 2, 0, MAX (0, screen_height - requisition.height));
      
      gtk_widget_set_uposition (calw, MAX (x, 0), MAX (y, 0));
      
      gtk_widget_show_all (calw);
      t->cal = calw;
      gtk_signal_connect (GTK_OBJECT (cal), "day-selected-double-click",
			  GTK_SIGNAL_FUNC (click_calendar), t);
    }
}

static void
destroy_user_data (gpointer p)
{
  struct edit_todo *t = (struct edit_todo *)p;

  if (t->cal)
    {
      gtk_widget_hide (t->cal);
      gtk_widget_destroy (t->cal);
      t->cal = NULL;
    }

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
  
  memset(&tm, 0, sizeof(tm));
  tm.tm_year = t->year - 1900;
  tm.tm_mon = t->month;
  tm.tm_mday = t->day;

  when = mktime(&tm);
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
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *text = gtk_text_new (NULL, NULL);
  GtkWidget *duebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *donebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok = gtk_button_new_with_label ("OK");
  GtkWidget *buttoncancel = gtk_button_new_with_label ("Cancel");
  GtkWidget *due = gtk_label_new ("Due:");
  GtkWidget *due_b = gtk_button_new ();
  GtkWidget *due_e = gtk_entry_new ();
  GtkWidget *state = gtk_option_menu_new ();
  GtkWidget *state_menu = gtk_menu_new ();
  struct edit_todo *t = g_malloc(sizeof(struct edit_todo));
  struct tm tm;
  time_t the_time;
  char buf[32];

  gtk_menu_append (GTK_MENU (state_menu), 
		   gtk_menu_item_new_with_label ("Not started"));
  gtk_menu_append (GTK_MENU (state_menu), 
		   gtk_menu_item_new_with_label ("In progress"));
  gtk_menu_append (GTK_MENU (state_menu), 
		   gtk_menu_item_new_with_label ("Completed"));

  t->list = list;
  t->item = item;
  t->state = state;
  
  gtk_widget_set_usize (window, 240, 320);

  gtk_widget_set_usize (buttonok, 60, -1);
  gtk_widget_set_usize (buttoncancel, 60, -1);

  gtk_widget_set_usize (state, -1, state->style->font->ascent * 2);

  gtk_container_add (GTK_CONTAINER (due_b),
		     gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT));

  gtk_box_pack_start (GTK_BOX (duebox), due, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (duebox), due_e, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (duebox), due_b, FALSE, FALSE, 4);

  gtk_editable_set_editable (GTK_EDITABLE (due_e), FALSE);

  gtk_signal_connect (GTK_OBJECT (due_b), "clicked",
		      GTK_SIGNAL_FUNC (drop_calendar), window);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);

  gtk_box_pack_end (GTK_BOX (buttonbox), buttoncancel, FALSE, FALSE, 4);
  gtk_box_pack_end (GTK_BOX (buttonbox), buttonok, FALSE, FALSE, 4);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (state), state_menu);

  gtk_box_pack_start (GTK_BOX (vbox), state, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), duebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), text, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);
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
      //      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
      //				    item->completed);
    }
  else
    {
      time(&the_time);
    }
  localtime_r (&the_time, &tm);
  strftime (buf, sizeof(buf), "%a, %d %b %Y", &tm);
  gtk_entry_set_text (GTK_ENTRY (due_e), buf);
  t->year = tm.tm_year + 1900;
  t->month = tm.tm_mon;
  t->day = tm.tm_mday;

  t->cal = NULL;
  t->due_e = due_e;
  t->text = text;
  
  gtk_object_set_data_full (GTK_OBJECT (window), "todo", t, destroy_user_data);

  return window;
}
