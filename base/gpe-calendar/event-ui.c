/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Philip Blundell <philb@gnu.org>
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
#include <gpe/schedule.h>
#include <gpe/pim-categories.h>
#include <gpe/pim-categories-ui.h>

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

  event_t ev;
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


static void
weekly_toggled (GtkWidget *widget,
                           GtkWidget *d)
{
  struct edit_state *s = g_object_get_data (G_OBJECT (d), "edit_state");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
  {
     if (!s->ev || !s->ev->recur || !s->ev->recur->daymask)
     {
        time_t t = time(NULL);
        struct tm *lt = localtime(&t);
          
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[(lt->tm_wday+6)%7]), TRUE);
     }
  }
  recalculate_sensitivities(widget, d);
}


void
unschedule_alarm (event_t ev, GtkWidget *d)
{
  event_t ev_real = get_cloned_ev(ev);

  if (!schedule_cancel_alarm (ev_real->uid, ev->start)) 
  {
    GtkWidget* dialog;
                
    dialog = gtk_message_dialog_new (GTK_WINDOW(d),
    	   GTK_DIALOG_DESTROY_WITH_PARENT,
    	   GTK_MESSAGE_WARNING,
    	   GTK_BUTTONS_CLOSE,
    	   _("There is a problem with the scheduling daemon (perhaps atd is not running)!\nThis can cause issues."));
    gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
      
  }
}

void
schedule_next (guint skip, guint uid, GtkWidget *d)
{
  GSList *events = NULL, *iter;
  struct tm tm;
  time_t end, start = time(NULL);
  event_details_t ev_d;

  localtime_r (&start, &tm);
  tm.tm_year++;
  end=mktime (&tm);

  events = event_db_list_alarms_for_period (start, end);

  for (iter = events; iter; iter = g_slist_next (iter))
    {
      event_t ev_real, ev = iter->data;

      if (ev->flags & FLAG_ALARM) 
	{
	  ev_real = get_cloned_ev(ev);
	  ev_d = event_db_get_details (ev_real);
	  if (((int)(ev->start) != (int)skip) && (uid!=ev_real->uid))
	    {
	      gchar *action;

	      action = g_strdup_printf ("gpe-announce '%s'\ngpe-calendar -s %ld -e %ld &\n",
					ev_d->summary, (long)ev->start, ev_real->uid);
	      if (!schedule_set_alarm (ev->uid, ev->start, action, TRUE)) 
	      {
	        GtkWidget* dialog;
                
		dialog = gtk_message_dialog_new (GTK_WINDOW(d),
                       GTK_DIALOG_DESTROY_WITH_PARENT,
                       GTK_MESSAGE_WARNING,
                       GTK_BUTTONS_CLOSE,
                       _("There is a problem with the scheduling daemon (perhaps atd is not running)!\nThis can cause issues."));
		gtk_dialog_run (GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
              }
	      g_free (action);
	      break;
	    }
	}
    }
    event_db_list_destroy (events);
}

static gboolean
edit_finished (GtkWidget *d)
{
  gtk_widget_hide (d);

  if (cached_window)
    gtk_widget_destroy (d);
  else
    cached_window = d;

  return TRUE;
}

static void
click_delete (GtkWidget *widget, GtkWidget *d)
{
  struct edit_state *s = g_object_get_data (G_OBJECT (d), "edit_state");
  event_t ev = s->ev, ev_real;
  event_details_t ev_d;
  recur_t r;
  
  ev_real = get_cloned_ev(ev);
  ev_d = event_db_get_details (ev_real);
     
  if (ev_real->recur)
    {
      if (gpe_question_ask(
        _("Delete all recurring entries?\n(If no, delete this instance only)"), 
        _("Question"), "question", "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
        {			  
          if (ev_real->flags & FLAG_ALARM)
            {
              unschedule_alarm (ev, gtk_widget_get_toplevel(widget));
              schedule_next (0,0, gtk_widget_get_toplevel(widget));
            }
          
          event_db_remove (ev_real);
        }
      else 
        {
          r = event_db_get_recurrence (ev_real);
          r->exceptions = g_slist_append (r->exceptions, (void *)ev->start);
          event_db_flush (ev_real);
        }
    }
  else
    {			  
      if (ev_real->flags & FLAG_ALARM)
        {
          unschedule_alarm (ev, gtk_widget_get_toplevel(widget));
          schedule_next (0,0, gtk_widget_get_toplevel(widget));
        }
      
      event_db_remove (ev_real);
  }
  
  update_all_views ();
  event_db_forget_details (ev_real);
  edit_finished (d);
}

static void
click_ok (GtkWidget *widget, GtkWidget *d)
{
  struct edit_state *s = g_object_get_data (G_OBJECT (d), "edit_state");
  event_t ev;
  event_details_t ev_d;
  struct tm tm_start, tm_end, tm_rend, tm_daystart;
  time_t start_t, end_t, rend_t;
  gboolean new_event = FALSE;
  GtkTextIter start, end;
  GtkTextBuffer *buf;
    
  if (s->ev)
    {
      ev = get_cloned_ev(s->ev);
      ev_d = event_db_get_details (ev);
      if (ev->flags & FLAG_ALARM)
        unschedule_alarm (s->ev, gtk_widget_get_toplevel(widget));
      ev_d->sequence++;
      ev_d->modified=time(NULL);
    }
  else
    {
      ev = event_db_new ();
      ev_d = event_db_alloc_details (ev);
      new_event = TRUE;
    }

  ev->flags = 0;

  if (s->page == 0)
    {
      /* Appointment */
      char *start = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO
                                                          (s->starttime)->entry),
                                            0, -1);
      char *end = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO
                                                        (s->endtime)->entry),
                                          0, -1);

      memset (&tm_start, 0, sizeof (struct tm));
      tm_start.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
      tm_start.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
      tm_start.tm_mday = GTK_DATE_COMBO (s->startdate)->day;

      memset (&tm_end, 0, sizeof (struct tm));
      tm_end.tm_year = GTK_DATE_COMBO (s->enddate)->year - 1900;
      tm_end.tm_mon = GTK_DATE_COMBO (s->enddate)->month;
      tm_end.tm_mday = GTK_DATE_COMBO (s->enddate)->day;

      strptime (start, TIMEFMT, &tm_start);
      strptime (end, TIMEFMT, &tm_end);

      start_t = mktime (&tm_start);
      end_t = mktime (&tm_end);

      /* If DST was in effect, mktime will have "helpfully" incremented the
         hour.  */
      if (tm_start.tm_isdst)
        {
          start_t -= 60*60;
          end_t -= 60*60;
        }

      g_free (end);
      g_free (start);
    }
  else
    {
      /* Reminder */
      memset (&tm_start, 0, sizeof (struct tm));
      tm_start.tm_year = GTK_DATE_COMBO (s->reminderdate)->year - 1900;
      tm_start.tm_mon = GTK_DATE_COMBO (s->reminderdate)->month;
      tm_start.tm_mday = GTK_DATE_COMBO (s->reminderdate)->day;

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->remindertimebutton)))
        {
          char *start = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO
                                                              (s->remindertime)->entry),
                                                0, -1);
          strptime (start, TIMEFMT, &tm_start);
        }
      else
        {
          ev->flags |= FLAG_UNTIMED;
          tm_start.tm_hour = 12;
        }

      start_t = mktime (&tm_start);

      /* If DST was in effect, mktime will have "helpfully" incremented the
         hour.  */
      if (tm_start.tm_isdst)
        start_t -= 60*60;

      end_t = start_t;
    }

  if (end_t < start_t)
    {
      gpe_error_box (_("End time must not be earlier than start time"));
      event_db_destroy (ev);
      return;
    }

  memset (&tm_daystart, 0, sizeof (struct tm));
  tm_daystart.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
  tm_daystart.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
  tm_daystart.tm_mday = GTK_DATE_COMBO (s->startdate)->day;
      
  if (((start_t < time(NULL)) && !(ev->flags & FLAG_UNTIMED))
    || ((ev->flags & FLAG_UNTIMED) && (start_t < mktime(&tm_daystart))))
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
          event_db_destroy (ev);
          return;
        }
    }
    
  ev->start = start_t;
  ev->duration = end_t - start_t;

  if (ev_d->description)
    g_free (ev_d->description);

    buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description));
    gtk_text_buffer_get_bounds (buf, &start, &end);
    ev_d->description = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  if (ev_d->summary)
    g_free (ev_d->summary);

  ev_d->summary = gtk_editable_get_chars (GTK_EDITABLE (s->summary), 0, -1);
  
  if (ev_d->location)
    g_free (ev_d->location);
  ev_d->location = gtk_editable_get_chars (GTK_EDITABLE (s->location), 0, -1);

  g_slist_free (ev_d->categories);
  ev_d->categories = g_slist_copy (s->categories);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
    {
      unsigned int mi;
 
      mi = gtk_option_menu_get_history (GTK_OPTION_MENU (s->alarmoption));
      
      ev->flags |= FLAG_ALARM;
      ev->alarm = alarm_multipliers[mi] * gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->alarmspin));
    }
  else
    ev->flags &= ~FLAG_ALARM;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone)))
    {
      if (ev->recur)
        g_free (ev->recur);
      ev->flags |= FLAG_RECUR;
      ev->recur = NULL;
    }
  else
    {
      recur_t r = event_db_get_recurrence (ev);
      ev->flags &= ~FLAG_RECUR;
      
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttondaily)))
        {
          r->type = RECUR_DAILY;
          r->increment = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->dailyspin));
          r->daymask = 0;
        }
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonweekly)))
        {
          guint i;
          r->type = RECUR_WEEKLY;
          r->increment = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->weeklyspin));
          r->daymask = 0;

          for (i = 0; i < 7; i++)
            {
              if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[i])))
                r->daymask |= 1 << i;
            }
        }
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonmonthly)))
        {
          r->type = RECUR_MONTHLY;
          r->increment=gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->monthlyspin));
          r->daymask=0;
        }
      else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonyearly)))
        {
          r->type = RECUR_YEARLY;
          r->increment=gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->yearlyspin));
          r->daymask=0;
        }

      if (gtk_toggle_button_get_active
          (GTK_TOGGLE_BUTTON (s->radiobuttonforever)))
        {
          r->count=0;
          r->end=0;
        }
      else if (gtk_toggle_button_get_active
               (GTK_TOGGLE_BUTTON (s->radiobuttonendafter)))
        {
          r->count=gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->endspin));
          r->end=0;
        }

      else if (gtk_toggle_button_get_active
               (GTK_TOGGLE_BUTTON (s->radiobuttonendon)))
        {
          char *rend = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO
                                                             (s->endtime)->entry),
                                               0, -1);
          memset (&tm_rend, 0, sizeof (struct tm));

          tm_rend.tm_year = GTK_DATE_COMBO (s->datecomboendon)->year - 1900;
          tm_rend.tm_mon = GTK_DATE_COMBO (s->datecomboendon)->month;
          tm_rend.tm_mday = GTK_DATE_COMBO (s->datecomboendon)->day;

          strptime (rend, TIMEFMT, &tm_rend);

          g_free (rend);

          rend_t = mktime (&tm_rend);

          /* If DST was in effect, mktime will have "helpfully" incremented the
             hour.  */
          if (tm_rend.tm_isdst)
            rend_t -= 60*60;

          r->count = 0;
          r->end = rend_t;
        }
    }

  if (new_event)
    event_db_add (ev);
  else
    event_db_flush (ev);

  if (ev->flags & FLAG_ALARM)
    schedule_next (0,0, gtk_widget_get_toplevel(widget));

  event_db_forget_details (ev);

  update_all_views ();

  edit_finished (d);
}

static void
check_constrains(struct edit_state *s)
{
    if ((s->page == 0) && (s->ev))
    {
      char buf[32];
      struct tm tm_start, tm_end;
      time_t start_t, end_t;
      event_t ev;
      
      ev = get_cloned_ev(s->ev);
      
      /* Appointment */
      char *start = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO
                                                          (s->starttime)->entry),
                                            0, -1);
      char *end = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO
                                                        (s->endtime)->entry),
                                          0, -1);

      memset (&tm_start, 0, sizeof (struct tm));
      tm_start.tm_year = GTK_DATE_COMBO (s->startdate)->year - 1900;
      tm_start.tm_mon = GTK_DATE_COMBO (s->startdate)->month;
      tm_start.tm_mday = GTK_DATE_COMBO (s->startdate)->day;

      memset (&tm_end, 0, sizeof (struct tm));
      tm_end.tm_year = GTK_DATE_COMBO (s->enddate)->year - 1900;
      tm_end.tm_mon = GTK_DATE_COMBO (s->enddate)->month;
      tm_end.tm_mday = GTK_DATE_COMBO (s->enddate)->day;

      strptime (start, TIMEFMT, &tm_start);
      strptime (end, TIMEFMT, &tm_end);

      start_t = mktime (&tm_start);
      end_t = mktime (&tm_end);

      /* If DST was in effect, mktime will have "helpfully" incremented the
         hour.  */
      if (tm_start.tm_isdst)
        {
          start_t -= 60*60;
          end_t -= 60*60;
        }

      if (start_t == end_t)
        {
          tm_end.tm_hour+=1;
          strftime (buf, sizeof(buf), TIMEFMT, &tm_end);
          gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->endtime)->entry), buf);
        }
      g_free (end);
      g_free (start);
      ev->start = start_t;
      ev->duration = end_t - start_t;
    }

}

static gboolean
set_notebook_page (GtkWidget *w, struct edit_state *s)
{
  guint i = gtk_option_menu_get_history (GTK_OPTION_MENU (w));
  gtk_notebook_set_page (GTK_NOTEBOOK (s->notebooktype), i);
  s->page = i;
  check_constrains(s);
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
update_categories (GtkWidget *ui, GSList *new, struct edit_state *s)
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
  GtkWidget *menutypehbox;
  GtkWidget *menutypelabel;

  /* Summary widgets */
  GtkWidget *summaryhbox;
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
  
  /* if screen is large enough, make it a real dialog */
  if ((gdk_screen_width() >= 300) && (gdk_screen_height() >= 300))
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

  menutypehbox        = gtk_hbox_new (FALSE, boxspacing);
  menutypelabel       = gtk_label_new (_("Type:"));
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

  gtk_box_pack_start (GTK_BOX (menutypehbox), menutypelabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (menutypehbox), s->optionmenutype, TRUE, TRUE, 0);
  /* Building the summary wigets */
  summaryhbox         = gtk_hbox_new (FALSE, boxspacing);
  summaryentry        = gtk_entry_new ();
  summarylabel        = gtk_label_new (_("Text:"));
  
  gtk_misc_set_alignment (GTK_MISC (summarylabel),0,0.5);

  gtk_box_pack_start (GTK_BOX (summaryhbox), summarylabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (summaryhbox), summaryentry, TRUE, TRUE, 0);

  /* table for start/end time/date */
  startendtable       = gtk_table_new (2, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (startendtable), boxspacing);
  gtk_table_set_row_spacings (GTK_TABLE (startendtable), boxspacing);

  starttime           = gtk_combo_new ();
  endtime             = gtk_combo_new ();

#ifdef IS_HILDON
  gtk_widget_set_size_request (starttime, 96, -1);
  gtk_widget_set_size_request (endtime, 96, -1);
#else
  gtk_widget_set_size_request (starttime, 42, -1);
  gtk_widget_set_size_request (endtime, 42, -1);
#endif
  gtk_combo_set_popdown_strings (GTK_COMBO (starttime), times);
  gtk_combo_set_popdown_strings (GTK_COMBO (endtime), times);
  gtk_combo_set_use_arrows (GTK_COMBO(starttime), FALSE);
  gtk_combo_set_use_arrows (GTK_COMBO(endtime), FALSE);

  startdatelabel      = gtk_label_new (_("Start at:"));
  enddatelabel        = gtk_label_new (_("End at:"));
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

  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->startdate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->enddate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->reminderdate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->taskdate), week_starts_monday);

  gtk_table_attach (GTK_TABLE (startendtable), startdatelabel, 0, 1, 0, 1,
                    0, 0, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (startendtable), starttime, 1, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), starttimelabel, 2, 3, 0, 1,
                    0, 0, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (startendtable), s->startdate, 3, 4, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), enddatelabel, 0, 1, 1, 2,
                    0, 0, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (startendtable), endtime, 1, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (startendtable), endtimelabel, 2, 3, 1, 2,
                    0, 0, 0, boxspacing);
  gtk_table_attach (GTK_TABLE (startendtable), s->enddate, 3, 4, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  gtk_box_pack_start (GTK_BOX (vboxappointment), startendtable, FALSE, FALSE, 0);

  datetimetable       = gtk_table_new (2, 2, FALSE);

  datelabel           = gtk_label_new (_("Date:"));
  gtk_misc_set_alignment (GTK_MISC (datelabel), 0.0, 0.5);

  s->remindertimebutton = gtk_check_button_new_with_label (_("Time:"));
  g_signal_connect (G_OBJECT (s->remindertimebutton), "clicked",
                    G_CALLBACK (recalculate_sensitivities), window);

  s->remindertime     = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (s->remindertime), times);
  gtk_combo_set_use_arrows (GTK_COMBO(s->remindertime), FALSE);

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

  gtk_box_pack_start (GTK_BOX (vboxevent), menutypehbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxevent), summaryhbox, FALSE, FALSE, 0);
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
                    G_CALLBACK (weekly_toggled), window);
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

  gchar *gs;
  
  for (i = 0; i < 3; i++)
    {
      gs = g_locale_to_utf8 (nl_langinfo(days[i]), -1, NULL, NULL, NULL);
      GtkWidget *b = gtk_check_button_new_with_label (gs);
      gtk_table_attach_defaults (GTK_TABLE (weeklytable), b, i, i + 1, 0, 1);
      s->checkbuttonwday[i] = b;
      g_free (gs);
    }
  for (i = 3; i < 6; i++)
    {
      gs = g_locale_to_utf8 (nl_langinfo(days[i]), -1, NULL, NULL, NULL);
      GtkWidget *b = gtk_check_button_new_with_label (gs);
      gtk_table_attach_defaults (GTK_TABLE (weeklytable), b, i - 3, i - 2, 1, 2);
      s->checkbuttonwday[i] = b;
      g_free(gs);
    }
  for (i = 6; i < 7; i++)
    {
      gs = g_locale_to_utf8 (nl_langinfo(days[i]), -1, NULL, NULL, NULL);
      GtkWidget *b = gtk_check_button_new_with_label (gs);
      gtk_table_attach_defaults (GTK_TABLE (weeklytable), b, i - 6, i - 5, 2, 3);
      s->checkbuttonwday[i] = b;
      g_free (gs);
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
  endlabel= gtk_label_new (_("events"));
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
  gtk_widget_grab_default(summaryentry);
  g_object_set_data(G_OBJECT(window), "default-entry", summaryentry);
  
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
    }
  else
    w = build_edit_event_window ();

  return w;
}

void
event_ui_init (void)
{
  cached_window = build_edit_event_window ();
}

GtkWidget *
new_event (time_t t, guint timesel)
{
  GtkWidget *entry, *w = edit_event_window ();

  if (w)
    {
      struct tm tm;
      char buf[32];
      struct edit_state *s = g_object_get_data (G_OBJECT (w),
                                                "edit_state");

      gtk_window_set_title (GTK_WINDOW (w), _("Calendar: New event"));

      s->ev = NULL;
      gtk_widget_set_sensitive (s->deletebutton, FALSE);

      localtime_r (&t, &tm);
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->starttime)->entry), buf);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->reminderdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      t += 60 * 60;
      localtime_r (&t, &tm);
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->endtime)->entry), buf);
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
	  
      entry = g_object_get_data(G_OBJECT(w), "default-entry");
      if (entry)
        {
	      gtk_widget_grab_focus(entry);
          gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
        }
	}

  return w;
}

GtkWidget *
edit_event (event_t ev)
{
  GtkWidget *w = edit_event_window ();
  event_details_t evd;

  if (w)
    {
      time_t end;
      struct tm tm;
      char buf[32];
      struct edit_state *s = g_object_get_data (G_OBJECT (w),
                                                "edit_state");

      gtk_window_set_title (GTK_WINDOW (w), _("Calendar: Edit event"));

      gtk_widget_set_sensitive (s->deletebutton, TRUE);
      evd = event_db_get_details (ev);
      gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description)),
				evd->description ? evd->description : "", -1);
      gtk_entry_set_text (GTK_ENTRY (s->summary), evd->summary ? evd->summary : "");
      gtk_entry_set_text (GTK_ENTRY (s->location), evd->location ? evd->location : "");
      update_categories (w, evd->categories, s);
      event_db_forget_details (ev);

      localtime_r (&(ev->start), &tm);
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->starttime)->entry), buf);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->reminderdate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      end=ev->start+ev->duration;
      localtime_r (&end, &tm);
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->endtime)->entry), buf);
      gtk_notebook_set_page (GTK_NOTEBOOK (s->notebookedit), 0);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
                               tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);

      /* we assume that "Appointment" is at index = 0
       * and "Reminder" is at index = 1 */
      gtk_option_menu_set_history (GTK_OPTION_MENU (s->optionmenutype),
                                   ev->duration == 0);
      s->page = (ev->duration == 0);
      gtk_notebook_set_page (GTK_NOTEBOOK (s->notebooktype), s->page);
 
      if (ev->flags & FLAG_ALARM)
        {
          unsigned int unit = 0;	/* minutes */

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), TRUE);

	  if (ev->alarm)
	    {
	      unsigned int i;

	      for (i = 0; i < 4; i++)
            {
		      if ((ev->alarm % alarm_multipliers[i]) == 0)
		        unit = i;
            }
	    }

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->alarmspin), ev->alarm / alarm_multipliers[unit]);
	  gtk_option_menu_set_history (GTK_OPTION_MENU (s->alarmoption), unit);
        }
      else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), FALSE);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttondaily), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonweekly), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonmonthly), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonyearly), FALSE);

      if (ev->recur)
        {
          recur_t r = ev->recur;

          switch (r->type)
            {
            case RECUR_NONE:
              abort ();

            case RECUR_DAILY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttondaily), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->dailyspin), r->increment);
              break;

            case RECUR_WEEKLY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonweekly), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->weeklyspin), r->increment);
              if (r->daymask & MON) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[0]), 1);
              if (r->daymask & TUE) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[1]), 1);
              if (r->daymask & WED) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[2]), 1);
              if (r->daymask & THU) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[3]), 1);
              if (r->daymask & FRI) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[4]), 1);
              if (r->daymask & SAT) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[5]), 1);
              if (r->daymask & SUN) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->checkbuttonwday[6]), 1);
              break;

            case RECUR_MONTHLY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonmonthly), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->monthlyspin), r->increment);
              break;

            case RECUR_YEARLY:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonyearly), TRUE);
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->yearlyspin), r->increment);
              break;
            }

          if (r->end)
            {
              localtime_r (&r->end, &tm);
              gtk_date_combo_set_date (GTK_DATE_COMBO (s->datecomboendon),
                                       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonendon),
                                            TRUE);
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonforever),
                                            FALSE);
            }
          else
            {
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonendon),
                                            FALSE);
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonforever),
                                            TRUE);
            }
        }
	else 
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone), TRUE);
	
	s->ev = ev;
    }

  return w;
}
