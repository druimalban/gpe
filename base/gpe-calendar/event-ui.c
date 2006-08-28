/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 * Copyright (C) 2005, 2006 Florian Boor <fb@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <langinfo.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/errorbox.h>
#include <gpe/event-db.h>
#include <gpe/spacing.h>
#include <gpe/pim-categories.h>
#include <gpe/pim-categories-ui.h>
#include <gpe/gpetimesel.h>

#include "calendars-widgets.h"
#include "globals.h"
#include "event-ui.h"
#include "calendar-edit-dialog.h"

#ifdef IS_HILDON
#include <hildon-widgets/hildon-number-editor.h>
#include <hildon-widgets/hildon-time-editor.h>
#include <hildon-widgets/hildon-date-editor.h>
#include <hildon-widgets/hildon-note.h>
#endif

struct edit_state
{
  GtkWindow *window;

  GtkWidget *tabs;

  GtkToggleButton *all_day;
  GtkWidget *startdate, *enddate;
  GtkWidget *starttime, *endtime;
  GtkWidget *calendar;

  /* Alarm page.  */
  guint alarm_page;
  GtkBox *alarm_box;
  GtkWidget *alarmbutton;
  GtkWidget *alarmspin;
  GtkComboBox *alarm_units;

  /* Description page.  */
  guint detail_page;
  GtkBox *detail_box;
  GtkWidget *description;
  GtkWidget *summary;
  GtkWidget *location;
  GSList *categories;
  GtkWidget *categories_label;


  /* Recurrence page.  */
  guint recurrence_page;
  GtkBox *recurrence_box;
  GtkComboBox *recur_type;

  GtkContainer *recur_box;

  GtkWidget *increment_unit;
  GtkLabel *increment_unit_postfix;

  GtkContainer *bydayweekly;
  GtkWidget *checkbuttonwday[7];

  GtkBox *bydaymonthly;
  /* List of struct bydaymonthly *.  */
  GSList *days;

  GtkWidget *radiobuttonforever, *radiobuttonendafter,
            *radiobuttonendon, *datecomboendon;
  GtkWidget *endafter;

  /* The event we are editing.  If NULL then creating a new event.  */
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
  if (s->ev)
    g_object_unref (s->ev);

  g_slist_free (s->categories);

  /* Free the list of days.  */
  GSList *i;
  for (i = s->days; i; i = i->next)
    g_free (i->data);
  g_slist_free (s->days);

  g_free (s);
}

struct bydaymonthly
{
  GtkContainer *cont;
  GtkComboBox *freq;
  GtkComboBox *day;
  gulong change_signal;
};

static struct bydaymonthly *bydaymonthly_new (struct edit_state *s,
					      gboolean create_new_signal);

/* A bydaymonthly combo changed.  Make a new one.  */
static void
day_combo_changed (GtkComboBox *combo, struct edit_state *s)
{
  if (gtk_combo_box_get_active (combo) == -1)
    return;

  g_signal_handlers_disconnect_by_func (combo, day_combo_changed, s);

  bydaymonthly_new (s, TRUE);
}

/* Creates a bydaymonthly box.  */
static struct bydaymonthly *
bydaymonthly_new (struct edit_state *s, gboolean create_new_signal)
{
  struct bydaymonthly *bdm = g_malloc (sizeof (*bdm));

  GtkBox *box = GTK_BOX (gtk_hbox_new (FALSE, 0));
  bdm->cont = GTK_CONTAINER (box);
  gtk_box_pack_start (s->bydaymonthly, GTK_WIDGET (box), FALSE, FALSE, 3);
  gtk_widget_show (GTK_WIDGET (box));

  GtkComboBox *combo = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  bdm->freq = combo;
  gtk_combo_box_append_text (combo, _("Every"));
  gtk_combo_box_append_text (combo, _("First"));
  gtk_combo_box_append_text (combo, _("Second"));
  gtk_combo_box_append_text (combo, _("Third"));
  gtk_combo_box_append_text (combo, _("Fourth"));
  gtk_combo_box_append_text (combo, _("Last"));
  gtk_combo_box_append_text (combo, _("Second to last"));
  gtk_combo_box_set_active (combo, 0);
  gtk_box_pack_start (box, GTK_WIDGET (combo), FALSE, FALSE, 3);
  gtk_widget_show (GTK_WIDGET (combo));

  const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5,
			   ABDAY_6, ABDAY_7, ABDAY_1 };

  combo = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  bdm->day = combo;
  int i;
  gtk_combo_box_append_text (combo, "");
  for (i = 0; i < 7; i ++)
    gtk_combo_box_append_text (combo, nl_langinfo(days[i]));
  if (create_new_signal)
    g_signal_connect (G_OBJECT (combo), "changed",
		      G_CALLBACK (day_combo_changed), s);
  gtk_box_pack_start (box, GTK_WIDGET (combo), FALSE, FALSE, 3);
  gtk_widget_show (GTK_WIDGET (combo));

  s->days = g_slist_prepend (s->days, bdm);

  return bdm;
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
    gtk_widget_show (GTK_WIDGET (s->bydayweekly));
  else
    gtk_widget_hide (GTK_WIDGET (s->bydayweekly));

  if (gtk_combo_box_get_active (s->recur_type) == 3)
    {
      if (! s->days)
	bydaymonthly_new (s, TRUE);

      gtk_widget_show (GTK_WIDGET (s->bydaymonthly));
    }
  else if (s->bydaymonthly)
    gtk_widget_hide (GTK_WIDGET (s->bydaymonthly));
}

static void
click_ok (GtkWidget *widget, struct edit_state *s)
{
  struct tm tm_start, tm_end, tm_rend;
  time_t start_t, end_t, rend_t;
  GtkTextIter start, end;

  memset (&tm_start, 0, sizeof (struct tm));
#ifdef IS_HILDON
  hildon_date_editor_get_date (HILDON_DATE_EDITOR (s->startdate), 
                  (guint *) &tm_start.tm_year, (guint *) &tm_start.tm_mon, 
                  (guint *) &tm_start.tm_mday);
  tm_start.tm_year -= 1900;
  tm_start.tm_mon -= 1;
#else
  tm_start.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
  tm_start.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
  tm_start.tm_mday = GTK_DATE_COMBO (s->startdate)->day;
#endif
  tm_start.tm_isdst = -1;

  memset (&tm_end, 0, sizeof (struct tm));
#ifdef IS_HILDON
  hildon_date_editor_get_date (HILDON_DATE_EDITOR (s->enddate), 
                  &tm_end.tm_year, &tm_end.tm_mon, &tm_end.tm_mday);
  tm_end.tm_year -= 1900;
  tm_end.tm_mon -= 1;
#else
  tm_end.tm_year = GTK_DATE_COMBO (s->enddate)->year - 1900;
  tm_end.tm_mon = GTK_DATE_COMBO (s->enddate)->month;
  tm_end.tm_mday = GTK_DATE_COMBO (s->enddate)->day;
#endif
  tm_end.tm_isdst = -1;

  if (! gtk_toggle_button_get_active (s->all_day))
    /* Appointment */
    {
#ifdef IS_HILDON
      guint sec;
      hildon_time_editor_get_time (HILDON_TIME_EDITOR (s->starttime), 
                                   (guint *) &tm_start.tm_hour,
			                       (guint *) &tm_start.tm_min, &sec);

      hildon_time_editor_get_time (HILDON_TIME_EDITOR (s->endtime), 
                                   (guint *) &tm_end.tm_hour,
			                       (guint *) &tm_end.tm_min, &sec);
#else        
      gpe_time_sel_get_time (GPE_TIME_SEL (s->starttime),
			     (guint *) &tm_start.tm_hour,
			     (guint *) &tm_start.tm_min);
      gpe_time_sel_get_time (GPE_TIME_SEL (s->endtime),
			     (guint *) &tm_end.tm_hour,
			     (guint *) &tm_end.tm_min);
#endif
      start_t = mktime (&tm_start);
      end_t = mktime (&tm_end) + 1;
    }
  else
    /* All day event.  */
    {
      start_t = mktime (&tm_start);
      end_t = mktime (&tm_end) + 24 * 60 * 60;
    }

  if (end_t < start_t)
    {
      gpe_error_box (_("End time must not be earlier than start time"));
      return;
    }

  if (start_t < time (NULL))
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
      gtk_widget_destroy (dialog);
      if ((ret == GTK_RESPONSE_NO) || (ret == GTK_RESPONSE_CANCEL))
	return;
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
    s->ev = ev = event_new (event_db, ec, NULL);

  event_set_recurrence_start (ev, start_t);
  event_set_duration (ev, end_t - start_t);
  event_set_untimed (ev, gtk_toggle_button_get_active (s->all_day));

  if (s->description)
    {
      GtkTextBuffer *buf;

      buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description));
      gtk_text_buffer_get_bounds (buf, &start, &end);
      char *text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
      event_set_description (ev, text);
      g_free (text);
    }

  if (s->summary)
    {
      char *text = gtk_editable_get_chars (GTK_EDITABLE (s->summary), 0, -1);
      event_set_summary (ev, text);
      g_free (text);
    }

  if (s->location)
    {
      char *text = gtk_editable_get_chars (GTK_EDITABLE (s->location), 0, -1);
      event_set_location (ev, text);
      g_free (text);
    }

  if (s->categories)
    event_set_categories (ev, g_slist_copy (s->categories));

  if (s->alarmbutton)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
	{
	  gint mi = gtk_combo_box_get_active (s->alarm_units);
#ifdef IS_HILDON
      gint as = 
         hildon_number_editor_get_value (HILDON_NUMBER_EDITOR (s->alarmspin));
#else
	  gint as = 
         gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->alarmspin));
#endif        
	  event_set_alarm (ev, alarm_multipliers[mi] * as);
	}
      else
	event_set_alarm (ev, 0);
    }

  if (s->recur_type)
    {
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
#ifdef IS_HILDON
        gint iu =
            hildon_number_editor_get_value (
                           HILDON_NUMBER_EDITOR (s->increment_unit));
#else
        gint iu = 
             gtk_spin_button_get_value_as_int (s->increment_unit);
#endif
      event_set_recurrence_increment (ev, iu);
	  if (gtk_toggle_button_get_active
	      (GTK_TOGGLE_BUTTON (s->radiobuttonforever)))
	    {
	      event_set_recurrence_count (ev, 0);
	      event_set_recurrence_end (ev, 0);
	    }
	  else if (gtk_toggle_button_get_active
		   (GTK_TOGGLE_BUTTON (s->radiobuttonendafter)))
	    {
#ifdef IS_HILDON
          gint ea = hildon_number_editor_get_value 
                               (HILDON_NUMBER_EDITOR (s->endafter));
#else            
	      gint ea = gtk_spin_button_get_value_as_int (s->endafter);
#endif
	      event_set_recurrence_count (ev, ea);
          event_set_recurrence_end (ev, 0);
	    }

	  else if (gtk_toggle_button_get_active
		   (GTK_TOGGLE_BUTTON (s->radiobuttonendon)))
	    {
	      memset (&tm_rend, 0, sizeof (struct tm));

#ifdef IS_HILDON
          hildon_date_editor_get_date (HILDON_DATE_EDITOR (s->datecomboendon),
                     (guint *) &tm_rend.tm_year, 
                     (guint *) &tm_rend.tm_mon, (guint *)&tm_rend.tm_mday);
          tm_rend.tm_year -= 1900;
          tm_rend.tm_mon -= 1;
          guint sec;
          hildon_time_editor_get_time (HILDON_TIME_EDITOR (s->endtime), 
                                   (guint *) &tm_rend.tm_hour,
			                       (guint *) &tm_rend.tm_min, &sec);
#else        
	      tm_rend.tm_year = GTK_DATE_COMBO (s->datecomboendon)->year - 1900;
	      tm_rend.tm_mon = GTK_DATE_COMBO (s->datecomboendon)->month;
	      tm_rend.tm_mday = GTK_DATE_COMBO (s->datecomboendon)->day;
	      gpe_time_sel_get_time (GPE_TIME_SEL (s->endtime),
				     (guint *)&tm_rend.tm_hour,
				     (guint *)&tm_rend.tm_min);
#endif
	      tm_rend.tm_isdst = -1;
	      rend_t = mktime (&tm_rend);

	      event_set_recurrence_count (ev, 0);
	      event_set_recurrence_end (ev, rend_t);
	    }
	}

      const char *days[] = { "MO", "TU", "WE", "TH", "FR", "SA", "SU" };

      if (recur == RECUR_WEEKLY)
	{
	  GSList *byday = NULL;

	  int i;
	  for (i = 0; i < 7; i++)
	    if (gtk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON (s->checkbuttonwday[i])))
	      byday = g_slist_prepend (byday, g_strdup (days[i]));

	  event_set_recurrence_byday (ev, byday);
	}
      if (recur == RECUR_MONTHLY)
	{
	  GSList *byday = NULL;
	  GSList *l;
	  for (l = s->days; l; l = l->next)
	    {
	      struct bydaymonthly *bdm = l->data;
	      int freq = gtk_combo_box_get_active (bdm->freq);
	      int day = gtk_combo_box_get_active (bdm->day);
	      /* The first entry is a blank.  */
	      day --;

	      if (freq < 0 || day <= 0)
		continue;

	      if (freq == 5)
		/* Last.  */
		freq = -1;
	      else if (freq == 6)
		/* Second to last.  */
		freq = -2;

	      char *s;
	      if (freq == 0)
		s = g_strdup (days[day]);
	      else
		s = g_strdup_printf ("%d%s", freq, days[day]);
	      byday = g_slist_prepend (byday, s);
	    }

	  event_set_recurrence_byday (ev, byday);
	}
    }

  event_flush (ev);
  gtk_widget_destroy (GTK_WIDGET (s->window));
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
      gtk_widget_destroy (GTK_WIDGET (s->window));
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

#ifdef IS_HILDON
static void
note_time_change (GtkWidget *widget, struct edit_state *s)
{
  guint hour, minute, sec;
    
  hildon_time_editor_get_time (HILDON_TIME_EDITOR (widget), 
                               &hour, &minute, &sec); 
    
  if (s->end_time_floating)
    {
      struct tm tm;
      memset (&tm, 0, sizeof (struct tm));
      hildon_date_editor_get_date (HILDON_DATE_EDITOR (s->startdate),
                                   (guint *) &tm.tm_year, 
                                   (guint *) &tm.tm_mon, (guint *) &tm.tm_mday);
      tm.tm_year -= 1900;
      tm.tm_mon -= 1;
      tm.tm_hour = hour;
      tm.tm_min = minute;
      tm.tm_isdst = -1;
      time_t t = mktime (&tm) + 60 * 60;
      localtime_r (&t, &tm);

      hildon_date_editor_set_date (HILDON_DATE_EDITOR (s->enddate),
			       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

      hildon_time_editor_set_time (HILDON_TIME_EDITOR (s->endtime), 
                                   tm.tm_hour, tm.tm_min, 0);
    }
}

static void
note_date_change (HildonDateEditor *c, GdkEventFocus *event, struct edit_state *s)
{
  if (s->recur_day_floating && s->checkbuttonwday[0])
    {
      GDate date;
      guint day, month, year;
        
      hildon_date_editor_get_date (c, &year, &month, &day);
      g_date_set_dmy (&date, day, month, year);
      int wday = g_date_weekday (&date);

      int i;
      for (i = 0; i < 7; i++)
          gtk_toggle_button_set_active
	         (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), i == wday - 1);

      /* Changing the buttons--even programmatically--will cause the
	 toggle's changed signal to be emitted thereby causing the
	 recur_day_floating to be set to false.  Reset it.  */
      s->recur_day_floating = TRUE;
    }
}

#else

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
  if (s->recur_day_floating && s->checkbuttonwday[0])
    {
      GDate date;
      g_date_set_dmy (&date, c->day, c->month + 1, c->year);
      int wday = g_date_weekday (&date);

      int i;
      for (i = 0; i < 7; i++)
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), i == wday - 1);

      /* Changing the buttons--even programmatically--will cause the
	 toggle's changed signal to be emitted thereby causing the
	 recur_day_floating to be set to false.  Reset it.  */
      s->recur_day_floating = TRUE;
    }
}
#endif

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

#ifdef IS_HILDON
static void
set_toggle_focus (GtkWidget *widget, GdkEventFocus *ev, GtkToggleButton *toggle)
{
  gtk_toggle_button_set_active (toggle, TRUE);
}
#endif

static void
set_toggle (GtkWidget *widget, GtkToggleButton *toggle)
{
  gtk_toggle_button_set_active (toggle, TRUE);
}

static void
build_detail_page (struct edit_state *s)
{
  g_assert (! s->location);

  GtkBox *vbox = s->detail_box;
  gtk_widget_show (GTK_WIDGET (vbox));

  GtkTable *table = GTK_TABLE (gtk_table_new (2, 3, FALSE));
  gtk_table_set_col_spacings (table, gpe_get_boxspacing());
  gtk_table_set_row_spacings (table, gpe_get_boxspacing());
  gtk_box_pack_start (vbox, GTK_WIDGET (table), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (table));
  
  /* Location.  */
  GtkWidget *label = gtk_label_new (_("Location:"));
  gtk_misc_set_alignment (GTK_MISC(label), 0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 0, 1, 0, 1, 
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  s->location = gtk_entry_new ();
  if (s->ev)
    {
      char *str = event_get_location (s->ev);
      gtk_entry_set_text (GTK_ENTRY (s->location), str ?: "");
      g_free (str);
    }
  gtk_table_attach (table, s->location, 1, 2, 0, 1, 
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
  if (s->ev)
    {
      GSList *list = event_get_categories (s->ev);
      update_categories (GTK_WIDGET (s->window), list, s);
      g_slist_free (list);
    }
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, 
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Description textarea.  */
  label = gtk_label_new (_("Description:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);  
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 2, 3, 
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  GtkWidget *frame = gtk_frame_new (NULL);
  gtk_box_pack_start (vbox, frame, TRUE, TRUE, gpe_get_boxspacing());
  gtk_widget_show (frame);

  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  s->description = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (s->description), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (s->description), TRUE);
  if (s->ev)
    {
      char *str = event_get_description (s->ev);
      gtk_text_buffer_set_text (gtk_text_view_get_buffer
				(GTK_TEXT_VIEW (s->description)),
				str ?: "", -1);
      g_free (str);
    }
  g_signal_connect (G_OBJECT (s->description), "focus-in-event", 
		    G_CALLBACK (on_description_focus_in), s->window);
  g_signal_connect (G_OBJECT (s->description), "focus-out-event", 
		    G_CALLBACK (on_description_focus_out), s->window);
  gtk_container_add (GTK_CONTAINER (scrolled_window), s->description);
  gtk_widget_show (s->description);
}

static void
build_alarm_page (struct edit_state *s)
{
  g_assert (! s->alarmbutton);

  GtkBox *vbox = s->alarm_box;
  gtk_widget_show (GTK_WIDGET (vbox));

  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, gpe_get_boxspacing()));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  s->alarmbutton = gtk_check_button_new_with_label (_("Alarm"));
  gtk_box_pack_start (hbox, s->alarmbutton, FALSE, FALSE, 0);
  gtk_widget_show (s->alarmbutton);

#ifdef IS_HILDON
  GtkWidget *spin = hildon_number_editor_new (1, 999);
  g_signal_connect (G_OBJECT (spin), "focus-in-event",
		    G_CALLBACK (set_toggle_focus), s->alarmbutton);
#else    
  GtkWidget *spin = gtk_spin_button_new_with_range (1, 1000, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 0);
  g_signal_connect (G_OBJECT (spin), "value-changed",
		    G_CALLBACK (set_toggle), s->alarmbutton);
#endif
    
  s->alarmspin = spin;
  gtk_box_pack_start (hbox, spin, FALSE, TRUE, 0);
  gtk_widget_show (spin);

  s->alarm_units = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  gtk_combo_box_append_text (s->alarm_units, _("minutes"));
  gtk_combo_box_append_text (s->alarm_units, _("hours"));
  gtk_combo_box_append_text (s->alarm_units, _("days"));
  gtk_combo_box_append_text (s->alarm_units, _("weeks"));
  gtk_combo_box_set_active (s->alarm_units, 1);
  g_signal_connect (G_OBJECT (s->alarm_units), "changed",
		    G_CALLBACK (set_toggle), s->alarmbutton);
  gtk_box_pack_start (hbox, GTK_WIDGET (s->alarm_units), FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (s->alarm_units));
#ifdef IS_HILDON
  gtk_widget_set_size_request (GTK_WIDGET(s->alarm_units), 110, -1);
#endif

  GtkWidget *label = gtk_label_new (_("before event"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (hbox, label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  if (s->ev && event_get_alarm (s->ev))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), TRUE);

      int i;
      int unit = 0;
      for (i = 0; i < 4; i++)
      if ((event_get_alarm (s->ev) % alarm_multipliers[i]) == 0)
        unit = i;
      else
        break;

#ifdef IS_HILDON
      hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (s->alarmspin),
                event_get_alarm (s->ev) / alarm_multipliers[unit]);
#else
      gtk_spin_button_set_value	(GTK_SPIN_BUTTON (s->alarmspin),
	            event_get_alarm (s->ev) / alarm_multipliers[unit]);
#endif      
      gtk_combo_box_set_active (s->alarm_units, unit);
    }
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), FALSE);
}

static void
build_recurrence_page (struct edit_state *s)
{
  g_assert (! s->recur_type);

  GtkBox *vbox = s->recurrence_box;
  gtk_widget_show (GTK_WIDGET (vbox));

  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, gpe_get_boxspacing()));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  GtkWidget *label = gtk_label_new (_("Recurrence type:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  GtkComboBox *combo = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  s->recur_type = combo;
  gtk_combo_box_append_text (combo, _("single occurrence"));
  gtk_combo_box_append_text (combo, _("daily"));
  gtk_combo_box_append_text (combo, _("weekly"));
  gtk_combo_box_append_text (combo, _("monthly"));
  gtk_combo_box_append_text (combo, _("yearly"));
  gtk_box_pack_start (hbox, GTK_WIDGET (combo), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (combo));
  int i = 0;
  if (s->ev)
    switch (event_get_recurrence_type (s->ev))
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
  g_signal_connect (G_OBJECT (combo), "changed",
                    G_CALLBACK (recur_combo_changed), s);

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

#ifdef IS_HILDON
  GtkWidget *spin = hildon_number_editor_new (1, 1000);
  if (s->ev)
    hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (s->increment_unit),
			       event_get_recurrence_increment (s->ev));
#else
  GtkWidget *spin = gtk_spin_button_new_with_range (1, 1000, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 0);
  if (s->ev)
    gtk_spin_button_set_value (spin,
			       event_get_recurrence_increment (s->ev));
#endif
  s->increment_unit = spin;
  gtk_box_pack_start (hbox, spin, FALSE, FALSE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (NULL);
  s->increment_unit_postfix = GTK_LABEL (label);
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);


  /* By day (weekly).  */
  s->bydayweekly = GTK_CONTAINER (gtk_hbox_new (FALSE, 0));
  if (gdk_screen_width () <= 500)
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (s->bydayweekly),
			FALSE, FALSE, 0);
  else
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (s->bydayweekly),
			FALSE, FALSE, 0);
  hbox = GTK_BOX (s->bydayweekly);
  /* Don't show.  */

  const char *days[]
    = { _("M"), _("T"), _("W"), _("T"), _("F"), _("S"), _("S") };
  for (i = 0; i < 7; i++)
    {
      GtkWidget *b = gtk_toggle_button_new_with_label (days[i]);
      s->checkbuttonwday[i] = b;
      g_signal_connect (G_OBJECT (b), "clicked", G_CALLBACK (sink_weekly), s);
      gtk_box_pack_start (hbox, b, FALSE, FALSE, 3);
      gtk_widget_show (b);
    }
  if (s->ev && event_get_recurrence_type (s->ev) == RECUR_WEEKLY)
    {
      GSList *byday = event_get_recurrence_byday (s->ev);
      GSList *l;
      for (l = byday; l; l = l->next)
	{
	  int day = -1;

	  if (strcmp (l->data, "MO") == 0)
	    day = 0;
	  else if (strcmp (l->data, "TU") == 0)
	    day = 1;
	  else if (strcmp (l->data, "WE") == 0)
	    day = 2;
	  else if (strcmp (l->data, "TH") == 0)
	    day = 3;
	  else if (strcmp (l->data, "FR") == 0)
	    day = 4;
	  else if (strcmp (l->data, "SA") == 0)
	    day = 5;
	  else if (strcmp (l->data, "SU") == 0)
	    day = 6;

	  if (day == -1)
	    {
	      g_warning ("%s: Unable to parse %s, ignoring",
			 __func__, (char *) l->data);
	      continue;
	    }

	  gtk_toggle_button_set_active
	    (GTK_TOGGLE_BUTTON (s->checkbuttonwday[day]), TRUE);
	}
      event_recurrence_byday_free (byday);

      gtk_widget_show (GTK_WIDGET (hbox));
    }
  else
#ifdef IS_HILDON      
    note_date_change (HILDON_DATE_EDITOR (s->startdate), NULL, s);
#else
    note_date_change (GTK_DATE_COMBO (s->startdate), s);
#endif
  /* By day (monthly).  */
  s->bydaymonthly = GTK_BOX (gtk_vbox_new (FALSE, 0));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (s->bydaymonthly),
		      FALSE, FALSE, 0);
  /* Don't show it.  */

  if (s->ev && event_get_recurrence_type (s->ev) == RECUR_MONTHLY)
    {
      GSList *byday = event_get_recurrence_byday (s->ev);
      GSList *l;
      for (l = byday; l; l = l->next)
	{
	  char *tail;
	  int prefix = strtol (l->data, &tail, 10);

	  int day = -1;
	  if (prefix < -2 || prefix > 4)
	    /* XXX: We'd like to not ignore it, but... */
	    continue;

	  if (strcmp (tail, "MO") == 0)
	    day = 0;
	  else if (strcmp (tail, "TU") == 0)
	    day = 1;
	  else if (strcmp (tail, "WE") == 0)
	    day = 2;
	  else if (strcmp (tail, "TH") == 0)
	    day = 3;
	  else if (strcmp (tail, "FR") == 0)
	    day = 4;
	  else if (strcmp (tail, "SA") == 0)
	    day = 5;
	  else if (strcmp (tail, "SU") == 0)
	    day = 6;

	  if (day == -1)
	    {
	      g_warning ("%s: Unable to parse %s, ignoring",
			 __func__, (char *) l->data);
	      continue;
	    }

	  struct bydaymonthly *bdm = bydaymonthly_new (s, FALSE);
	  if (prefix == -1)
	    gtk_combo_box_set_active (bdm->freq, 5);
	  else if (prefix == -2)
	    gtk_combo_box_set_active (bdm->freq, 6);
	  else
	    gtk_combo_box_set_active (bdm->freq, prefix);

	  /* The first entry is a blank.  */
	  day ++;
	  gtk_combo_box_set_active (bdm->day, day);
	}
      event_recurrence_byday_free (byday);

      /* Create an extra one to allow the user to enter an additional
	 day.  */
      bydaymonthly_new (s, TRUE);
    }

  /* End date.  */

  GtkWidget *frame = gtk_frame_new (NULL);
  gtk_box_pack_start (vbox, frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  GtkWidget *button;
  {
    GtkBox *vbox = GTK_BOX (gtk_vbox_new (FALSE, gpe_get_boxspacing()));
    gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (vbox));
    gtk_widget_show (GTK_WIDGET (vbox));

    /* forever radio button */
    button = gtk_radio_button_new_with_label (NULL, _("forever"));
    s->radiobuttonforever = button;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    /* end after radio button */
    hbox = GTK_BOX (gtk_hbox_new (FALSE, gpe_get_boxspacing()));
    gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
    gtk_widget_show (GTK_WIDGET (hbox));

    button = gtk_radio_button_new_with_label_from_widget
      (GTK_RADIO_BUTTON (button), _("end after"));
    s->radiobuttonendafter = button;
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

#ifdef IS_HILDON
    spin = hildon_number_editor_new (1, 1000);
    g_signal_connect (G_OBJECT (spin), "focus-in-event",
		      G_CALLBACK (set_toggle_focus), button);
#else
    spin = gtk_spin_button_new_with_range (1, 1000, 1);
    g_signal_connect (G_OBJECT (spin), "changed",
		      G_CALLBACK (set_toggle), button);
#endif
    s->endafter = spin;
    gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
    gtk_widget_show (spin);

    label = gtk_label_new (_("occurrences"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    /* End on.  */
    hbox = GTK_BOX (gtk_hbox_new (FALSE, gpe_get_boxspacing()));
    gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
    gtk_widget_show (GTK_WIDGET (hbox));

    button = gtk_radio_button_new_with_label_from_widget
      (GTK_RADIO_BUTTON (button), _("end on"));
    s->radiobuttonendon = button; 
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

#ifdef IS_HILDON
    s->datecomboendon = hildon_date_editor_new();
#else
    s->datecomboendon = gtk_date_combo_new ();
    g_signal_connect (G_OBJECT (s->datecomboendon), "changed",
		      G_CALLBACK (set_toggle), button);
#endif
    gtk_box_pack_start (GTK_BOX (hbox), s->datecomboendon, FALSE, FALSE, 0);
    gtk_widget_show (s->datecomboendon);
  }

  if (s->ev && event_get_recurrence_type (s->ev) != RECUR_NONE)
    /* Then turn the right one on.  */
    {
      time_t end = event_get_recurrence_end (s->ev);
      if (end)
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonendon), TRUE);
      else if (event_get_recurrence_count (s->ev))
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonendafter), TRUE);
      else
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonforever), TRUE);

#ifdef IS_HILDON
      if (end)
        {
          struct tm tm;
          localtime_r (&end, &tm);
          hildon_date_editor_set_date (HILDON_DATE_EDITOR (s->datecomboendon), 
                                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        }
      hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (s->endafter),
				 event_get_recurrence_count (s->ev));
#else
      if (end)
        {
          struct tm tm;
          localtime_r (&end, &tm);
          gtk_date_combo_set_date (GTK_DATE_COMBO (s->datecomboendon),
                       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
        }
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->endafter),
				 event_get_recurrence_count (s->ev));
#endif        
    }
}

static void
page_changed (GtkNotebook *notebook, GtkNotebookPage *page,
	      gint page_num, struct edit_state *s)
{
  if (page_num == s->detail_page && ! s->description)
    build_detail_page (s);
  else if (page_num == s->alarm_page && ! s->alarmbutton)
    build_alarm_page (s);
  else if (page_num == s->recurrence_page && ! s->recur_type)
    build_recurrence_page (s);
}

static struct edit_state *
build_edit_event_window (Event *ev)
{
  struct edit_state *s = g_malloc0 (sizeof (struct edit_state));

  if (ev)
    {
      g_object_ref (ev);
      s->ev = ev;
    }

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

  GtkBox *window_box = GTK_BOX (gtk_vbox_new (FALSE, boxspacing * 2));
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (window_box));
  gtk_widget_show (GTK_WIDGET (window_box));

  s->tabs = gtk_notebook_new ();
  gtk_box_pack_start (window_box, GTK_WIDGET (s->tabs), TRUE, TRUE, 0);
  gtk_widget_show (s->tabs);
  
  /* Event tab.  */
  GtkWidget *label = gtk_label_new (_("Event"));
  gtk_widget_show (label);

  GtkWidget *scrolled_window = NULL;
  if (! large)
    {
      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				scrolled_window, label);
      gtk_widget_show (scrolled_window);
    }

  GtkBox *vbox = GTK_BOX (gtk_vbox_new (FALSE, gpe_get_boxspacing()));
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

  /* The summary.  */
  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, 3));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Summary:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (label));

  s->summary = gtk_entry_new ();
  if (s->ev)
    {
      char *str = event_get_summary (s->ev);
      gtk_entry_set_text (GTK_ENTRY (s->summary), str ?: "");
      g_free (str);
    }
  gtk_box_pack_start (hbox, s->summary, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (s->summary));

  /* Whether the event is an all day event.  */
  s->all_day
    = GTK_TOGGLE_BUTTON (gtk_check_button_new_with_label (_("All day event")));
  if (s->ev)
    gtk_toggle_button_set_active (s->all_day, event_get_untimed (s->ev));
  g_signal_connect (G_OBJECT (s->all_day), "toggled",
                    G_CALLBACK (all_day_toggled), s);
  gtk_box_pack_start (vbox, GTK_WIDGET (s->all_day), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->all_day));
  
  /* We want the end of the labels to occur at the same horizontal
     point.  */
  GtkTable *table = GTK_TABLE (gtk_table_new (2, 2, FALSE));
  gtk_table_set_col_spacings (table, gpe_get_boxspacing());
  gtk_table_set_row_spacings (table, gpe_get_boxspacing());
  gtk_box_pack_start (vbox, GTK_WIDGET (table), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (table));

  /* Start time.  */
  label = gtk_label_new (_("Start:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (table, label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (label));

  hbox = GTK_BOX (gtk_hbox_new (FALSE, gpe_get_boxspacing()));
  gtk_table_attach (table, GTK_WIDGET (hbox), 1, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

#ifdef IS_HILDON
  s->starttime = hildon_time_editor_new ();
#else
  s->starttime = gpe_time_sel_new ();
  g_signal_connect (G_OBJECT (s->starttime), "changed",
		    G_CALLBACK (note_time_change), s);
#endif
  gtk_box_pack_start (hbox, s->starttime, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->starttime));

  label = gtk_label_new (_("on:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (label));

#ifdef IS_HILDON
  s->startdate = hildon_date_editor_new ();
  g_signal_connect (G_OBJECT (s->startdate), "focus-out-event",
		    G_CALLBACK (note_date_change), s);
#else
  s->startdate = gtk_date_combo_new ();
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->startdate),
				     ! week_starts_sunday);
  g_signal_connect (G_OBJECT (s->startdate), "changed",
		    G_CALLBACK (note_date_change), s);
#endif
  gtk_box_pack_start (hbox, s->startdate, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->startdate));

  time_t start;
  if (s->ev)
    {
      struct tm tm;
      start = event_get_recurrence_start (s->ev);
      localtime_r (&start, &tm);
#ifdef IS_HILDON
      hildon_time_editor_set_time (HILDON_TIME_EDITOR (s->starttime),
			     tm.tm_hour, tm.tm_min, 0);
      hildon_date_editor_set_date (HILDON_DATE_EDITOR (s->startdate),
			       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
#else
      gpe_time_sel_set_time (GPE_TIME_SEL (s->starttime),
			     tm.tm_hour, tm.tm_min);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
			       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
#endif
    }

  /* End time.  */
  label = gtk_label_new (_("End:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (table, label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (label));

  hbox = GTK_BOX (gtk_hbox_new (FALSE, gpe_get_boxspacing()));
  gtk_table_attach (table, GTK_WIDGET (hbox), 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

#ifdef IS_HILDON
  s->endtime = hildon_time_editor_new ();
#else
  s->endtime = gpe_time_sel_new ();
#endif
  gtk_box_pack_start (hbox, s->endtime, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->endtime));

  label = gtk_label_new (_("on:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (label));

#ifdef IS_HILDON
  s->enddate = hildon_date_editor_new ();
#else
  s->enddate = gtk_date_combo_new ();
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->enddate),
				     ! week_starts_sunday);
#endif
  gtk_box_pack_start (hbox, s->enddate, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (s->enddate));

  if (s->ev)
    {
      struct tm tm;
      time_t end = start + event_get_duration (s->ev);
      if (event_get_untimed (s->ev))
        end -= 24 * 60 * 60;
      else
        end --;
      localtime_r (&end, &tm);
#ifdef IS_HILDON
      hildon_time_editor_set_time (HILDON_TIME_EDITOR (s->endtime), 
                                   tm.tm_hour, tm.tm_min, 0);
      hildon_date_editor_set_date (HILDON_DATE_EDITOR (s->enddate),
			       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
#else
      gpe_time_sel_set_time (GPE_TIME_SEL (s->endtime), tm.tm_hour, tm.tm_min);
      g_signal_connect (G_OBJECT (s->endtime), "changed",
		    G_CALLBACK (sink_end_time), s);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
 			tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      g_signal_connect (G_OBJECT (s->enddate), "changed",
	        G_CALLBACK (sink_end_date), s);
#endif
      gtk_notebook_set_page (GTK_NOTEBOOK (s->tabs), 0);
    }

  /* The calendar combo box.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, gpe_get_boxspacing()));
  gtk_box_pack_start (vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  label = gtk_label_new (_("Calendar:"));
  gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  s->calendar = calendars_combo_box_new (event_db);
  if (s->ev)
    {
      /* Calendar.  */
      EventCalendar *c = event_get_calendar (ev);
      calendars_combo_box_set_active (GTK_COMBO_BOX (s->calendar), c);
      g_object_unref (c);
    }

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
      s->detail_page = gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
						 scrolled_window, label);
      gtk_widget_show (scrolled_window);
    }

  s->detail_box = vbox = GTK_BOX (gtk_vbox_new (FALSE, gpe_get_boxspacing()));
  gtk_container_set_border_width (GTK_CONTAINER (vbox), border);
  gtk_widget_show (GTK_WIDGET (s->detail_box));

  if (! large)
    {
      gtk_scrolled_window_add_with_viewport
             (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (vbox));
      gtk_viewport_set_shadow_type (GTK_VIEWPORT(GTK_WIDGET (vbox)->parent),
				    GTK_SHADOW_NONE);
    }
  else
    s->detail_page = gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
					       GTK_WIDGET (vbox), label);

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
	  s->alarm_page = gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
						    scrolled_window, label);
	  gtk_widget_show (scrolled_window);
	}

      s->alarm_box = vbox = GTK_BOX (gtk_vbox_new (FALSE, 3));
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
	s->alarm_page = gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
						  GTK_WIDGET (vbox), label);
    }
  else
    {
      s->alarm_page = 0;
      s->alarm_box = vbox = main_box;

      build_alarm_page (s);
    }


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
	  s->recurrence_page =
	    gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				      scrolled_window, label);
	  gtk_widget_show (scrolled_window);
	}

      s->recurrence_box = vbox = GTK_BOX (gtk_vbox_new (FALSE, gpe_get_boxspacing()));
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
	s->recurrence_page
	  = gtk_notebook_append_page (GTK_NOTEBOOK (s->tabs),
				      GTK_WIDGET (vbox), label);
    }
  else
    {
      s->recurrence_page = 0;
      s->recurrence_box = main_box;
      build_recurrence_page (s);
    }


  /* Button box.  */
#ifdef IS_HILDON
  GtkBox *button_box = hbox = GTK_BOX (gtk_hbutton_box_new ());
  gtk_box_set_spacing (GTK_BOX (button_box), gpe_get_boxspacing());
#else
  GtkBox *button_box = hbox = GTK_BOX (gtk_hbox_new (FALSE, 0));
#endif
  gtk_box_pack_start (GTK_BOX (window_box), GTK_WIDGET (hbox),
		      FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  GtkWidget *button;
#ifdef IS_HILDON
  button = gtk_button_new_with_label (_("Cancel"));
#else
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
  g_signal_connect_swapped (G_OBJECT (button), "clicked",
                            G_CALLBACK (gtk_widget_destroy), s->window);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, boxspacing);
  gtk_widget_show (button);

#ifdef IS_HILDON
  button = gtk_button_new_with_label (_("Save"));
#else
  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
#endif
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (click_ok), s);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, boxspacing);
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

  gtk_widget_grab_focus (s->summary);

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (gtk_widget_destroy), NULL);

  g_signal_connect (G_OBJECT (window), "key_press_event", 
		            G_CALLBACK (event_ui_key_press_event), s);
  gtk_widget_add_events (GTK_WIDGET (window), 
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK
                         | GDK_FOCUS_CHANGE_MASK);
  
  GTK_WIDGET_SET_FLAGS (s->summary, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (s->summary);
  g_object_set_data(G_OBJECT (window), "default-entry", s->summary);

  /* When the user selects a page, make sure its content is there.  */
  g_signal_connect (G_OBJECT (s->tabs), "switch-page",
		    G_CALLBACK (page_changed), s);

  return s;
}

GtkWidget *
new_event (time_t t)
{
  struct edit_state *s = build_edit_event_window (NULL);
  if (! s)
    return NULL;

  gtk_window_set_title (s->window, _("Calendar: New event"));

  /* 15min steps */
  if ((t % 900) < 450)
      t -= (t % 900);
  else
      t += (900 - (t % 900));
  
  struct tm tm;
  localtime_r (&t, &tm);
#ifdef IS_HILDON
  hildon_time_editor_set_time (HILDON_TIME_EDITOR (s->starttime), 
                               tm.tm_hour, tm.tm_min, 0);
  hildon_date_editor_set_date (HILDON_DATE_EDITOR (s->startdate),
			   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
#else
  gpe_time_sel_set_time (GPE_TIME_SEL (s->starttime), tm.tm_hour, tm.tm_min);
  gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
			   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
#endif
  
  t += 60 * 60;
  localtime_r (&t, &tm);
#ifdef IS_HILDON
  hildon_time_editor_set_time (HILDON_TIME_EDITOR (s->endtime), 
                               tm.tm_hour, tm.tm_min, 0);
  hildon_date_editor_set_date (HILDON_DATE_EDITOR (s->enddate),
			   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
#else
  gpe_time_sel_set_time (GPE_TIME_SEL (s->endtime), tm.tm_hour, tm.tm_min);
  gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
			   tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
#endif

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
  struct edit_state *s = build_edit_event_window (ev);
  if (! s)
    return NULL;

  gtk_window_set_title (s->window, _("Calendar: Edit event"));

  return GTK_WIDGET (s->window);
}
