/* event-menu.c - Event menu implementation.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <gpe/question.h>
#include <gpe/pixmaps.h>
#include "event-menu.h"
#include "globals.h"
#include "export-vcal.h"
#include "event-ui.h"
#include "calendars-widgets.h"

static void
edit_event_cb (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = edit_event (EVENT (d));
  gtk_widget_show (w);
}

static void
delete_event_cb (GtkWidget *widget, gpointer d)
{
  Event *ev = EVENT (d);

  if (event_is_recurrence (ev))
    {
      if (gpe_question_ask
	  (_
	   ("Delete all recurring entries?\n"
	    "(If no, delete this instance only)"),
	   _("Question"), "question", "!gtk-no", NULL, "!gtk-yes", NULL,
	   NULL))
	event_remove (ev);
      else
	{
	  event_add_recurrence_exception (ev, event_get_start (ev));
	  event_flush (ev);
	}
    }
  else
    event_remove (ev);

  update_view ();
}

static void
save_cb (GtkWidget *widget, gpointer d)
{
  vcal_do_save (EVENT (d));
}

static void
send_ir_cb (GtkWidget *widget, gpointer d)
{
  vcal_do_send_irda (EVENT (d));
}

static void
send_bt_cb (GtkWidget *widget, gpointer d)
{
  vcal_do_send_bluetooth (EVENT (d));
}

static void
move_event_to (EventCalendar *ec, gpointer user_data)
{
  Event *ev = EVENT (user_data);
  event_set_calendar (ev, ec);
}

static void
event_menu_destroy (GtkWidget *widget, gpointer d)
{
  Event *ev = EVENT (d);
  g_object_unref (ev);
  gtk_widget_destroy (widget);
}

GtkMenu *
event_menu_new (Event *ev, gboolean show_summary)
{
  GtkMenu *menu = GTK_MENU (gtk_menu_new ());
  g_object_ref (ev);
  int i = 0;

  /* The event title.  */
  if (show_summary)
    {
      struct tm start_tm, end_tm;

      time_t start = event_get_start (ev);
      localtime_r (&start, &start_tm);
      char *strstart = strftime_strdup_utf8_locale (TIMEFMT, &start_tm);

      time_t end = start + event_get_duration (ev);
      gchar *strend;
      if (end == start)
	strend = 0;
      else
	{
	  localtime_r (&end, &end_tm);
	  strend = strftime_strdup_utf8_locale (TIMEFMT, &end_tm);
	}

      char buffer[64];
      char *summary = event_get_summary (ev);
      char *description = event_get_description (ev);
      int l = snprintf (buffer, 64, "%s %s%s%s %s%s%s",
			summary,
			strstart, strend ? "-" : "", strend ?: "",
			event_get_alarm (ev) ? "(A)" : "",
			description ? "\n" : "",
			description ? description : "");
      g_free (summary);
      g_free (description);
      g_free(strstart);
      g_free(strend);
      buffer[64] = 0;
      if (l > sizeof (buffer))
	l = sizeof (buffer);
      l --;
      while (buffer[l] == '\n')
	buffer[l --] = 0;
    
      char *tbuffer = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
      if (tbuffer || buffer)
	{
	  GtkWidget *event_menu_info
	    = gtk_menu_item_new_with_label (tbuffer ?: buffer);
	  gtk_widget_show (event_menu_info);
	  gtk_menu_attach (menu, event_menu_info, 0, 1, i, i + 1);
	  i ++;
	}

      if (tbuffer)
	g_free (tbuffer);
    }

  /* Create an edit button.  */
  {
    GtkWidget *edit;

#ifdef IS_HILDON
    edit = gtk_menu_item_new_with_label (_("Edit"));
#else
    edit = gtk_image_menu_item_new_from_stock (GTK_STOCK_EDIT, NULL);
#endif
    g_signal_connect (G_OBJECT (edit), "activate",
		      G_CALLBACK (edit_event_cb), ev);
    gtk_widget_show (edit);
    gtk_menu_attach (menu, edit, 0, 1, i, i + 1);
    i ++;
  }

  /* A delete button.  */
  {
    GtkWidget *delete;

#ifdef IS_HILDON
    delete = gtk_menu_item_new_with_label (_("Delete"));
#else
    delete = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
#endif
    g_signal_connect (G_OBJECT (delete), "activate",
		      G_CALLBACK (delete_event_cb), ev);
    gtk_widget_show (delete);
    gtk_menu_attach (menu, delete, 0, 1, i, i + 1);
    i ++;
  }

  /* And a save button.  */
  {
    GtkWidget *save;

#ifdef IS_HILDON
    save = gtk_menu_item_new_with_label (_("Save"));
#else
    save = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE, NULL);
#endif
    g_signal_connect (G_OBJECT (save), "activate",
		      G_CALLBACK (save_cb), ev);
    gtk_widget_show (save);
    gtk_menu_attach (menu, save, 0, 1, i, i + 1);
    i ++;
  }

  /* And a "Move to..." sub menu.  */
  GtkWidget *calendars = calendars_menu (move_event_to, ev);
  if (calendars)
    {
      gtk_widget_show (calendars);

      GtkWidget *item
	= gtk_menu_item_new_with_label (_("Move to calendar..."));
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), calendars);
      gtk_menu_attach (menu, item, 0, 1, i, i + 1);
      gtk_widget_show (item);
      i ++;
    }

  /* Create a Send via infra-red button if infra-red is available.  */
  if (export_irda_available ())
    {
      GtkWidget *send_ir_button;

      send_ir_button = gtk_menu_item_new_with_label (_("Send via infra-red"));
      g_signal_connect (G_OBJECT (send_ir_button), "activate",
			G_CALLBACK (send_ir_cb), ev);
      gtk_widget_show (send_ir_button);
      gtk_menu_attach (menu, send_ir_button, 0, 1, i, i + 1);
      i ++;
    }

  /* Create a Send via Bluetooth button if bluetooth is available.  */
  if (export_bluetooth_available ())
    {
      GtkWidget *send_bt_button;

      send_bt_button = gtk_menu_item_new_with_label (_("Send via Bluetooth"));
      g_signal_connect (G_OBJECT (send_bt_button), "activate",
			G_CALLBACK (send_bt_cb), ev);
      gtk_widget_show (send_bt_button);
      gtk_menu_attach (menu, send_bt_button, 0, 1, i, i + 1);
      i ++;
    }

  g_signal_connect (G_OBJECT (menu), "selection-done",
		    G_CALLBACK (event_menu_destroy), ev);

  return menu;
}

