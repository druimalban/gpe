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

#include "globals.h"
#include "event-ui.h"

#define _(_x) gettext (_x)

struct edit_state
{
  GtkWidget *deletebutton;
  GtkWidget *notebooktype, *notebookedit;

  GtkWidget *startdate, *enddate;
  GtkWidget *starttime, *endtime;
  GtkWidget *reminderdate, *remindertime, *remindertimebutton;
  GtkWidget *taskdate, *taskspin;
  GtkWidget *optionmenutype, *optionmenutask;

  GtkWidget *alarmbutton;
  GtkWidget *alarmspin;
  GtkWidget *alarmoption;

  GtkWidget *description;
  GtkWidget *summary;
  GtkWidget *location;

  GtkWidget *radiobuttonforever, *radiobuttonendafter,
            *radiobuttonendon, *datecomboendon;
  GtkWidget *endspin, *endlabel;
  GtkWidget *radiobuttonnone, *radiobuttondaily, *radiobuttonweekly,
            *radiobuttonmonthly, *radiobuttonyearly;

  GtkWidget *dailybox, *dailyspin;
  GtkWidget *monthlybox, *monthlyspin;
  GtkWidget *yearlybox, *yearlyspin;
  GtkWidget *weeklybox, *weeklyspin, *checkbuttonwday[7];

  GtkAdjustment *endspin_adj, *dailyspin_adj, *monthlyspin_adj,
                *yearlyspin_adj;

  GSList *categories;
  GtkWidget *categories_label;

  GtkWidget *notebookrecur;

  guint page;

  Event *ev;

  gboolean recur_day_floating;
  gboolean end_date_floating, end_time_floating;
};

static GtkWidget *cached_window;

/* minute, hour, day, week */
static const int alarm_multipliers[] = { 60, 60*60, 24*60*60, 7*24*60*60 };
 
static void
destroy_user_data (gpointer p)
{
  struct edit_state *s = (struct edit_state *)p;

  g_slist_free (s->categories);
  g_free (p);
}

static void
recalculate_sensitivities (GtkWidget *widget,
                           GtkWidget *d)
{
  struct edit_state *s = g_object_get_data (G_OBJECT (d), "edit_state");

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
    {
      gtk_widget_set_sensitive (s->alarmoption, 1);
      gtk_widget_set_sensitive (s->alarmspin, 1);
    }
  else
    {
      gtk_widget_set_sensitive (s->alarmoption, 0);
      gtk_widget_set_sensitive (s->alarmspin, 0);
    }

  gtk_widget_set_sensitive (s->remindertime,
                            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->remindertimebutton)));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone)))
    {
      gtk_widget_hide (s->dailybox);
      gtk_widget_hide (s->weeklybox);
      gtk_widget_hide (s->monthlybox);
      gtk_widget_hide (s->yearlybox);
      gtk_widget_set_sensitive (s->endspin, 0);
      gtk_widget_set_sensitive (s->endlabel, 0);
      gtk_widget_set_sensitive (s->radiobuttonforever, 0);
      gtk_widget_set_sensitive (s->radiobuttonendafter, 0);
      gtk_widget_set_sensitive (s->radiobuttonendon, 0);
      gtk_widget_set_sensitive (s->datecomboendon, 0);
    }
  else
    {
      gtk_widget_set_sensitive (s->radiobuttonforever, 1);
      gtk_widget_set_sensitive (s->radiobuttonendafter, 1);
      gtk_widget_set_sensitive (s->radiobuttonendon, 1);
      gtk_widget_set_sensitive (s->datecomboendon, 1);

      if (gtk_toggle_button_get_active
          (GTK_TOGGLE_BUTTON (s->radiobuttonforever)))
        {
          gtk_widget_set_sensitive (s->endspin, 0);
          gtk_widget_set_sensitive (s->endlabel, 0);
          gtk_widget_set_sensitive (s->datecomboendon, 0);
        }
      else if (gtk_toggle_button_get_active
               (GTK_TOGGLE_BUTTON (s->radiobuttonendafter)))
        {
          gtk_widget_set_sensitive (s->endspin, 1);
          gtk_widget_set_sensitive (s->endlabel, 1);
          gtk_widget_set_sensitive (s->datecomboendon, 0);
        }

      else if (gtk_toggle_button_get_active
               (GTK_TOGGLE_BUTTON (s->radiobuttonendon)))
        {
          gtk_widget_set_sensitive (s->endspin, 0);
          gtk_widget_set_sensitive (s->endlabel, 0);
          gtk_widget_set_sensitive (s->datecomboendon, 1);
        }

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                        (s->radiobuttondaily)))
        gtk_widget_show (s->dailybox);
      else
        gtk_widget_hide (s->dailybox);

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                        (s->radiobuttonweekly)))
        gtk_widget_show (s->weeklybox);
      else
        gtk_widget_hide (s->weeklybox);

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                        (s->radiobuttonmonthly)))
        gtk_widget_show (s->monthlybox);
      else
        gtk_widget_hide (s->monthlybox);

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                        (s->radiobuttonyearly)))
        gtk_widget_show (s->yearlybox);
      else
        gtk_widget_hide (s->yearlybox);
    }
}

static guint cached_window_destroy_source;

static gboolean
cached_window_destroy (gpointer data)
{
  if (cached_window)
    {
      gtk_widget_destroy (cached_window);
      cached_window = NULL;
    }
      
  return FALSE;
}

static gboolean
edit_finished (GtkWidget *d)
{
  gtk_widget_hide (d);

  if (cached_window)
    gtk_widget_destroy (d);
  else
    {
      cached_window = d;
      /* Remove cached window in 5 minutes.  */
      cached_window_destroy_source
	= g_timeout_add ((time (NULL) + 5 * 60 * 60) * 1000,
			 cached_window_destroy, NULL);
    }

  return TRUE;
}

static void
click_delete (GtkWidget *widget, GtkWidget *d)
{
  struct edit_state *s = g_object_get_data (G_OBJECT (d), "edit_state");
  Event *ev = s->ev;
  
  if (event_is_recurrence (ev))
    {
      if (gpe_question_ask(
        _("Delete all recurring entries?\n(If no, delete this instance only)"), 
        _("Question"), "question", "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
	event_remove (ev);
      else 
	event_add_recurrence_exception (ev, event_get_start (ev));
    }
  else
    event_remove (ev);
  
  update_view ();
  edit_finished (d);
}

static void
click_ok (GtkWidget *widget, GtkWidget *d)
{
  struct edit_state *s = g_object_get_data (G_OBJECT (d), "edit_state");
  Event *ev;
  struct tm tm_start, tm_end, tm_rend, tm_daystart;
  time_t start_t, end_t, rend_t;
  GtkTextIter start, end;
  GtkTextBuffer *buf;

  if (s->ev)
    ev = s->ev;
  else
    ev = event_new (event_db, NULL);

  if (s->page == 0)
    {
      /* Appointment */
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

      gpe_time_sel_get_time (GPE_TIME_SEL (s->starttime), (guint *)&tm_start.tm_hour, (guint *)&tm_start.tm_min);
      gpe_time_sel_get_time (GPE_TIME_SEL (s->endtime), (guint *)&tm_end.tm_hour, (guint *)&tm_end.tm_min);

      start_t = mktime (&tm_start);
      end_t = mktime (&tm_end);

      /* Zero length appointments would be confused with reminders, so make them one second long.  */
      if (end_t == start_t)
	end_t++;

      event_set_untimed (ev, FALSE);
    }
  else
    {
      /* Reminder */
      memset (&tm_start, 0, sizeof (struct tm));
      tm_start.tm_year = GTK_DATE_COMBO (s->reminderdate)->year - 1900;
      tm_start.tm_mon = GTK_DATE_COMBO (s->reminderdate)->month;
      tm_start.tm_mday = GTK_DATE_COMBO (s->reminderdate)->day;
      tm_start.tm_isdst = -1;

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->remindertimebutton)))
        {
	  gpe_time_sel_get_time (GPE_TIME_SEL (s->remindertime), (guint *)&tm_start.tm_hour, (guint *)&tm_start.tm_min);
	  event_set_untimed (ev, FALSE);
        }
      else
        {
	  event_set_untimed (ev, TRUE);
          tm_start.tm_hour = 12;
        }

      start_t = mktime (&tm_start);
      end_t = start_t;
    }

  if (end_t < start_t)
    {
      gpe_error_box (_("End time must not be earlier than start time"));
      g_object_unref (ev);
      return;
    }

  memset (&tm_daystart, 0, sizeof (struct tm));
  tm_daystart.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
  tm_daystart.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
  tm_daystart.tm_mday = GTK_DATE_COMBO (s->startdate)->day;
      
  if (((start_t < time(NULL)) && !event_get_untimed (ev))
      || (event_get_untimed (ev) && (start_t < mktime(&tm_daystart))))
    {
      GtkWidget* dialog;
      gint ret;
#ifdef IS_HILDON
      dialog = hildon_note_new_confirmation (GTK_WINDOW(d),
                       _("Event starts in the past!\nSave anyway?"));
#else        
      dialog = gtk_message_dialog_new (GTK_WINDOW(d),
                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                       GTK_MESSAGE_WARNING,
                       GTK_BUTTONS_YES_NO,
                       _("Event starts in the past!\nSave anyway?"));
#endif        
      ret = gtk_dialog_run (GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      if ((ret == GTK_RESPONSE_NO) || (ret == GTK_RESPONSE_CANCEL))
        {
	  if (ev != s->ev)
	    event_remove (ev);
          g_object_unref (ev);
          return;
        }
    }
    
  event_set_recurrence_start (ev, start_t);
  event_set_duration (ev, end_t - start_t);

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
      unsigned int mi;
 
      mi = gtk_option_menu_get_history (GTK_OPTION_MENU (s->alarmoption));

      event_set_alarm (ev, alarm_multipliers[mi]
		       * gtk_spin_button_get_value_as_int
		       (GTK_SPIN_BUTTON (s->alarmspin)));
    }
  else
    event_set_alarm (ev, 0);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone)))
    event_set_recurrence_type (ev, RECUR_NONE);
  else
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
					(s->radiobuttondaily)))
        {
	  event_set_recurrence_type (ev, RECUR_DAILY);
	  event_set_recurrence_increment
	    (ev, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->dailyspin)));
        }
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonweekly)))
        {
          guint i;

	  event_set_recurrence_type (ev, RECUR_WEEKLY);
	  event_set_recurrence_increment
	    (ev, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->weeklyspin)));

	  int daymask = 0;
          for (i = 0; i < 7; i++)
	    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i])))
	      daymask |= 1 << i;
	  event_set_recurrence_daymask (ev, daymask);
        }
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonmonthly)))
        {
	  event_set_recurrence_type (ev, RECUR_MONTHLY);
	  event_set_recurrence_increment
	    (ev, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->monthlyspin)));
        }
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonyearly)))
        {
	  event_set_recurrence_type (ev, RECUR_YEARLY);
	  event_set_recurrence_increment
	    (ev, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->yearlyspin)));
        }

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
	    (ev, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->endspin)));
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
	  gpe_time_sel_get_time (GPE_TIME_SEL (s->endtime), (guint *)&tm_rend.tm_hour, (guint *)&tm_rend.tm_min);

          rend_t = mktime (&tm_rend);

	  event_set_recurrence_count (ev, 0);
	  event_set_recurrence_end (ev, rend_t);
        }
    }

  event_flush (ev);

  update_view ();

  edit_finished (d);
}

static gboolean
set_notebook_page (GtkWidget *w, struct edit_state *s)
{
  guint i = gtk_option_menu_get_history (GTK_OPTION_MENU (w));
  gtk_notebook_set_page (GTK_NOTEBOOK (s->notebooktype), i);
  s->page = i;
  gtk_widget_draw (s->notebooktype, NULL);
  
  return FALSE;
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
click_categories (GtkWidget *b, GtkWidget *w)
{
  struct edit_state *s;
  GtkWidget *ui;

  s = g_object_get_data (G_OBJECT (w), "edit_state");
#ifdef IS_HILDON  
  ui = gpe_pim_categories_dialog (s->categories, TRUE, G_CALLBACK (update_categories), s);
#else
  ui = gpe_pim_categories_dialog (s->categories, G_CALLBACK (update_categories), s);
#endif
  gtk_window_set_transient_for(GTK_WINDOW(ui),
                               GTK_WINDOW(gtk_widget_get_toplevel(b)));
  gtk_window_set_modal(GTK_WINDOW(ui), TRUE);
}

gboolean
on_description_focus_in(GtkWidget* widget, GdkEventFocus *event,
                        gpointer user_data)
{
  GtkTextIter iter;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
  GObject *window = user_data;
  
  g_object_set_data(window,"multiline",(void*)TRUE);
  gtk_text_buffer_get_start_iter(buf,&iter);
  gtk_text_buffer_place_cursor(buf,&iter);
  
  return FALSE;
}

gboolean
on_description_focus_out(GtkWidget* widget, GdkEventFocus *event,
                        gpointer user_data)
{
  GObject *window = user_data;
  
  g_object_set_data(window,"multiline",(void*)FALSE);
  return FALSE;
}

void        
tv_move_cursor (GtkTextView *textview,
                GtkMovementStep arg1,
                gint arg2,
                gboolean arg3,
                gpointer user_data)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer(textview);
  
  int cnt = (int)g_object_get_data(G_OBJECT(textview),"cnt");
  
  if (arg1 == GTK_MOVEMENT_DISPLAY_LINES)
    {
      cnt += arg2;
      if (cnt >= gtk_text_buffer_get_line_count(buf))
        {
          cnt = 0;
          gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(textview)),
		                         GTK_DIR_DOWN);
        }
      else if (cnt < 0)
        {
          cnt = 0;
          gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(textview)),
		                         GTK_DIR_UP);
        }  
      g_object_set_data(G_OBJECT(textview),"cnt",(void*)cnt);  
    }
}

static gboolean
event_ui_key_press_event (GtkWidget *widget, GdkEventKey *k, gpointer data)
{
  if (k->keyval == GDK_Escape)
    {
      edit_finished(widget);
      return TRUE;
    }
  if (k->keyval == GDK_Return)
    {
      if (!g_object_get_data(G_OBJECT(widget),"multiline"))
        {
          click_ok(NULL,widget);
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
note_date_change (struct edit_state *s)
{
  if (s->recur_day_floating)
    {
      GtkDateCombo *c;
      int wday, i;

      switch (s->page)
	{
	case 0:  c = GTK_DATE_COMBO (s->startdate); break;
	case 1:  c = GTK_DATE_COMBO (s->reminderdate); break;
	case 2:  c = GTK_DATE_COMBO (s->taskdate); break;
	default: return;
	}
      
      wday = day_of_week (c->year, c->month + 1, c->day);

      for (i = 0; i < 7; i++)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), (i == wday) ? TRUE : FALSE);
    }
}

static void
note_date_change_appt (GtkDateCombo  *c, struct edit_state *s)
{
  if (s->page == 0)
    {
      note_date_change (s);
      if (s->end_date_floating)
	gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate), c->year, c->month, c->day);
    }
}

static void
note_date_change_reminder (GtkWidget *widget, struct edit_state *s)
{
  if (s->page == 1)
    note_date_change (s);
}

static void
note_date_change_task (GtkWidget *widget, struct edit_state *s)
{
  if (s->page == 2)
    note_date_change (s);
}

static GtkWidget *
build_edit_event_window (void)
{
  static const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5,
                                  ABDAY_6, ABDAY_7, ABDAY_1 };
  int i;
  char buf[64];
  struct edit_state *s;

  /* main window */
  GtkWidget *window;

  /* notebook widget within window */
  GtkWidget *notebookedit;

  /* Labels for the tabs */
  GtkWidget *labeleventpage, *labelrecurpage, *labelalarmpage, *labeldetailpage;

  /* SimpleMenu */
  GtkWidget *menutable;
  GtkWidget *menutypelabel;

  /* Summary widgets */
  GtkWidget *summarylabel;
  GtkWidget *summaryentry;

  /* Start/End time widgets */
  GtkWidget *startendtable, *datetimetable, *detailtable;
  GtkWidget *starttime, *endtime;
  GtkWidget *startdatelabel, *enddatelabel,
            *starttimelabel, *endtimelabel,
            *datelabel, *duelabel, *allowlabel;

  /* Description widgets */
  GtkWidget *description;
  GtkWidget *descriptionlabel;

  /* Categories widgets */
  GtkWidget *catbutton, *catlabel;

  /* Buttons !! */
  GtkWidget *buttonbox;
  GtkWidget *buttonok, *buttoncancel, *buttondelete;

  /* Alarm widgets */
  GtkObject *alarmadj;
  GtkWidget *alarmhbox1, *alarmhbox2, *vboxalarm;
  GtkWidget *alarmlabel;
  GtkWidget *alarmmenu;
  GtkWidget *alarmbutton;
  GtkWidget *alarmspin;
  GtkWidget *alarmoption;

  GtkWidget *vboxevent, *vboxrepeat,
            *vboxend, *vboxtop, *vboxrecur,
            *vboxappointment, *vboxreminder, *vboxtask;

  GtkWidget *recurtable, *hboxendafter, *hboxendon,
            *hboxtask1, *hboxtask2;

  GSList    *vboxend_group;
  GSList    *radiogroup;

  GtkWidget *menu1, *menu2;
  GtkWidget *optionmenu1, *optionmenu2;
  GtkWidget *daybutton, *weekbutton;

  GtkWidget *radiobuttonforever, *radiobuttonendafter;
  GtkWidget *radiobuttonendon, *datecomboendon;
  GtkWidget *endspin, *endlabel;
  GtkWidget *radiobuttonnone, *radiobuttondaily, *radiobuttonweekly,
            *radiobuttonmonthly, *radiobuttonyearly;
  GtkWidget *recurborder, *recurendborder;
  GtkWidget *vboxrepeattop;

  GtkWidget *weeklytable, *weeklyhbox3,
            *monthlyhbox1, *monthlyhbox2, *dailyhbox, *yearlyhbox;
  GtkWidget *weeklylabelevery, *weeklylabelweeks, *weeklyspin;

  GtkWidget *dailylabelevery, *dailyspin, *dailylabels;
  GtkWidget *monthlylabelevery, *monthlyspin, *monthlylabels;
  GtkWidget *yearlylabelevery, *yearlyspin, *yearlylabels;

  GtkWidget *scrolledwindowevent, *scrolledwindowalarm, *scrolledwindowrecur;
  GtkWidget *scrolledwindowdetail;
  
  GtkAdjustment *endspin_adj, *dailyspin_adj, *weeklyspin_adj,
                *monthlyspin_adj, *yearlyspin_adj;
  GtkAdjustment *taskadj;
  GtkWidget *locationlabel;

  /* End of declarations */

  int boxspacing      = gpe_get_boxspacing ();
  int border          = gpe_get_border ();

  /* Building the dialog */
  window              = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  notebookedit        = gtk_notebook_new ();
  labeleventpage      = gtk_label_new (_("Event"));
  labeldetailpage     = gtk_label_new (_("Detail"));
  labelrecurpage      = gtk_label_new (_("Recurrence"));
  labelalarmpage      = gtk_label_new (_("Alarm"));

  vboxevent           = gtk_vbox_new (FALSE, boxspacing);
  vboxalarm           = gtk_vbox_new (FALSE, boxspacing);
  vboxrecur           = gtk_vbox_new (FALSE, boxspacing);
  vboxappointment     = gtk_vbox_new (FALSE, 0);
  vboxreminder        = gtk_vbox_new (FALSE, 0);
  vboxtask            = gtk_vbox_new (FALSE, 0);
  vboxrepeattop       = gtk_vbox_new (FALSE, 0);
  vboxrepeat          = gtk_vbox_new (FALSE, 0);
  vboxend             = gtk_vbox_new (FALSE, 0);
  vboxtop             = gtk_vbox_new (FALSE, 0);
  vboxtop             = gtk_vbox_new (FALSE, 0);
  hboxendafter        = gtk_hbox_new (FALSE, 0);
  hboxtask1           = gtk_hbox_new (FALSE, 0);
  hboxtask2           = gtk_hbox_new (FALSE, 0);

  s                   = g_malloc0 (sizeof (struct edit_state));
    
  gpe_set_window_icon (window, "icon");

  int tiny = (gdk_screen_width() <= 300) || (gdk_screen_height() <= 400);
  
  /* if screen is large enough, make it a real dialog */
  if (! tiny)
    {
      gtk_window_set_default_size (GTK_WINDOW (window), 280, 380);
      gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
      gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(gtk_widget_get_toplevel(main_window)));
      gtk_window_set_modal (GTK_WINDOW(window), TRUE);
    }
  
  /* Scrolled window for event tab */
  scrolledwindowevent = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindowevent),
                                         vboxevent);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(vboxevent->parent), GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindowevent),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  menutable           = gtk_table_new (2, 2, FALSE);
  menutypelabel       = gtk_label_new (_("Type:"));
  gtk_table_set_col_spacings (GTK_TABLE (menutable), boxspacing); 
  gtk_table_set_row_spacings (GTK_TABLE (menutable), boxspacing);
  gtk_misc_set_alignment (GTK_MISC (menutypelabel), 0.0, 0.5);

  /* Simple menu for choosing type of task */
  s->optionmenutype   = gtk_simple_menu_new ();
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutype), _("Appointment"));
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutype), _("Reminder"));
  g_signal_connect (G_OBJECT (s->optionmenutype), "changed",
                    G_CALLBACK (set_notebook_page), s);

  s->optionmenutask   = gtk_simple_menu_new ();
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutask), _("Days"));
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutask), _("Weeks"));

  gtk_table_attach (GTK_TABLE (menutable), menutypelabel, 0, 1 , 0, 1, 
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (menutable), s->optionmenutype, 1, 2 , 0, 1, 
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  
  /* Building the summary wigets */
  summaryentry        = gtk_entry_new ();
  summarylabel        = gtk_label_new (_("Text:"));
  
  gtk_misc_set_alignment (GTK_MISC (summarylabel),0,0.5);

  gtk_table_attach (GTK_TABLE (menutable), summarylabel, 0, 1 , 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (menutable), summaryentry, 1, 2 , 1, 2, 
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  /* table for start/end time/date */
  startendtable       = gtk_table_new (2, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (startendtable), boxspacing);
  gtk_table_set_row_spacings (GTK_TABLE (startendtable), boxspacing);

  starttime           = gpe_time_sel_new ();
  endtime             = gpe_time_sel_new ();
  
  g_signal_connect (G_OBJECT (starttime), "changed", G_CALLBACK (note_time_change), s);
  g_signal_connect (G_OBJECT (endtime), "changed", G_CALLBACK (sink_end_time), s);

  startdatelabel      = gtk_label_new (_("Start:"));
  enddatelabel        = gtk_label_new (_("End:"));
  starttimelabel      = gtk_label_new (_("on:"));
  endtimelabel        = gtk_label_new (_("on:"));
  
  gtk_misc_set_alignment (GTK_MISC (startdatelabel), 0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (enddatelabel), 0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (starttimelabel), 0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (endtimelabel), 0, 0.5);
  
  s->startdate        = gtk_date_combo_new ();
  s->enddate          = gtk_date_combo_new ();
  s->reminderdate     = gtk_date_combo_new ();
  s->taskdate         = gtk_date_combo_new ();

  g_signal_connect (G_OBJECT (s->startdate), "changed", G_CALLBACK (note_date_change_appt), s);
  g_signal_connect (G_OBJECT (s->reminderdate), "changed", G_CALLBACK (note_date_change_reminder), s);
  g_signal_connect (G_OBJECT (s->taskdate), "changed", G_CALLBACK (note_date_change_task), s);
  
  g_signal_connect (G_OBJECT (s->enddate), "changed", G_CALLBACK (sink_end_date), s);

  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->startdate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->enddate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->reminderdate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->taskdate), week_starts_monday);

  gtk_table_attach (GTK_TABLE (startendtable), startdatelabel, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), starttime, 1, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), starttimelabel, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (startendtable), s->startdate, 3, 4, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), enddatelabel, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), endtime, 1, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), endtimelabel, 2, 3, 1, 2,
                    GTK_FILL, GTK_FILL, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (startendtable), s->enddate, 3, 4, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  gtk_box_pack_start (GTK_BOX (vboxappointment), startendtable, FALSE, FALSE, 0);

  datetimetable       = gtk_table_new (2, 2, FALSE);

  datelabel           = gtk_label_new (_("Date:"));
  gtk_misc_set_alignment (GTK_MISC (datelabel), 0.0, 0.5);

  s->remindertimebutton = gtk_check_button_new_with_label (_("Time:"));
  g_signal_connect (G_OBJECT (s->remindertimebutton), "clicked",
                    G_CALLBACK (recalculate_sensitivities), window);

  s->remindertime     = gpe_time_sel_new ();

  gtk_table_attach (GTK_TABLE (datetimetable), datelabel, 0, 1, 0, 1,
                    0, GTK_EXPAND | GTK_FILL, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (datetimetable), s->reminderdate, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (datetimetable), s->remindertimebutton, 0, 1, 1, 2,
                    0, GTK_EXPAND | GTK_FILL, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (datetimetable), s->remindertime, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_box_pack_start (GTK_BOX (vboxreminder), datetimetable, TRUE, TRUE, 0);

  taskadj             = (GtkAdjustment *)gtk_adjustment_new (1, 1, 100, 1, 10, 10);
  s->taskspin         = gtk_spin_button_new (taskadj, 1, 0);
  duelabel            = gtk_label_new (_("Due:"));
  gtk_box_pack_start (GTK_BOX (hboxtask1), duelabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hboxtask1), s->taskdate, TRUE, TRUE, 0);

  allowlabel          = gtk_label_new (_("Allow:"));
  gtk_box_pack_start (GTK_BOX (hboxtask2), allowlabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hboxtask2), s->taskspin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hboxtask2), s->optionmenutask, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vboxtask), hboxtask1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxtask), hboxtask2, FALSE, FALSE, 0);

  /* Detail table */
  detailtable       = gtk_table_new (2, 4, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER(detailtable), border);
  gtk_table_set_col_spacings (GTK_TABLE (detailtable), boxspacing);
  gtk_table_set_row_spacings (GTK_TABLE (detailtable), boxspacing);
  
  /* location  */  
  locationlabel     = gtk_label_new (_("Location:"));
  s->location       = gtk_entry_new ();
  gtk_misc_set_alignment (GTK_MISC(locationlabel), 0, 0.5);
  /* Categories */
  catbutton = gtk_button_new_with_label (_("Categories"));
  catlabel = gtk_label_new (_("Define categories by tapping the Categories button."));
  gtk_label_set_line_wrap (GTK_LABEL(catlabel), TRUE);
  gtk_misc_set_alignment (GTK_MISC (catlabel), 0.0, 0.5);  

  g_signal_connect (G_OBJECT (catbutton), "clicked",
                    G_CALLBACK (click_categories), window);

  /* Description textarea */
  description         = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (description), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (description), TRUE);

  g_signal_connect(G_OBJECT(description), "focus-in-event", 
                   G_CALLBACK(on_description_focus_in),window);
  g_signal_connect(G_OBJECT(description), "focus-out-event", 
                   G_CALLBACK(on_description_focus_out),window);
  g_signal_connect(G_OBJECT(description),"move_cursor",
                   G_CALLBACK(tv_move_cursor),NULL);

  descriptionlabel    = gtk_label_new (_("Description:"));
  gtk_misc_set_alignment(GTK_MISC (descriptionlabel), 0, 0.5);
  gtk_widget_set_size_request (GTK_WIDGET (description), -1, 68);

  gtk_table_attach(GTK_TABLE(detailtable), locationlabel, 0, 1, 0, 1, 
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(detailtable), s->location, 1, 2, 0, 1, 
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(detailtable), catbutton, 0, 1, 1, 2, 
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(detailtable), catlabel, 1, 2, 1, 2, 
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(detailtable), descriptionlabel, 0, 1, 2, 3, 
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(detailtable), description, 0, 2, 3, 4, 
                   GTK_FILL, GTK_FILL, 0, 0);
  
  /* Putting the tab together */
  s->notebooktype = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (s->notebooktype), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (s->notebooktype), FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebooktype), vboxappointment, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebooktype), vboxreminder, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebooktype), vboxtask, NULL);

  gtk_box_pack_start (GTK_BOX (vboxevent), menutable, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxevent), s->notebooktype, FALSE, FALSE, 0);
  
  gtk_container_set_border_width (GTK_CONTAINER (vboxevent), border);

  scrolledwindowdetail = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindowdetail),
                                         detailtable);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(detailtable->parent), GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindowdetail),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);


  /* Alarm page */
  alarmhbox1          = gtk_hbox_new (FALSE, boxspacing);
  alarmhbox2          = gtk_hbox_new (FALSE, boxspacing);
  scrolledwindowalarm = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindowalarm),
                                         vboxalarm);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(vboxalarm->parent), GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindowalarm),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  alarmmenu           = gtk_menu_new ();
  alarmbutton         = gtk_check_button_new_with_label (_("Alarm"));
  alarmadj            = gtk_adjustment_new (5.0, 1.0, 10000.0, 1.0, 5.0, 5.0);
  alarmspin           = gtk_spin_button_new (GTK_ADJUSTMENT (alarmadj), 1.0, 0);
  alarmoption         = gtk_option_menu_new ();
  alarmlabel          = gtk_label_new (_("before event"));

  gtk_menu_append (GTK_MENU (alarmmenu),
                   gtk_menu_item_new_with_label (_("minutes")));
  gtk_menu_append (GTK_MENU (alarmmenu),
                   gtk_menu_item_new_with_label (_("hours")));
  gtk_menu_append (GTK_MENU (alarmmenu),
                   gtk_menu_item_new_with_label (_("days")));
  gtk_menu_append (GTK_MENU (alarmmenu),
                   gtk_menu_item_new_with_label (_("weeks")));

  gtk_option_menu_set_menu (GTK_OPTION_MENU (alarmoption), alarmmenu);

  g_signal_connect (G_OBJECT (alarmbutton), "clicked",
                    G_CALLBACK (recalculate_sensitivities), window);

  gtk_box_pack_start (GTK_BOX (alarmhbox1), alarmbutton, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (alarmhbox2), alarmspin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (alarmhbox2), alarmoption, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (alarmhbox2), alarmlabel, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vboxalarm), alarmhbox1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxalarm), alarmhbox2, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxalarm), border);

  gtk_container_set_border_width (GTK_CONTAINER (vboxrecur), border);

  /* Button box */

#ifdef IS_HILDON
  buttonbox           = gtk_hbutton_box_new ();
  buttonok            = gtk_button_new_with_label (_("Save"));
  buttoncancel        = gtk_button_new_with_label (_("Cancel"));
  buttondelete        = gtk_button_new_with_label (_("Delete"));

  gtk_box_pack_start (GTK_BOX (buttonbox), buttonok, FALSE, TRUE, 
                      gpe_get_boxspacing());
  gtk_box_pack_start (GTK_BOX (buttonbox), buttoncancel, FALSE, TRUE, 
                      gpe_get_boxspacing());
  gtk_box_pack_start (GTK_BOX (buttonbox), buttondelete, FALSE, TRUE, 
                      gpe_get_boxspacing());
#else
  /* using GtkHBox here since GtkHButtonBox packs the widgets in a way
     that stop them fitting on a 240x320 screen.  */
  buttonbox           = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (buttonbox), border / 2);
  
  buttonok            = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  buttoncancel        = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  buttondelete        = gtk_button_new_from_stock (GTK_STOCK_DELETE);

  gtk_box_pack_start (GTK_BOX (buttonbox), buttondelete, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttoncancel, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), buttonok, TRUE, FALSE, 0);
#endif

  g_signal_connect (G_OBJECT (buttonok), "clicked",
                    G_CALLBACK (click_ok), window);
  g_signal_connect (G_OBJECT (buttondelete), "clicked",
                    G_CALLBACK (click_delete), window);
  g_signal_connect_swapped (G_OBJECT (buttoncancel), "clicked",
                            G_CALLBACK (edit_finished), window);

  /* Reccurence page */

  vboxrepeattop = gtk_vbox_new (FALSE, gpe_get_boxspacing());
  gtk_container_set_border_width(GTK_CONTAINER(vboxrepeattop), gpe_get_border());

  scrolledwindowrecur = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindowrecur), vboxrepeattop);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(vboxrepeattop->parent), GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindowrecur),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  recurborder = gtk_frame_new (_("Type"));
  gtk_box_pack_start (GTK_BOX (vboxrepeattop), recurborder,
                           FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (recurborder), vboxrepeat);

  vboxrecur = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxrepeat), vboxrecur, FALSE, FALSE, 0);

  /* recurrence radio buttons */
  recurtable = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vboxrecur), recurtable, FALSE, FALSE, 0);

  radiobuttonnone = gtk_radio_button_new_with_label (NULL, _("No recurrence"));
  gtk_table_attach_defaults (GTK_TABLE (recurtable), radiobuttonnone, 0, 1, 0, 1);

  radiobuttondaily = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobuttonnone), _("Daily"));
  gtk_table_attach_defaults (GTK_TABLE (recurtable), radiobuttondaily, 0, 1, 1, 2);

  radiobuttonweekly = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobuttonnone), _("Weekly"));
  gtk_table_attach_defaults (GTK_TABLE (recurtable), radiobuttonweekly, 1, 2, 1, 2);

  radiobuttonmonthly = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobuttonnone), _("Monthly"));
  gtk_table_attach_defaults (GTK_TABLE (recurtable), radiobuttonmonthly, 0, 1, 2, 3);

  radiobuttonyearly = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radiobuttonnone), _("Yearly"));
  gtk_table_attach_defaults (GTK_TABLE (recurtable), radiobuttonyearly, 1, 2, 2, 3);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonnone), TRUE);

  g_signal_connect (G_OBJECT (radiobuttonnone), "toggled",
                    G_CALLBACK (recalculate_sensitivities), window);
  g_signal_connect (G_OBJECT (radiobuttondaily), "toggled",
                    G_CALLBACK (recalculate_sensitivities), window);
  g_signal_connect (G_OBJECT (radiobuttonweekly), "toggled",
                    G_CALLBACK (recalculate_sensitivities), window);
  g_signal_connect (G_OBJECT (radiobuttonmonthly), "toggled",
                    G_CALLBACK (recalculate_sensitivities), window);
  g_signal_connect (G_OBJECT (radiobuttonyearly), "toggled",
                    G_CALLBACK (recalculate_sensitivities), window);

  /* daily hbox */
  s->dailybox      = gtk_vbox_new (FALSE, 0);
  dailyhbox        = gtk_hbox_new (FALSE, 6);

  /* "every" label */
  dailylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start (GTK_BOX (dailyhbox), dailylabelevery,
                           FALSE, FALSE, 0);

  /* daily spinner */
  dailyspin_adj    = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  dailyspin = gtk_spin_button_new (GTK_ADJUSTMENT (dailyspin_adj), 1, 0);
  gtk_box_pack_start (GTK_BOX (dailyhbox), dailyspin, FALSE, FALSE, 0);

  /* days label */
  dailylabels      = gtk_label_new (_("days"));
  gtk_box_pack_start (GTK_BOX (dailyhbox), dailylabels,
                           FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (s->dailybox), dailyhbox, FALSE, FALSE, 0);

  /* weekly box */
  s->weeklybox     = gtk_vbox_new (FALSE, 0);

  /* weekly hbox3 */
  weeklyhbox3      = gtk_hbox_new (FALSE, 0);
  weeklylabelevery = gtk_label_new (_("Every"));
  weeklylabelweeks = gtk_label_new (_("weeks, on:"));
  weeklyspin_adj   = (GtkAdjustment *) gtk_adjustment_new (1, 1, 52, 1, 5, 5);
  weeklyspin       = gtk_spin_button_new (GTK_ADJUSTMENT(weeklyspin_adj), 1, 0);
  gtk_box_pack_start (GTK_BOX (weeklyhbox3), weeklylabelevery,
                             FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (weeklyhbox3), weeklyspin,
                             FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (weeklyhbox3), weeklylabelweeks,
                             FALSE, FALSE, 0);

/* weekly hbox1 */
  weeklytable      = gtk_table_new (3, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (s->weeklybox), weeklytable, FALSE, FALSE, 0);

  for (i = 0; i < 7; i++)
    {
      gchar *gs;
  
      gs = g_locale_to_utf8 (nl_langinfo(days[i]), -1, NULL, NULL, NULL);
      GtkWidget *b = gtk_check_button_new_with_label (gs);
      gtk_table_attach_defaults (GTK_TABLE (weeklytable), b,
				 i % 3, (i % 3) + 1, i / 3, (i / 3) + 1);
      s->checkbuttonwday[i] = b;
      g_free (gs);
      g_signal_connect (G_OBJECT (b), "clicked", G_CALLBACK (sink_weekly), s);
    }

/* monthly hbox */

  s->monthlybox    = gtk_vbox_new (FALSE, 6);

  monthlyhbox1     = gtk_hbox_new (FALSE, 6);
  monthlyhbox2     = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (s->monthlybox), monthlyhbox1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (s->monthlybox), monthlyhbox2, FALSE, FALSE, 0);

  /* "every" label */
  monthlylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start (GTK_BOX (monthlyhbox1), monthlylabelevery,
                           FALSE, FALSE, 0);

  /* monthly spinner */
  monthlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 12, 1, 2, 2);
  monthlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (monthlyspin_adj), 1, 0);
  gtk_box_pack_start (GTK_BOX (monthlyhbox1), monthlyspin,
                           FALSE, FALSE, 0);

  /* months label */
  monthlylabels = gtk_label_new (_("months"));
  gtk_box_pack_start (GTK_BOX (monthlyhbox1), monthlylabels,
                           FALSE, FALSE, 0);


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

  /* yearly hbox */
  s->yearlybox = gtk_vbox_new (FALSE, 6);
  yearlyhbox = gtk_hbox_new (FALSE, 6);

  /* "every" label */
  yearlylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start (GTK_BOX (yearlyhbox), yearlylabelevery,
                           FALSE, FALSE, 0);

  /* yearly spinner */
  yearlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  yearlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (yearlyspin_adj), 1, 0);
  gtk_box_pack_start (GTK_BOX (yearlyhbox), yearlyspin, FALSE, FALSE, 0);

  /* years label */
  yearlylabels = gtk_label_new (_("years"));
  gtk_box_pack_start (GTK_BOX (yearlyhbox), yearlylabels,
                           FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (s->yearlybox), yearlyhbox, FALSE, FALSE, 0);

  s->notebookrecur = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (s->notebookrecur), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (s->notebookrecur), FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebookrecur), s->dailybox, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebookrecur), s->weeklybox, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebookrecur), s->monthlybox, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebookrecur), s->yearlybox, NULL);

  gtk_box_pack_start (GTK_BOX (vboxrepeat), s->notebookrecur, TRUE, TRUE, 0);

/* Begin end-date vbox */

  recurendborder = gtk_frame_new (_("End Date"));
  gtk_container_add (GTK_CONTAINER (recurendborder), vboxend);
  gtk_box_pack_start (GTK_BOX (vboxrepeattop), recurendborder,
                    FALSE, TRUE, 0);

  /* forever radio button */
  radiobuttonforever = gtk_radio_button_new_with_label (NULL, _("forever"));
  gtk_box_pack_start (GTK_BOX (vboxend), radiobuttonforever, 
                           FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonforever , FALSE);

  /* end after radio button */
  gtk_box_pack_start (GTK_BOX (vboxend), hboxendafter,
		      FALSE, TRUE, 0);
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonforever));
  radiobuttonendafter = gtk_radio_button_new_with_label (vboxend_group,
                                                         _("end after"));
  gtk_box_pack_start (GTK_BOX (hboxendafter), radiobuttonendafter,
                           FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonendafter, FALSE);
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonendafter));

  /* end after spinner */
  endspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  endspin = gtk_spin_button_new (GTK_ADJUSTMENT (endspin_adj), 1, 0);
  gtk_widget_set_usize (endspin, 50, 20);
  gtk_box_pack_start (GTK_BOX (hboxendafter), endspin, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endspin, FALSE);

  /* end on hbox */
  hboxendon = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxend), hboxendon, FALSE, FALSE, 0);
  radiobuttonendon = gtk_radio_button_new_with_label (vboxend_group,
                                                      _("end on"));
  gtk_box_pack_start (GTK_BOX (hboxendon), radiobuttonendon,
                           FALSE, FALSE, 0);
  datecomboendon = gtk_date_combo_new ();
  gtk_box_pack_start (GTK_BOX (hboxendon), datecomboendon,
                           TRUE, TRUE, 2);

  /* events label */
  endlabel = gtk_label_new (_("recurrences"));
  gtk_box_pack_start (GTK_BOX (hboxendafter), endlabel, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endlabel, FALSE);

  g_signal_connect (G_OBJECT (radiobuttonforever), "toggled",
                    G_CALLBACK (recalculate_sensitivities), window);
  g_signal_connect (G_OBJECT (radiobuttonendafter), "toggled",
                    G_CALLBACK (recalculate_sensitivities), window);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonforever), TRUE);

  gtk_box_pack_start (GTK_BOX (vboxtop), notebookedit, TRUE, TRUE, 2);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebookedit), scrolledwindowevent,
                            labeleventpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebookedit), scrolledwindowdetail,
                            labeldetailpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebookedit), scrolledwindowalarm,
                            labelalarmpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebookedit), scrolledwindowrecur,
                            labelrecurpage);
  gtk_widget_grab_focus (summaryentry);

  s->notebookedit = notebookedit;
  s->deletebutton = buttondelete;
  s->starttime = starttime;
  s->endtime = endtime;
  s->alarmbutton = alarmbutton;
  s->alarmspin = alarmspin;
  s->alarmoption = alarmoption;
  s->description = description;
  s->summary = summaryentry;
  s->radiobuttonforever = radiobuttonforever;
  s->radiobuttonendafter = radiobuttonendafter;
  s->radiobuttonendon = radiobuttonendon;
  s->datecomboendon = datecomboendon;
  s->endspin = endspin;
  s->endlabel = endlabel;
  s->radiobuttonnone = radiobuttonnone;
  s->radiobuttondaily = radiobuttondaily;
  s->radiobuttonweekly = radiobuttonweekly;
  s->radiobuttonmonthly = radiobuttonmonthly;
  s->radiobuttonyearly = radiobuttonyearly;
  s->dailyspin = dailyspin;
  s->weeklyspin = weeklyspin;
  s->monthlyspin = monthlyspin;
  s->yearlyspin = yearlyspin;
  s->endspin_adj = endspin_adj;
  s->dailyspin_adj = dailyspin_adj;
  s->monthlyspin_adj = monthlyspin_adj;
  s->yearlyspin_adj = yearlyspin_adj;
  s->categories_label = catlabel;

  gtk_box_pack_start (GTK_BOX (vboxtop), buttonbox, FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (window), vboxtop);

  g_object_set_data_full (G_OBJECT (window), "edit_state", s, destroy_user_data);

  gtk_widget_show_all (vboxtop);
  recalculate_sensitivities (NULL, window);
  
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (edit_finished), NULL);

  g_signal_connect (G_OBJECT (window), "key_press_event", 
		            G_CALLBACK (event_ui_key_press_event), NULL);
  gtk_widget_add_events (GTK_WIDGET (window), 
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  
  GTK_WIDGET_SET_FLAGS (summaryentry, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (summaryentry);
  g_object_set_data(G_OBJECT (window), "default-entry", summaryentry);
  
  return window;
}

static GtkWidget *
edit_event_window (void)
{
  GtkWidget *w;

  if (cached_window)
    {
      w = cached_window;
      cached_window = NULL;
      if (cached_window_destroy_source)
	{
	  g_source_remove (cached_window_destroy_source);
	  cached_window_destroy_source = 0;
	}
    }
  else
    w = build_edit_event_window ();

  return w;
}

GtkWidget *
new_event (time_t t, guint timesel)
{
  GtkWidget *entry, *w = edit_event_window ();

  if (w)
    {
      struct tm tm;
      int i, wday;
      struct edit_state *s = g_object_get_data (G_OBJECT (w),
                                                "edit_state");

      gtk_window_set_title (GTK_WINDOW (w), _("Calendar: New event"));

      s->ev = NULL;
      gtk_widget_set_sensitive (s->deletebutton, FALSE);

      localtime_r (&t, &tm);
      gpe_time_sel_set_time (GPE_TIME_SEL (s->starttime), tm.tm_hour, tm.tm_min);
      gpe_time_sel_set_time (GPE_TIME_SEL (s->remindertime), tm.tm_hour, tm.tm_min);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->reminderdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      t += 60 * 60;
      localtime_r (&t, &tm);
      gpe_time_sel_set_time (GPE_TIME_SEL (s->endtime), tm.tm_hour, tm.tm_min);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone), TRUE);
      gtk_notebook_set_page (GTK_NOTEBOOK (s->notebookedit), 0);
      gtk_entry_set_text (GTK_ENTRY (s->summary), "");
      gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description)), "", -1);

      gtk_option_menu_set_history (GTK_OPTION_MENU (s->optionmenutype), 0);
      s->page = 0;
      gtk_notebook_set_page (GTK_NOTEBOOK (s->notebooktype), s->page);
	  
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->remindertimebutton), FALSE);

      wday = day_of_week (tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
      for (i = 0; i < 7; i++)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), (i == wday) ? TRUE : FALSE);

      s->recur_day_floating = TRUE;
      s->end_date_floating = TRUE;
      s->end_time_floating = TRUE;

      entry = g_object_get_data (G_OBJECT (w), "default-entry");
      if (entry)
        {
	  gtk_widget_grab_focus (entry);
          gtk_editable_select_region (GTK_EDITABLE (entry), 0, 0);
        }
    }

  return w;
}

GtkWidget *
edit_event (Event *ev)
{
  GtkWidget *w = edit_event_window ();

  if (w)
    {
      time_t end;
      struct tm tm;
      int i;
      struct edit_state *s = g_object_get_data (G_OBJECT (w), "edit_state");
	
      s->ev = ev;

      gtk_window_set_title (GTK_WINDOW (w), _("Calendar: Edit event"));

      gtk_widget_set_sensitive (s->deletebutton, TRUE);
      gtk_text_buffer_set_text (gtk_text_view_get_buffer
				(GTK_TEXT_VIEW (s->description)),
				event_get_description (ev) ?: "", -1);
      gtk_entry_set_text (GTK_ENTRY (s->summary),
			  event_get_summary (ev) ?: "");
      gtk_entry_set_text (GTK_ENTRY (s->location),
			  event_get_location (ev) ?: "");
      update_categories (w, event_get_categories (ev), s);

      time_t start = event_get_recurrence_start (ev);
      localtime_r (&start, &tm);
      gpe_time_sel_set_time (GPE_TIME_SEL (s->starttime), tm.tm_hour, tm.tm_min);
      gpe_time_sel_set_time (GPE_TIME_SEL (s->remindertime), tm.tm_hour, tm.tm_min);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->reminderdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      end = start + event_get_duration (ev);
      localtime_r (&end, &tm);
      gpe_time_sel_set_time (GPE_TIME_SEL (s->endtime), tm.tm_hour, tm.tm_min);
      gtk_notebook_set_page (GTK_NOTEBOOK (s->notebookedit), 0);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      /* we assume that "Appointment" is at index = 0
       * and "Reminder" is at index = 1 */
      gtk_option_menu_set_history (GTK_OPTION_MENU (s->optionmenutype),
                                   event_get_duration (ev) == 0);
      s->page = is_reminder (ev) ? 1 : 0;
      gtk_notebook_set_page (GTK_NOTEBOOK (s->notebooktype), s->page);

      /* Reminder */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->remindertimebutton),
				    (!event_get_untimed (ev)));
 
      if (event_get_alarm (ev))
        {
          unsigned int unit = 0;	/* minutes */
	  unsigned int i;

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), TRUE);
	  for (i = 0; i < 4; i++)
	    {
	      if ((event_get_alarm (ev) % alarm_multipliers[i]) == 0)
		unit = i;
	    }
	  
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->alarmspin), event_get_alarm (ev) / alarm_multipliers[unit]);
	  gtk_option_menu_set_history (GTK_OPTION_MENU (s->alarmoption), unit);
        }
      else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), FALSE);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttondaily), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonweekly), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonmonthly), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonyearly), FALSE);

      for (i = 0; i < 7; i++)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i]), FALSE);

      if (event_is_recurrence (ev))
        {
          switch (event_get_recurrence_type (ev))
            {
            case RECUR_NONE:
              abort ();

            case RECUR_DAILY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttondaily), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->dailyspin),
					 event_get_recurrence_increment (ev));
              break;

            case RECUR_WEEKLY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonweekly), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->weeklyspin),
					 event_get_recurrence_increment (ev));
	      int daymask = event_get_recurrence_daymask (ev);
              if (daymask & MON) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[0]), 1);
              if (daymask & TUE) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[1]), 1);
              if (daymask & WED) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[2]), 1);
              if (daymask & THU) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[3]), 1);
              if (daymask & FRI) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[4]), 1);
              if (daymask & SAT) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[5]), 1);
              if (daymask & SUN) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[6]), 1);
              break;

            case RECUR_MONTHLY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonmonthly), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->monthlyspin),
					 event_get_recurrence_increment (ev));
              break;

            case RECUR_YEARLY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonyearly), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->yearlyspin),
					 event_get_recurrence_increment (ev));
              break;
            }

	  /* Turn all of the buttons off.  */
	  gtk_toggle_button_set_active
	    (GTK_TOGGLE_BUTTON (s->radiobuttonendon), FALSE);
	  gtk_toggle_button_set_active
	    (GTK_TOGGLE_BUTTON (s->radiobuttonendafter), FALSE);
	  gtk_toggle_button_set_active
	    (GTK_TOGGLE_BUTTON (s->radiobuttonforever), FALSE);

	  /* Then turn the right one on.  */
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
                                       tm.tm_year + 1900, tm.tm_mon,
				       tm.tm_mday);
	    }
	  gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->endspin),
				     event_get_recurrence_count (ev));
        }
      else 
	gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (s->radiobuttonnone), TRUE);
    }

  return w;
}
