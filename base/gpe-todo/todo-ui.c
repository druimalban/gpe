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
#include "todo-sql.h"
#include "pixmaps.h"
#include "picturebutton.h"

#define _(_x) gettext(_x)

struct edit_todo
{
  GtkWidget *text;
  GtkWidget *summary;
  GtkWidget *duetoggle;
  GtkWidget *duedate;

  struct todo_item *item;
  struct todo_list *list;

  item_state state;
};

static void
destroy_user_data (gpointer p)
{
  g_free (p);
}

static void
due_toggle_clicked(GtkWidget *widget, struct edit_todo *t)
{
  gtk_widget_set_sensitive (t->duedate, 
		    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
						  (t->duetoggle)));
}

static void
click_cancel(GtkWidget *widget,
	     GtkWidget *window)
{
  gtk_widget_hide (window);
  gtk_widget_destroy (window);
}

static void
click_delete(GtkWidget *widget,
	     GtkWindow *window)
{
  struct edit_todo *t = gtk_object_get_data (GTK_OBJECT (window), "todo");
  
  if (t->item)
    delete_item (t->list, t->item);
}

static void
click_ok(GtkWidget *widget,
	 GtkWidget *window)
{
  struct edit_todo *t = gtk_object_get_data (GTK_OBJECT (window), "todo");

  time_t when;
  struct tm tm;
  const char *what, *summary;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (t->duetoggle)))
    {
      memset(&tm, 0, sizeof(tm));
      tm.tm_year = GTK_DATE_COMBO (t->duedate)->year - 1900;
      tm.tm_mon = GTK_DATE_COMBO (t->duedate)->month;
      tm.tm_mday = GTK_DATE_COMBO (t->duedate)->day;
      when = mktime(&tm);
    }
  else
    when = (time_t)0;

  what = gtk_editable_get_chars (GTK_EDITABLE (t->text), 0, -1);
  summary = gtk_editable_get_chars (GTK_EDITABLE (t->summary), 0, -1);

  if (t->item)
    {
      g_free ((char *)t->item->what);
      t->item->what = what;
      g_free ((char *)t->item->summary);
      t->item->summary = summary;
      t->item->time = when;
      t->item->state = t->state;
      push_item (t->item);
      t->list->items = g_list_sort (t->list->items, list_sort_func);
    }
  else
    add_new_item (t->list, when, what, t->state, summary, new_unique_id ());

  gtk_widget_draw (g_draw, NULL);

  gtk_widget_hide (window);
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

GtkWidget *
edit_todo(struct todo_list *list, struct todo_item *item)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *text = gtk_text_new (NULL, NULL);
  GtkWidget *duebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok;
  GtkWidget *buttoncancel;
    GtkWidget *buttondelete;
  GtkWidget *state = gtk_option_menu_new ();
  GtkWidget *state_menu = gtk_menu_new ();
  GtkWidget *label_summary = gtk_label_new (_("Summary:"));
  GtkWidget *frame_details = gtk_frame_new (_("Details"));
  GtkWidget *entry_summary = gtk_entry_new ();
  GtkWidget *hbox_summary = gtk_hbox_new (FALSE, 0);
  struct edit_todo *t = g_malloc(sizeof(struct edit_todo));

  const char *state_strings[] = { _("Not started"), _("In progress"),
				  _("Completed") };
  void (*state_funcs[3])(GtkMenuItem *, gpointer) = 
    { state_func_0, state_func_1, state_func_2 };
  guint i;
  struct tm tm;
  time_t the_time;

  gtk_widget_realize (window);

  buttonok = gpe_picture_button (window->style, _("Save"), "save");
  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
  buttondelete = gpe_picture_button (window->style, _("Delete"), "delete");
 
  for (i = 0; i < 3; i++)
    {
      GtkWidget *l = gtk_menu_item_new_with_label (state_strings[i]);
      gtk_menu_append (GTK_MENU (state_menu), l);
      gtk_signal_connect (GTK_OBJECT (l), "activate", state_funcs[i], t);
    }
		   
  t->duetoggle = gtk_check_button_new_with_label (_("Due:"));
  t->duedate = gtk_date_combo_new ();

  t->list = list;
  t->item = item;
  
  gtk_widget_set_usize (window, 240, 320);

  gtk_widget_set_usize (state, -1, state->style->font->ascent + 
			state->style->font->descent + 4);

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

  gtk_container_add (GTK_CONTAINER (frame_details), text);

  gtk_box_pack_start (GTK_BOX (hbox_summary), label_summary, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox_summary), entry_summary, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox_summary, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), duebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), state, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), frame_details, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), buttonbox, FALSE, FALSE, 2);

  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  gtk_text_set_word_wrap (GTK_TEXT (text), TRUE);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_grab_focus (entry_summary);

  the_time = time (NULL);

  if (item)
    {
      gint p = 0;
      gtk_editable_insert_text (GTK_EDITABLE (text), item->what, 
				strlen (item->what), &p);
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

      gtk_window_set_title (GTK_WINDOW (window), _("Edit item"));
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

  return window;
}
