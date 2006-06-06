/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <libintl.h>
#include <langinfo.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/question.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/errorbox.h>
#include <gpe/gtksimplemenu.h>
#include <gpe/event-db.h>
#include <gpe/spacing.h>
#include <gpe/pim-categories.h>
#include <gpe/pim-categories-ui.h>
#include <gpe/gpetimesel.h>

#include "calendars-widgets.h"
#include "globals.h"
#include "event-ui.h"
#include "calendar-edit-dialog.h"

struct edit_state
{
  GtkWindow *window;

  GtkWidget *tabs;

  GtkToggleButton *all_day;
  GtkWidget *startdate, *enddate;
  GtkWidget *starttime, *endtime;
  GtkWidget *calendar;

  GtkWidget *alarmbutton;
  GtkWidget *alarmspin;
  GtkComboBox *alarm_units;

  GtkWidget *description;
  GtkWidget *summary;
  GtkWidget *location;

  GtkComboBox *recur_type;

  GtkContainer *recur_box;

  GtkSpinButton *increment_unit;
  GtkLabel *increment_unit_postfix;

  GtkContainer *byday;
  GtkWidget *checkbuttonwday[7];

  GtkWidget *radiobuttonforever, *radiobuttonendafter,
            *radiobuttonendon, *datecomboendon;
  GtkSpinButton *endafter;

  GSList *categories;
  GtkWidget *categories_label;

  Event *ev;

  gboolean recur_day_floating;
  gboolean end_date_floating, end_time_floating;
};

/* minute, hour, day, week */
static const int alarm_multipliers[] = { 60, 60*60, 24*60*60, 7*24*60*60 };
 
static void
destroy_user_data (gpointer user_data, GObject *object)
{
  struct edit_state *s = user_data;
  g_slist_free (s->categories);
  g_free (s);
}

static void
recur_combo_changed (GtkWidget *widget, struct edit_state *s)
{
  switch (gtk_combo_box_get_active (s->recur_type))
    {
    case 0:
      gtk_label_set_text (s->increment_unit_postfix, _(""));
      break;
    case 1:
      gtk_label_set_text (s->increment_unit_postfix, _("days"));
      break;
    case 2:
      gtk_label_set_text (s->increment_unit_postfix, _("weeks, on:"));
      break;
    case 3:
      gtk_label_set_text (s->increment_unit_postfix, _("months"));
      break;
    case 4:
      gtk_label_set_text (s->increment_unit_postfix, _("years"));
      break;
    }

  gtk_widget_set_sensitive (GTK_WIDGET (s->recur_box),
			    gtk_combo_box_get_active (s->recur_type) != 0);

  if (gtk_combo_box_get_active (s->recur_type) == 2)
    gtk_widget_show (GTK_WIDGET (s->byday));
  else
    gtk_widget_hide (GTK_WIDGET (s->byday));
}

static guint cached_edit_state_destroy_source;
static struct edit_state *cached_edit_state;

static gboolean
cached_edit_state_destroy (gpointer data)
{
  if (cached_edit_state)
    {
      gtk_widget_destroy (GTK_WIDGET (cached_edit_state->window));
      cached_edit_state = NULL;
    }
      
  return FALSE;
}

static gboolean
edit_finished (struct edit_state *s)
{
  gtk_widget_hide (GTK_WIDGET (s->window));

  if (cached_edit_state)
    gtk_widget_destroy (GTK_WIDGET (s->window));
  else
    {
      cached_edit_state = s;
      /* Remove cached window in 5 minutes.  */
      cached_edit_state_destroy_source
	= g_timeout_add ((time (NULL) + 5 * 60 * 60) * 1000,
			 cached_edit_state_destroy, NULL);
    }

  return TRUE;
}

static void
click_ok (GtkWidget *widget, struct edit_state *s)
{
  struct tm tm_start, tm_end, tm_rend;
  time_t start_t, end_t, rend_t;
  GtkTextIter start, end;
  GtkTextBuffer *buf;

  memset (&tm_start, 0, sizeof (struct tm));
  tm_start.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
  tm_start.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
  tm_start.tm_mday = GTK_DATE_COMBO (s->startdate)->day;
  tm_start.tm_isdst = -1;

  memset (&tm_end, 0, sizeof (struct tm));
  tm_end.tm_year = GTK_DATE_COMBO (s->enddate)->year - 1900;
  tm_end.tm_mon = GTK_DATE_COMBO (s->enddate)->month;
  tm_end.tm_mday = GTK_DATE_COMBO (s->enddate)->day;
  tm_end.tm_isdst = -1;

  if (! gtk_toggle_button_get_active (s->all_day))
    /* Appointment */
    {
      gpe_time_sel_get_time (GPE_TIME_SEL (s->starttime), (guint *)&tm_start.tm_hour, (guint *)&tm_start.tm_min);
      gpe_time_sel_get_time (GPE_TIME_SEL (s->endtime), (guint *)&tm_end.tm_hour, (guint *)&tm_end.tm_min);

      start_t = mktime (&tm_start);
      end_t = mktime (&tm_end);

      /* Zero length appointments would be confused with reminders, so
	 make them one second long.  */
      if (end_t == start_t)
	end_t++;
    }
  else
    {
      /* Reminder */
      start_t = mktime (&tm_start);
      end_t = mktime (&tm_end);
    }

  if (end_t < start_t)
    {
      gpe_error_box (_("End time must not be earlier than start time"));
      if (s->ev)
	g_object_unref (s->ev);
      return;
    }

  if (start_t < time(NULL))
    {
      GtkWindow *window = GTK_WINDOW (gtk_widget_get_toplevel (widget));
      GtkWidget* dialog;
#ifdef IS_HILDON
      dialog = hildon_note_new_confirmation
	(window, _("Event starts in the past!\nSave anyway?"));
#else        
      dialog = gtk_message_dialog_new
	(window, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	 GTK_MESSAGE_WARNING,
	 GTK_BUTTONS_YES_NO,
	 _("Event starts in the past!\nSave anyway?"));
#endif        
      int ret = gtk_dialog_run (GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      if ((ret == GTK_RESPONSE_NO) || (ret == GTK_RESPONSE_CANCEL))
        {
	  if (s->ev)
	    g_object_unref (s->ev);
          return;
        }
    }

  EventCalendar *ec
    = calendars_combo_box_get_active (GTK_COMBO_BOX (s->calendar));
  Event *ev;
  if (s->ev)
    {
      ev = s->ev;
      event_set_calendar (ev, ec);
    }
  else
    ev = event_new (event_db, ec, NULL);

  event_set_recurrence_start (ev, start_t);
  event_set_duration (ev, end_t - start_t);
  event_set_untimed (ev, gtk_toggle_button_get_active (s->all_day));

  buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description));
  gtk_text_buffer_get_bounds (buf, &start, &end);
  char *text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  event_set_description (ev, text);
  g_free (text);

  text = gtk_editable_get_chars (GTK_EDITABLE (s->summary), 0, -1);
  event_set_summary (ev, text);
  g_free (text);
  
  text = gtk_editable_get_chars (GTK_EDITABLE (s->location), 0, -1);
  event_set_location (ev, text);
  g_free (text);

  event_set_categories (ev, g_slist_copy (s->categories));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
    {
      int mi = gtk_combo_box_get_active (s->alarm_units);
      event_set_alarm (ev, alarm_multipliers[mi]
		       * gtk_spin_button_get_value_as_int
		       (GTK_SPIN_BUTTON (s->alarmspin)));
    }
  else
    event_set_alarm (ev, 0);

  enum event_recurrence_type recur = RECUR_NONE;
  switch (gtk_combo_box_get_active (s->recur_type))
    {
    case 0:
      recur = RECUR_NONE;
      break;
    case 1:
      recur = RECUR_DAILY;
      break;
    case 2:
      recur = RECUR_WEEKLY;
      break;
    case 3:
      recur = RECUR_MONTHLY;
      break;
    case 4:
      recur = RECUR_YEARLY;
      break;
    }

  event_set_recurrence_type (ev, recur);
  if (recur != RECUR_NONE)
    {
      event_set_recurrence_increment
	(ev, gtk_spin_button_get_value_as_int (s->increment_unit));

      if (gtk_toggle_button_get_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonforever)))
	{
	  event_set_recurrence_count (ev, 0);
	  event_set_recurrence_end (ev, 0);
	}
      else if (gtk_toggle_button_get_active
	       (GTK_TOGGLE_BUTTON (s->radiobuttonendafter)))
	{
	  event_set_recurrence_count
	    (ev, gtk_spin_button_get_value_as_int (s->endafter));
	  event_set_recurrence_end (ev, 0);
	}

      else if (gtk_toggle_button_get_active
	       (GTK_TOGGLE_BUTTON (s->radiobuttonendon)))
	{
	  memset (&tm_rend, 0, sizeof (struct tm));

	  tm_rend.tm_year = GTK_DATE_COMBO (s->datecomboendon)->year - 1900;
	  tm_rend.tm_mon = GTK_DATE_COMBO (s->datecomboendon)->month;
	  tm_rend.tm_mday = GTK_DATE_COMBO (s->datecomboendon)->day;
	  tm_rend.tm_isdst = -1;
	  gpe_time_sel_get_time (GPE_TIME_SEL (s->endtime),
				 (guint *)&tm_rend.tm_hour,
				 (guint *)&tm_rend.tm_min);

	  rend_t = mktime (&tm_rend);

	  event_set_recurrence_count (ev, 0);
	  event_set_recurrence_end (ev, rend_t);
	}
    }

  if (recur == RECUR_WEEKLY)
    {
      int i;
      int daymask = 0;
      for (i = 0; i < 7; i++)
	if (gtk_toggle_button_get_active
	    (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i])))
	  daymask |= 1 << i;
      event_set_recurrence_daymask (ev, daymask);
    }

  event_flush (ev);
  update_view ();
  edit_finished (s);
}

static void
all_day_toggled (GtkToggleButton *all_day, struct edit_state *s)
{
  if (gtk_toggle_button_get_active (all_day))
    {
      gtk_widget_set_sensitive (s->starttime, 0);
      gtk_widget_set_sensitive (s->endtime, 0);
    }
  else
    {
      gtk_widget_set_sensitive (s->starttime, 1);
      gtk_widget_set_sensitive (s->endtime, 1);
    }
}

static gchar *
build_categories_string (struct edit_state *es)
{
  gchar *s = NULL;
  GSList *iter;

  for (iter = es->categories; iter; iter = iter->next)
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
update_categories (GtkWidget *ui, GSList *new,
		   struct edit_state *s)
{
  gchar *str;

  g_slist_free (s->categories);

  s->categories = g_slist_copy (new);

  str = build_categories_string (s);
  gtk_label_set_text (GTK_LABEL (s->categories_label), str);
  g_free (str);
}

static void
click_categories (GtkWidget *b, struct edit_state *s)
{
  GtkWidget *ui;

#ifdef IS_HILDON  
  ui = gpe_pim_categories_dialog (s->categories, TRUE, G_CALLBACK (update_categories), s);
#else
  ui = gpe_pim_categories_dialog (s->categories, G_CALLBACK (update_categories), s);
#endif
  gtk_window_set_transient_for(GTK_WINDOW(ui),
                               GTK_WINDOW(gtk_widget_get_toplevel(b)));
  gtk_window_set_modal(GTK_WINDOW(ui), TRUE);
}

static gboolean
on_description_focus_in(GtkWidget* widget, GdkEventFocus *event,
                        gpointer user_data)
{
  GObject *window = user_data;
  g_object_set_data(window,"multiline",(void*)TRUE);
  return FALSE;
}

static gboolean
on_description_focus_out(GtkWidget* widget, GdkEventFocus *event,
                        gpointer user_data)
{
  GObject *window = user_data;
  g_object_set_data(window,"multiline",(void*)FALSE);
  return FALSE;
}

static gboolean
event_ui_key_press_event (GtkWidget *widget, GdkEventKey *k,
			  struct edit_state *s)
{
  if (k->keyval == GDK_Escape)
    {
      edit_finished (s);
      return TRUE;
    }
  if (k->keyval == GDK_Return)
    {
      if (!g_object_get_data(G_OBJECT(widget),"multiline"))
        {
          click_ok (widget, s);
          return TRUE;
       }
    }
  return FALSE;
}

static void
sink_weekly (GtkWidget *widget, struct edit_state *s)
{
  s->recur_day_floating = FALSE;
}

static void
sink_end_date (GtkWidget *widget, struct edit_state *s)
{
  s->end_date_floating = FALSE;
  s->end_time_floating = FALSE;
}

static void
sink_end_time (GtkWidget *widget, struct edit_state *s)
{
  s->end_time_floating = FALSE;
}

static void
note_time_change (GtkWidget *widget, struct edit_state *s)
{
  guint hour, minute;

  gpe_time_sel_get_time (GPE_TIME_SEL (widget), &hour, &minute);

  if (s->end_time_floating)
    {
      struct tm tm;
      memset (&tm, 0, sizeof (struct tm));
      tm.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
      tm.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
      tm.tm_mday = GTK_DATE_COMBO (s->startdate)->day;
      tm.tm_hour = hour;
      tm.tm_min = minute;
      tm.tm_isdst = -1;
      time_t t = mktime (&tm) + 60 * 60;
      localtime_r (&t, &tm);

      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
			       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      gpe_time_sel_set_time (GPE_TIME_SEL (s->endtime), tm.tm_hour, tm.tm_min);
    }
}

static void
note_date_change (GtkDateCombo *c, struct edit_state *s)
{
  if (s->recur_day_floating)
    {
      GDate date;
      g_date_set_dmy (&date, c->day, c->month + 1, c->year);
      int wday = g_date_weekday (&date);

      int i;
      for (i = 0; i < 7; i++)
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), i == wday - 1);
    }
}

static void
new_calendar_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *w = calendar_edit_dialog_new (NULL);
  gtk_window_set_transient_for
    (GTK_WINDOW (w),
     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))));
  if (gtk_dialog_run (GTK_DIALOG (w)) == GTK_RESPONSE_ACCEPT)
    {
      EventCalendar *ec
	= calendar_edit_dialog_get_calendar (CALENDAR_EDIT_DIALOG (w));
      if (ec)
	calendars_combo_box_set_active (user_data, ec);
    }

  gtk_widget_destroy (w);
}

static void
set_toggle (GtkWidget *widget, GtkToggleButton *toggle)
{
  gtk_toggle_button_set_active (toggle, TRUE);
}

static struct edit_state *
build_edit_event_window (void)
{
  static const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5,
                                  ABDAY_6, ABDAY_7, ABDAY_1 };
  struct edit_state *s = g_malloc0 (sizeof (struct edit_state));

  int boxspacing = gpe_get_boxspacing () / 2;
  int border = gpe_get_border () / 2;

  /* Building the dialog */
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  s->window = GTK_WINDOW (window);
  g_object_weak_ref (G_OBJECT (window), destroy_user_data, s);
  gpe_set_window_icon (window, "icon");

  int tiny = (gdk_screen_width () <= 300) || (gdk_screen_height () <= 400);
  int large = (gdk_screen_height () >= 600);
  /* if screen is large enough, make it a real dialog */
  if (! tiny)
    {
      gtk_window_set_type_hint (GTK_WINDOW (window),
				GDK_WINDOW_TYPE_HINT_DIALOG);
      gtk_window_set_transient_for
	(GTK_WINDOW (window),
	 GTK_WINDOW (gtk_widget_get_toplevel (main_window)));
      gtk_window_set_modal (GTK_WINDOW(window), TRUE);
    }

  GtkBox *window_box = GTK_BOX (gtk_vbox_new (FALSE, 3));
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (window_box));
  gtk_widget_show (GTK_WIDGET (window_box));

  s->tabs = gtk_notebook_new ();
  gtk_box_pack_start (window_box, GTK_WIDGET (s->tabs), TRUE, TRUE, 0);
  gtk_widget_show (s->tabs);
  
  /* Event tab.  */
  GtkWidget *label = gtk_label_new (_("Event"));
  gtk_widget_show (label);

  GtkWidget *scrolled_window;
  if (! large)
    {
      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				scrolled_window, label);
      gtk_widget_show (scrolled_window);
    }

  GtkBox *vbox = GTK_BOX (gtk_vbox_new (FALSE, 3));
  GtkBox *main_box = vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vbox), border);
  gtk_widget_show (GTK_WIDGET (vbox));

  if (! large)
    {
      gtk_scrolled_window_add_with_viewport
	(GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (vbox));
      gtk_viewport_set_shadow_type (GTK_VIEWPORT(GTK_WIDGET (vbox)->parent),
				    GTK_SHADOW_NONE);
    }
  else
    gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
			      GTK_WIDGET (vbox), label);

  /* Whether the event is an all day event.  */
  s->all_day
    = GTK_TOGGLE_BUTTON (gtk_check_button_new_with_label (_("All day event")));
  g_signal_connect (G_OBJECT (s->all_day), "toggled",
                    G_CALLBACK (all_day_toggled), s);
  gtk_box_pack_start (vbox, GTK_WIDGET (s->all_day), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->all_day));
  
  /* The summary.  */
  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Summary:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (label));

  s->summary = gtk_entry_new ();
  gtk_box_pack_start (hbox, s->summary, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (s->summary));

  /* We want the end of the labels to occur at the same horizontal
     point.  */
  GtkTable *table = GTK_TABLE (gtk_table_new (2, 2, FALSE));
  gtk_box_pack_start (vbox, GTK_WIDGET (table), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (table));

  /* Start time.  */
  label = gtk_label_new (_("Start:"));
  gtk_table_attach (table, label, 0, 1, 0, 1, GTK_SHRINK, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (label));

  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_table_attach (table, GTK_WIDGET (hbox), 1, 2, 0, 1,
		    GTK_EXPAND|GTK_FILL, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  s->starttime = gpe_time_sel_new ();
  g_signal_connect (G_OBJECT (s->starttime), "changed",
		    G_CALLBACK (note_time_change), s);
  gtk_box_pack_start (hbox, s->starttime, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->starttime));

  label = gtk_label_new (_("on:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (label));

  s->startdate = gtk_date_combo_new ();
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->startdate),
				     ! week_starts_sunday);
  g_signal_connect (G_OBJECT (s->startdate), "changed",
		    G_CALLBACK (note_date_change), s);
  gtk_box_pack_start (hbox, s->startdate, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->startdate));

  /* End time.  */
  label = gtk_label_new (_("End:"));
  gtk_table_attach (table, label, 0, 1, 1, 2,
		    GTK_SHRINK, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (label));

  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_table_attach (table, GTK_WIDGET (hbox), 1, 2, 1, 2,
		    GTK_EXPAND|GTK_FILL, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  s->endtime = gpe_time_sel_new ();
  g_signal_connect (G_OBJECT (s->endtime), "changed",
		    G_CALLBACK (sink_end_time), s);
  gtk_box_pack_start (hbox, s->endtime, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->endtime));

  label = gtk_label_new (_("on:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (label));

  s->enddate = gtk_date_combo_new ();
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->enddate),
				     ! week_starts_sunday);
  g_signal_connect (G_OBJECT (s->enddate), "changed",
		    G_CALLBACK (sink_end_date), s);
  gtk_box_pack_start (hbox, s->enddate, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->enddate));

  /* The calendar combo box.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Calendar:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  s->calendar = calendars_combo_box_new ();
  gtk_widget_show (s->calendar);
  gtk_box_pack_start (hbox, s->calendar, FALSE, FALSE, 0);

  GtkWidget *w = gtk_button_new_from_stock (GTK_STOCK_NEW);
  g_signal_connect (G_OBJECT (w), "clicked",
		    G_CALLBACK (new_calendar_clicked), s->calendar);
  gtk_widget_show (w);
  gtk_box_pack_start (hbox, w, FALSE, FALSE, 0);


  /* Detail page.  */
  label = gtk_label_new (_("Details"));
  gtk_widget_show (label);

  if (! large)
    {
      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				scrolled_window, label);
      gtk_widget_show (scrolled_window);
    }

  vbox = GTK_BOX (gtk_vbox_new (FALSE, 3));
  gtk_container_set_border_width (GTK_CONTAINER (vbox), border);
  gtk_widget_show (GTK_WIDGET (vbox));

  if (! large)
    {
      gtk_scrolled_window_add_with_viewport
	(GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (vbox));
      gtk_viewport_set_shadow_type (GTK_VIEWPORT(GTK_WIDGET (vbox)->parent),
				    GTK_SHADOW_NONE);
    }
  else
    gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
			      GTK_WIDGET (vbox), label);

  table = GTK_TABLE (gtk_table_new (2, 3, FALSE));
  gtk_table_set_col_spacings (table, 3);
  gtk_table_set_row_spacings (table, 3);
  gtk_box_pack_start (vbox, GTK_WIDGET (table), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (table));
  
  /* Location.  */
  label = gtk_label_new (_("Location:"));
  gtk_misc_set_alignment (GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, 
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  s->location = gtk_entry_new ();
  gtk_table_attach(table, s->location, 1, 2, 0, 1, 
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (s->location);

  /* Categories.  */
  GtkWidget *button = gtk_button_new_with_label (_("Categories"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (click_categories), s);
  gtk_table_attach (GTK_TABLE (table), button, 0, 1, 1, 2, 
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  s->categories_label = label = gtk_label_new
    (_("Define categories by tapping the Categories button."));
  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);  
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, 
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Description textarea.  */
  label = gtk_label_new (_("Description:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, 
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (vbox, scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  s->description = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (s->description), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (s->description), TRUE);
  g_signal_connect (G_OBJECT (s->description), "focus-in-event", 
		    G_CALLBACK (on_description_focus_in), window);
  g_signal_connect (G_OBJECT (s->description), "focus-out-event", 
		    G_CALLBACK (on_description_focus_out), window);
  gtk_container_add (GTK_CONTAINER (scrolled_window), s->description);
  gtk_widget_show (s->description);
  
  /* Alarm page */
  if (gdk_screen_height () <= 480)
    /* Give it its own page.  */
    {
      label = gtk_label_new (_("Alarm"));
      gtk_widget_show (label);

      if (! large)
	{
	  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	  gtk_scrolled_window_set_policy
	    (GTK_SCROLLED_WINDOW (scrolled_window),
	     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	  gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				    scrolled_window, label);
	  gtk_widget_show (scrolled_window);
	}

      vbox = GTK_BOX (gtk_vbox_new (FALSE, 3));
      gtk_container_set_border_width (GTK_CONTAINER (vbox), border);
      gtk_widget_show (GTK_WIDGET (vbox));

      if (! large)
	{
	  gtk_scrolled_window_add_with_viewport
	    (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (vbox));
	  gtk_viewport_set_shadow_type
	    (GTK_VIEWPORT(GTK_WIDGET (vbox)->parent), GTK_SHADOW_NONE);
	}
      else
	gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				  GTK_WIDGET (vbox), label);
    }
  else
    vbox = main_box;

  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  s->alarmbutton = gtk_check_button_new_with_label (_("Alarm"));
  gtk_box_pack_start (hbox, s->alarmbutton, FALSE, FALSE, 0);
  gtk_widget_show (s->alarmbutton);

  GtkWidget *spin = gtk_spin_button_new_with_range (1, 1000, 1);
  s->alarmspin = spin;
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 0);
  g_signal_connect (G_OBJECT (spin), "value-changed",
		    G_CALLBACK (set_toggle), s->alarmbutton);
  gtk_box_pack_start (hbox, spin, FALSE, FALSE, 0);
  gtk_widget_show (spin);

  s->alarm_units = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  gtk_combo_box_append_text (s->alarm_units, _("minutes"));
  gtk_combo_box_append_text (s->alarm_units, _("hours"));
  gtk_combo_box_append_text (s->alarm_units, _("days"));
  gtk_combo_box_append_text (s->alarm_units, _("weeks"));
  gtk_combo_box_set_active (s->alarm_units, 1);
  g_signal_connect (G_OBJECT (s->alarm_units), "changed",
		    G_CALLBACK (set_toggle), s->alarmbutton);
  gtk_box_pack_start (hbox, GTK_WIDGET (s->alarm_units), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->alarm_units));

  label = gtk_label_new (_("before event"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);


  /* Reccurence page.  */
  if (gdk_screen_height () <= 600)
    /* Give it its own page.  */
    {
      label = gtk_label_new (_("Recurrence"));
      gtk_widget_show (label);

      if (! large)
	{
	  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	  gtk_scrolled_window_set_policy
	    (GTK_SCROLLED_WINDOW (scrolled_window),
	     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	  gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				    scrolled_window, label);
	  gtk_widget_show (scrolled_window);
	}

      vbox = GTK_BOX (gtk_vbox_new (FALSE, 3));
      gtk_container_set_border_width (GTK_CONTAINER (vbox), border);
      gtk_widget_show (GTK_WIDGET (vbox));

      if (! large)
	{
	  gtk_scrolled_window_add_with_viewport
	    (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (vbox));
	  gtk_viewport_set_shadow_type
	    (GTK_VIEWPORT(GTK_WIDGET (vbox)->parent), GTK_SHADOW_NONE);
	}
      else
	gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				  GTK_WIDGET (vbox), label);
    }
  else
    vbox = main_box;

  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Recurrence type:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  GtkComboBox *combo = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  s->recur_type = combo;
  gtk_combo_box_append_text (combo, _("single occurrence"));
  gtk_combo_box_append_text (combo, _("daily"));
  gtk_combo_box_append_text (combo, _("weekly"));
  gtk_combo_box_append_text (combo, _("monthly"));
  gtk_combo_box_append_text (combo, _("yearly"));
  gtk_combo_box_set_active (combo, 1);
  g_signal_connect (G_OBJECT (combo), "changed",
                    G_CALLBACK (recur_combo_changed), s);
  gtk_box_pack_start (hbox, GTK_WIDGET (combo), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (combo));

  s->recur_box = GTK_CONTAINER (gtk_vbox_new (FALSE, 3));
  gtk_box_pack_start (vbox, GTK_WIDGET (s->recur_box), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->recur_box));
  vbox = GTK_BOX (s->recur_box);

  /* Every.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Every"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spin = gtk_spin_button_new_with_range (1, 1000, 1);
  s->increment_unit = GTK_SPIN_BUTTON (spin);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 0);
  gtk_box_pack_start (hbox, spin, FALSE, FALSE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (NULL);
  s->increment_unit_postfix = GTK_LABEL (label);
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* By day.  */
  table = GTK_TABLE (gtk_table_new (3, 3, FALSE));
  s->byday = GTK_CONTAINER (table);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (table), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (table));

  int i;
  for (i = 0; i < 7; i++)
    {
      char *gs = g_locale_to_utf8 (nl_langinfo(days[i]), -1, NULL, NULL, NULL);
      GtkWidget *b = gtk_check_button_new_with_label (gs);
      g_free (gs);
      s->checkbuttonwday[i] = b;
      g_signal_connect (G_OBJECT (b), "clicked", G_CALLBACK (sink_weekly), s);
      gtk_table_attach_defaults (GTK_TABLE (table), b,
				 i % 3, (i % 3) + 1, i / 3, (i / 3) + 1);
      gtk_widget_show (b);
    }

  /* Advanced by day.  */
#if 0
  snprintf (buf, sizeof(buf), _("on day %d"), 5);
  daybutton = gtk_radio_button_new_with_label (NULL, buf);

  radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (daybutton));
  weekbutton = gtk_radio_button_new_with_label (radiogroup, _("on the"));

  menu1 = gtk_menu_new ();
  menu2 = gtk_menu_new ();
  optionmenu1 = gtk_option_menu_new ();
  optionmenu2 = gtk_option_menu_new ();

  gtk_menu_append (GTK_MENU (menu1),
                   gtk_menu_item_new_with_label (_("first")));
  gtk_menu_append (GTK_MENU (menu1),
                   gtk_menu_item_new_with_label (_("second")));
  gtk_menu_append (GTK_MENU (menu1),
                   gtk_menu_item_new_with_label (_("third")));
  gtk_menu_append (GTK_MENU (menu1),
                   gtk_menu_item_new_with_label (_("fourth")));
  gtk_menu_append (GTK_MENU (menu1),
                   gtk_menu_item_new_with_label (_("fifth")));
  gtk_menu_append (GTK_MENU (menu1),
                   gtk_menu_item_new_with_label (_("last")));

  for (i = 0; i < 7; i++)
    gtk_menu_append (GTK_MENU (menu2),
                     gtk_menu_item_new_with_label (nl_langinfo(days[i])));

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), menu1);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu2), menu2);

  gtk_widget_set_usize (optionmenu1, 60, -1);
  gtk_widget_set_usize (optionmenu2, 60, -1);
#endif

  /* End date.  */

  GtkWidget *frame = gtk_frame_new (_("End Date"));
  gtk_box_pack_start (vbox, frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  {
    GtkBox *vbox = GTK_BOX (gtk_vbox_new (FALSE, 3));
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (vbox));
    gtk_widget_show (GTK_WIDGET (vbox));

    /* forever radio button */
    button = gtk_radio_button_new_with_label (NULL, _("forever"));
    s->radiobuttonforever = button;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    /* end after radio button */
    hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
    gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
    gtk_widget_show (GTK_WIDGET (hbox));

    button = gtk_radio_button_new_with_label_from_widget
      (GTK_RADIO_BUTTON (button), _("end after"));
    s->radiobuttonendafter = button;
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    spin = gtk_spin_button_new_with_range (1, 1000, 1);
    s->endafter = GTK_SPIN_BUTTON (spin);
    g_signal_connect (G_OBJECT (spin), "changed",
		      G_CALLBACK (set_toggle), button);
    gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
    gtk_widget_show (spin);

    label = gtk_label_new (_("occurrences"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    /* End on.  */
    hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
    gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
    gtk_widget_show (GTK_WIDGET (hbox));

    button = gtk_radio_button_new_with_label_from_widget
      (GTK_RADIO_BUTTON (button), _("end on"));
    s->radiobuttonendon = button; 
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    s->datecomboendon = gtk_date_combo_new ();
    g_signal_connect (G_OBJECT (s->datecomboendon), "changed",
		      G_CALLBACK (set_toggle), button);
    gtk_box_pack_start (GTK_BOX (hbox), s->datecomboendon, FALSE, FALSE, 0);
    gtk_widget_show (s->datecomboendon);
  }


  /* Button box.  */
  GtkBox *button_box = hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (GTK_BOX (window_box), GTK_WIDGET (hbox),
		      FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

#ifdef IS_HILDON
  button = gtk_button_new_with_label (_("Cancel"));
#else
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
  g_signal_connect_swapped (G_OBJECT (button), "clicked",
                            G_CALLBACK (edit_finished), s);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, boxspacing);
  gtk_widget_show (button);

#ifdef IS_HILDON
  button = gtk_button_new_with_label (_("Save"));
#else
  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
#endif
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (click_ok), s);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, boxspacing);
  gtk_widget_show (button);


  /* Misc.  */
  if (! large)
    {
      GtkRequisition main_box_req;
      gtk_widget_size_request (GTK_WIDGET (main_box), &main_box_req);
      GtkRequisition buttons_req;
      gtk_widget_size_request (GTK_WIDGET (button_box), &buttons_req);
      int width = MIN (main_box_req.width + 40, gdk_screen_width () - 40);
      int height = MIN (main_box_req.height + buttons_req.height + 60,
			gdk_screen_height () - 40);
      gtk_window_set_default_size (GTK_WINDOW (window), width, height);
    }

  /* It should be initially hidden but we want it to contribute to the
     size calculation.  */
  gtk_widget_hide (GTK_WIDGET (s->byday));

  gtk_widget_grab_focus (s->summary);

  g_signal_connect_swapped (G_OBJECT (window), "delete_event",
			    G_CALLBACK (edit_finished), s);

  g_signal_connect (G_OBJECT (window), "key_press_event", 
		            G_CALLBACK (event_ui_key_press_event), s);
  gtk_widget_add_events (GTK_WIDGET (window), 
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  
  GTK_WIDGET_SET_FLAGS (s->summary, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (s->summary);
  g_object_set_data(G_OBJECT (window), "default-entry", s->summary);
  
  return s;
}

static struct edit_state *
edit_event_window (void)
{
  struct edit_state *s;

  if (cached_edit_state)
    {
      s = cached_edit_state;
      cached_edit_state = NULL;
      if (cached_edit_state_destroy_source)
	{
	  g_source_remove (cached_edit_state_destroy_source);
	  cached_edit_state_destroy_source = 0;
	}
    }
  else
    s = build_edit_event_window ();

  return s;
}

GtkWidget *
new_event (time_t t)
{
  struct edit_state *s = edit_event_window ();
  if (! s)
    return NULL;

  gtk_window_set_title (s->window, _("Calendar: New event"));

  s->ev = NULL;

  gtk_text_buffer_set_text
    (gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description)), "", -1);
  gtk_entry_set_text (GTK_ENTRY (s->summary), "");
  gtk_entry_set_text (GTK_ENTRY (s->location), "");
  update_categories (GTK_WIDGET (s->window), NULL, s);

  struct tm tm;
  localtime_r (&t, &tm);
  gpe_time_sel_set_time (GPE_TIME_SEL (s->starttime), tm.tm_hour, tm.tm_min);
  gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
			   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
  t += 60 * 60;
  localtime_r (&t, &tm);
  gpe_time_sel_set_time (GPE_TIME_SEL (s->endtime), tm.tm_hour, tm.tm_min);
  gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
			   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), FALSE);
  gtk_combo_box_set_active (s->recur_type, 0);
  gtk_notebook_set_page (GTK_NOTEBOOK (s->tabs), 0);
  gtk_entry_set_text (GTK_ENTRY (s->summary), "");
  gtk_text_buffer_set_text
    (gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description)), "", -1);

  gtk_toggle_button_set_active (s->all_day, FALSE);

  GDate date;
  g_date_set_dmy (&date, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
  int wday = g_date_weekday (&date);

  int i;
  for (i = 0; i < 7; i++)
    gtk_toggle_button_set_active
      (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), i == wday - 1);

  s->recur_day_floating = TRUE;
  s->end_date_floating = TRUE;
  s->end_time_floating = TRUE;

  GtkWidget *entry = g_object_get_data (G_OBJECT (s->window), "default-entry");
  if (entry)
    {
      gtk_widget_grab_focus (entry);
      gtk_editable_select_region (GTK_EDITABLE (entry), 0, 0);
    }

  return GTK_WIDGET (s->window);
}

GtkWidget *
edit_event (Event *ev)
{
  struct edit_state *s = edit_event_window ();
  if (! s)
    return NULL;

  s->ev = ev;

  gtk_window_set_title (s->window, _("Calendar: Edit event"));

  char *str = event_get_description (ev);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer
			    (GTK_TEXT_VIEW (s->description)),
			    str ?: "", -1);
  g_free (str);
  str = event_get_summary (ev);
  gtk_entry_set_text (GTK_ENTRY (s->summary), str ?: "");
  g_free (str);
  str = event_get_location (ev);
  gtk_entry_set_text (GTK_ENTRY (s->location), str ?: "");
  g_free (str);
  GSList *list = event_get_categories (ev);
  update_categories (GTK_WIDGET (s->window), list, s);
  g_slist_free (list);

  struct tm tm;
  time_t start = event_get_recurrence_start (ev);
  localtime_r (&start, &tm);
  gpe_time_sel_set_time (GPE_TIME_SEL (s->starttime), tm.tm_hour, tm.tm_min);
  gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
			   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

  time_t end = start + event_get_duration (ev);
  localtime_r (&end, &tm);
  gpe_time_sel_set_time (GPE_TIME_SEL (s->endtime), tm.tm_hour, tm.tm_min);
  gtk_notebook_set_page (GTK_NOTEBOOK (s->tabs), 0);
  gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
			   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

  gtk_toggle_button_set_active (s->all_day, event_get_untimed (ev));

  /* Calendar.  */
  EventCalendar *c = event_get_calendar (ev);
  calendars_combo_box_set_active (GTK_COMBO_BOX (s->calendar), c);
  g_object_unref (c);

  if (event_get_alarm (ev))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), TRUE);

      int unit = 0;
      int i;
      for (i = 0; i < 4; i++)
	if ((event_get_alarm (ev) % alarm_multipliers[i]) == 0)
	  unit = i;
	else
	  break;
	  
      gtk_spin_button_set_value
	(GTK_SPIN_BUTTON (s->alarmspin),
	 event_get_alarm (ev) / alarm_multipliers[unit]);
      gtk_combo_box_set_active (s->alarm_units, unit);
    }
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), FALSE);

  int i = 0;
  switch (event_get_recurrence_type (ev))
    {
    case RECUR_NONE:
      i = 0;
      break;
    case RECUR_DAILY:
      i = 1;
      break;
    case RECUR_WEEKLY:
      i = 2;
      break;
    case RECUR_MONTHLY:
      i = 3;
      break;
    case RECUR_YEARLY:
      i = 4;
      break;
    }
  gtk_combo_box_set_active (s->recur_type, i);

  gtk_spin_button_set_value (s->increment_unit,
			     event_get_recurrence_increment (ev));

  if (event_get_recurrence_type (ev) == RECUR_WEEKLY)
    {
      int daymask = event_get_recurrence_daymask (ev);
      for (i = 0; i < 7; i ++)
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), (daymask & (1 << i)));
    }
  else
    for (i = 0; i < 7; i++)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]),
				    FALSE);

  /* Turn all of the buttons off.  */
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (s->radiobuttonendon), FALSE);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (s->radiobuttonendafter), FALSE);
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (s->radiobuttonforever), FALSE);

  if (event_get_recurrence_type (ev) != RECUR_NONE)
    /* Then turn the right one on.  */
    {
      time_t end = event_get_recurrence_end (ev);
      if (end)
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonendon), TRUE);
      else if (event_get_recurrence_count (ev))
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonendafter), TRUE);
      else
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonforever), TRUE);

      if (end)
	{
	  localtime_r (&end, &tm);
	  gtk_date_combo_set_date (GTK_DATE_COMBO (s->datecomboendon),
				   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
	}
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->endafter),
				 event_get_recurrence_count (ev));
    }

  return GTK_WIDGET (s->window);
}
