/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
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
#include <string.h>
#include <time.h>
#include <libintl.h>
#include <langinfo.h>

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/errorbox.h>
#include <gpe/gtksimplemenu.h>
#include <gpe/event-db.h>

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

  guint page;
  
  event_t ev;
};

static GtkWidget *cached_window;

static void
gtk_box_pack_start_show (GtkBox *box,
			 GtkWidget *child,
			 gboolean expand,
			 gboolean fill,
			 guint padding)
{
  gtk_widget_show (child);
  gtk_box_pack_start (box, child, expand, fill, padding);
}

static void
destroy_user_data (gpointer p)
{
  g_free (p);
}

static void
recalculate_sensitivities (GtkWidget *widget,
			   GtkWidget *d)
{
  struct edit_state *s = gtk_object_get_data (GTK_OBJECT (d), "edit_state");

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
schedule_alarm(event_t ev, time_t start)
{
  time_t alarm_t;
  event_details_t ev_d;
  char filename[256];
  FILE *f;
	
  alarm_t = start-60*ev->alarm;
  ev_d = event_db_get_details (ev);
  
  sprintf(filename, "/var/spool/at/%ld.%ld", (long)alarm_t, ev->uid);
  
  if (fopen(filename, "w"))
  {
    if ((f=fopen(filename, "w")))
    {
      fprintf(f, "#!/bin/sh\n");
      fprintf(f, "export DISPLAY=:0.0\n");
      fprintf(f, "/usr/bin/gpe-announce '%s'\n", ev_d->summary);
      fprintf(f, "/usr/bin/gpe-calendar -s %ld -e %ld &\n", (long)alarm_t, ev->uid);
      fprintf(f, "/bin/rm $0\n");

      fclose(f);

      chmod (filename, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
      
      f=NULL;
      f=fopen("/var/spool/at/trigger","w");
      if (f != NULL) 
      {
        fprintf(f,"\n");
        fclose(f);
      }
    }
    
  }
  
}

static void
unschedule_alarm(event_t ev)
{
  time_t alarm_t;
  char filename[256], command[256];
  FILE *f;
  	
  alarm_t = ev->start-60*ev->alarm;
  
  sprintf(filename, "/var/spool/at/%ld.%ld", (long)alarm_t, ev->uid);
  sprintf(command, "rm %s", filename);
  system(command);
  
  f=NULL;
  f=fopen("/var/spool/at/trigger","w");
  if (f != NULL) 
  {
    fprintf(f,"\n");
    fclose(f);
  }

}

void
schedule_next(guint skip, guint uid)
{
  GSList *events=NULL, *iter;
  struct tm tm;
  time_t end, start=time(NULL);
  
  localtime_r (&start, &tm);
  tm.tm_year++;
  end=mktime (&tm);   
  
  events = event_db_list_alarms_for_period (start, end);
  
  for (iter = events; iter; iter = g_slist_next (iter))
    {
      event_t ev_real, ev = iter->data;
      
      if (ev->flags & FLAG_ALARM) {
	ev_real = event_db_find_by_uid (ev->uid);
	if (((int)(ev->start-60*ev->alarm) != (int)skip) && (uid!=ev->uid))
	  {
	    schedule_alarm (ev_real, ev->start);
	    break;
	  }
      }
    }
}
    
static void
edit_finished (GtkWidget *d)
{
  gtk_widget_hide (d);

  if (cached_window)
    gtk_widget_destroy (d);
  else
    cached_window = d;
}

static void
click_delete (GtkWidget *widget, event_t ev)
{
  GtkWidget *d = gtk_widget_get_toplevel (widget);
  
  if (ev->flags & FLAG_ALARM) 
    { 
      unschedule_alarm (ev);
      schedule_next(0,0);
    }
  
  event_db_remove (ev);
  update_current_view ();

  edit_finished (d);
}

static void
click_ok (GtkWidget *widget, GtkWidget *d)
{
  struct edit_state *s = gtk_object_get_data (GTK_OBJECT (d), "edit_state");
  event_t ev;
  event_details_t ev_d;
  struct tm tm_start, tm_end, tm_rend;
  time_t start_t, end_t, rend_t;
  gboolean new_event = FALSE;

  if (s->ev) 
    {
      ev = s->ev;
      ev_d = event_db_get_details (ev);
      if (ev->flags & FLAG_ALARM) 
	unschedule_alarm (ev);
      ev_d->sequence++;
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
  
  ev->start = start_t;
  ev->duration = end_t - start_t;
      
  if (ev_d->description)
    g_free (ev_d->description);

#if GTK_MAJOR_VERSION < 2
  ev_d->description = gtk_editable_get_chars (GTK_EDITABLE (s->description), 
					      0, -1);
#else
  {
    GtkTextIter start, end;
    GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description));
    gtk_text_buffer_get_bounds (buf, &start, &end);
    ev_d->description = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  }
#endif

  if (ev_d->summary)
    g_free (ev_d->summary);

  ev_d->summary = gtk_editable_get_chars (GTK_EDITABLE (s->summary), 
					  0, -1);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
    {
      ev->flags |= FLAG_ALARM;
      ev->alarm = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (s->alarmspin));
    }
  else
    ev->flags &= ~FLAG_ALARM;
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->radiobuttonnone)))
    {
      if (ev->recur)
	g_free (ev->recur);
      ev->recur = NULL;
    }
  else
    {
      recur_t r = event_db_get_recurrence (ev);

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
	  memset(&tm_rend, 0, sizeof (struct tm));
	  
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
    schedule_next (0, 0);

  event_db_forget_details (ev);
  
  update_current_view ();
      
  edit_finished (d);
}

static void
click_cancel (GtkWidget *widget,
	      GtkWidget *d)
{
  edit_finished (d);
}

static gboolean
set_notebook_page (GtkWidget *w, struct edit_state *s)
{
  guint i = gtk_simple_menu_get_result (GTK_SIMPLE_MENU (w));
  gtk_notebook_set_page (GTK_NOTEBOOK (s->notebooktype), i);
  s->page = i;
  gtk_widget_draw (s->notebooktype, NULL);
  return FALSE;
}

static GtkWidget *
build_edit_event_window (void)
{
  static const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, 
				  ABDAY_6, ABDAY_7, ABDAY_1 };
  int i;
  
  GSList *vboxrecur_group, *vboxend_group;
  GtkWidget *radiobuttonforever, *radiobuttonendafter;
  GtkWidget *hboxendon, *radiobuttonendon, *datecomboendon;
  GtkWidget *endspin, *endlabel;
  GtkWidget *radiobuttonnone, *radiobuttondaily, *radiobuttonweekly, 
    *radiobuttonmonthly, *radiobuttonyearly;
  GtkWidget *recurborder, *recurendborder;
  GtkWidget *vboxrepeattop;

  GtkWidget *dailylabelevery, *dailyspin, *dailylabels;
  GtkWidget *monthlylabelevery, *monthlyspin, *monthlylabels;
  GtkWidget *yearlylabelevery, *yearlyspin, *yearlylabels;

  GtkAdjustment *endspin_adj, *dailyspin_adj, *monthlyspin_adj, *yearlyspin_adj;
  GtkAdjustment *taskadj;
   
  GtkWidget *notebookedit = gtk_notebook_new ();
  GtkWidget *labeleventpage = gtk_label_new (_("Event"));
  GtkWidget *labelrecurpage = gtk_label_new (_("Recurrence"));
  GtkWidget *labelalarmpage = gtk_label_new (_("Alarm"));

  GtkWidget *vboxevent = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxrepeat = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxalarm = gtk_vbox_new (FALSE, 0);
  
  GtkWidget *vboxend = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxtop = gtk_vbox_new (FALSE, 0);
  
  GtkWidget *vboxrecur = gtk_vbox_new (FALSE, 0);
  GtkWidget *hboxrecurtypes = gtk_hbox_new (FALSE, 0);
  GtkWidget *hboxendafter = gtk_hbox_new (FALSE, 0);
  
#if GTK_MAJOR_VERSION < 2
  GtkWidget *description = gtk_text_new (NULL, NULL);
#else
  GtkWidget *description = gtk_text_view_new ();
#endif

  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok;
  GtkWidget *buttoncancel;
  GtkWidget *buttondelete;

  GtkWidget *starttime = gtk_combo_new ();
  GtkWidget *endtime = gtk_combo_new ();

  GtkWidget *startdatebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *enddatebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *startdatelabel = gtk_label_new (_("Start on:"));
  GtkWidget *enddatelabel = gtk_label_new (_("End on:"));
  GtkWidget *starttimelabel = gtk_label_new (_("at:"));
  GtkWidget *endtimelabel = gtk_label_new (_("at:"));

  GtkWidget *summaryhbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *summaryentry = gtk_entry_new ();
  GtkWidget *summarylabel = gtk_label_new (_("Summary:"));

  GtkWidget *descriptionframe = gtk_frame_new (_("Description:"));

  GtkWidget *alarmhbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *alarmmenu = gtk_menu_new ();
  GtkWidget *alarmbutton = gtk_check_button_new_with_label (_("Alarm"));
  GtkObject *alarmadj = gtk_adjustment_new (5.0, 1.0, 100.0, 1.0, 5.0, 5.0);
  GtkWidget *alarmspin = gtk_spin_button_new (GTK_ADJUSTMENT (alarmadj), 
					      1.0, 0);
  GtkWidget *alarmoption = gtk_option_menu_new ();
  GtkWidget *alarmbefore = gtk_label_new (_("before event"));

  GtkWidget *weeklyhbox3 = gtk_hbox_new (FALSE, 0);
  GtkWidget *weeklylabelevery = gtk_label_new (_("Every"));
  GtkWidget *weeklylabelweeks = gtk_label_new (_("week(s), on:"));

  GtkAdjustment *weeklyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 52, 1, 5, 5);
  GtkWidget *weeklyspin = gtk_spin_button_new (GTK_ADJUSTMENT (weeklyspin_adj), 1, 0);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *weeklyhbox1 = gtk_hbox_new (FALSE, 0);
  GtkWidget *weeklyhbox2 = gtk_hbox_new (FALSE, 0);
  GtkWidget *monthlyhbox1 = gtk_hbox_new (FALSE, 4);
  GtkWidget *monthlyhbox2 = gtk_hbox_new (FALSE, 4);
  GSList *radiogroup;
  GtkWidget *daybutton, *weekbutton;
  GtkWidget *optionmenu1, *optionmenu2;
  char buf[64];
  GtkWidget *scrolledwindowevent;
  GtkWidget *vboxappointment = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxreminder = gtk_vbox_new (FALSE, 0);
  GtkWidget *vboxtask = gtk_vbox_new (FALSE, 0);
  GtkWidget *hboxreminder1 = gtk_hbox_new (FALSE, 0);
  GtkWidget *hboxreminder2 = gtk_hbox_new (FALSE, 0);
  GtkWidget *hboxtask1 = gtk_hbox_new (FALSE, 0);
  GtkWidget *hboxtask2 = gtk_hbox_new (FALSE, 0);
  
  GtkWidget *menu1 = gtk_menu_new ();
  GtkWidget *menu2 = gtk_menu_new ();
  struct edit_state *s = g_malloc (sizeof (struct edit_state));

  memset (s, 0, sizeof (*s));

  s->optionmenutype = gtk_simple_menu_new ();
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutype), _("Appointment"));
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutype), _("Reminder"));
#if 0
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutype), _("Task"));
#endif
  gtk_signal_connect (GTK_OBJECT (s->optionmenutype), "item-changed", 
		      GTK_SIGNAL_FUNC (set_notebook_page), s);

  s->optionmenutask = gtk_simple_menu_new ();
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutask), _("Day(s)"));
  gtk_simple_menu_append_item (GTK_SIMPLE_MENU (s->optionmenutask), _("Week(s)"));
  
/* Begin event vbox */
  gtk_combo_set_popdown_strings (GTK_COMBO (starttime), times);
  gtk_combo_set_popdown_strings (GTK_COMBO (endtime), times);

  s->startdate = gtk_date_combo_new ();
  s->enddate = gtk_date_combo_new ();
  s->reminderdate = gtk_date_combo_new ();
  s->taskdate = gtk_date_combo_new ();

  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->startdate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->enddate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->reminderdate), week_starts_monday);
  gtk_date_combo_week_starts_monday (GTK_DATE_COMBO (s->taskdate), week_starts_monday);
 
  gtk_box_pack_end (GTK_BOX (startdatebox), starttime, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (startdatebox), starttimelabel, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (startdatebox), s->startdate, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (startdatebox), startdatelabel, FALSE, FALSE, 2);

  gtk_box_pack_end (GTK_BOX (enddatebox), endtime, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (enddatebox), endtimelabel, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (enddatebox), s->enddate, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (enddatebox), enddatelabel, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vboxappointment), startdatebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxappointment), enddatebox, FALSE, FALSE, 0);
  gtk_widget_show_all (vboxappointment);

  gtk_box_pack_start (GTK_BOX (hboxreminder1), gtk_label_new (_("Date:")), FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hboxreminder1), s->reminderdate, TRUE, TRUE, 2);

  s->remindertimebutton = gtk_check_button_new_with_label (_("Time:"));
  gtk_signal_connect (GTK_OBJECT (s->remindertimebutton), "clicked", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  s->remindertime = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (s->remindertime), times);
  gtk_box_pack_start (GTK_BOX (hboxreminder2), s->remindertimebutton, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hboxreminder2), s->remindertime, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (vboxreminder), hboxreminder1, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxreminder), hboxreminder2, FALSE, FALSE, 0);
  gtk_widget_show_all (vboxreminder);

  taskadj = (GtkAdjustment *)gtk_adjustment_new (1, 1, 100, 1, 10, 10);
  s->taskspin = gtk_spin_button_new (taskadj, 1, 0);
  gtk_box_pack_start (GTK_BOX (hboxtask1), gtk_label_new (_("Due:")), FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hboxtask1), s->taskdate, TRUE, TRUE, 2);
  
  gtk_box_pack_start (GTK_BOX (hboxtask2), gtk_label_new (_("Allow:")), FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hboxtask2), s->taskspin, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hboxtask2), s->optionmenutask, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (vboxtask), hboxtask1, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxtask), hboxtask2, FALSE, FALSE, 0);
  gtk_widget_show_all (vboxtask);

  s->notebooktype = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (s->notebooktype), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (s->notebooktype), FALSE);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebooktype), vboxappointment, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebooktype), vboxreminder, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (s->notebooktype), vboxtask, NULL);

#if GTK_MAJOR_VERSION < 2
  gtk_text_set_editable (GTK_TEXT (description), TRUE);
  gtk_text_set_word_wrap (GTK_TEXT (description), TRUE);
#else
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (description), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (description), TRUE);
#endif
  gtk_widget_set_usize (description, -1, 88);

  buttonok = gpe_button_new_from_stock (GTK_STOCK_SAVE, GPE_BUTTON_TYPE_BOTH);
  buttoncancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);
  buttondelete = gpe_button_new_from_stock (GTK_STOCK_DELETE, GPE_BUTTON_TYPE_BOTH);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);

  gtk_container_add (GTK_CONTAINER (descriptionframe), description);
  gtk_container_set_border_width (GTK_CONTAINER (descriptionframe), 2);

  gtk_box_pack_start (GTK_BOX (summaryhbox), summarylabel, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (summaryhbox), summaryentry, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (vboxevent), s->optionmenutype, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxevent), summaryhbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxevent), s->notebooktype, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxevent), descriptionframe, TRUE, TRUE, 2);
  
  gtk_widget_show_all (vboxevent);
  
/* Begin repeat vbox */

  gtk_widget_show (vboxrepeat);
  
  vboxrepeattop = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vboxrepeattop);
  
  recurborder = gtk_frame_new (_("Type:"));
  gtk_box_pack_start_show (GTK_BOX (vboxrepeattop), recurborder, 
			   TRUE, TRUE, 0); 
  gtk_container_add (GTK_CONTAINER (recurborder), vboxrepeat);
   
  vboxrecur = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), vboxrecur, FALSE, FALSE, 0);

/* recurrence radio buttons */
  radiobuttonnone = gtk_radio_button_new_with_label (NULL, _("No recurrence"));
  gtk_box_pack_start_show (GTK_BOX (vboxrecur), radiobuttonnone, 
			   FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonnone), TRUE);
  
  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonnone));
  radiobuttondaily = gtk_radio_button_new_with_label (vboxrecur_group, _("Daily"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttondaily, 
			   FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttondaily));
  radiobuttonweekly = gtk_radio_button_new_with_label (vboxrecur_group, _("Weekly"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttonweekly, 
			   FALSE, FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxrecur), hboxrecurtypes, FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonweekly));
  radiobuttonmonthly = gtk_radio_button_new_with_label (vboxrecur_group, _("Monthly"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttonmonthly, 
			   FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonmonthly));
  radiobuttonyearly = gtk_radio_button_new_with_label (vboxrecur_group, _("Yearly"));
  gtk_box_pack_start_show (GTK_BOX (hboxrecurtypes), radiobuttonyearly, 
			   FALSE, FALSE, 0);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonnone), TRUE);

  gtk_signal_connect (GTK_OBJECT (radiobuttonnone), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttondaily), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonweekly), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonmonthly), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonyearly), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  
/* daily hbox */
  s->dailybox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->dailybox, 
			   TRUE, FALSE, 0);

  /* "every" label */
  dailylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start_show (GTK_BOX (s->dailybox), dailylabelevery, 
			   FALSE, FALSE, 0);
  
  /* daily spinner */
  dailyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  dailyspin = gtk_spin_button_new (GTK_ADJUSTMENT (dailyspin_adj), 1, 0);
  gtk_box_pack_start_show (GTK_BOX (s->dailybox), dailyspin, FALSE, FALSE, 0);

  /* days label */
  dailylabels = gtk_label_new (_("day(s)"));
  gtk_box_pack_start_show (GTK_BOX (s->dailybox), dailylabels, 
			   FALSE, FALSE, 0);
  
/* weekly box */
  s->weeklybox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->weeklybox, TRUE, FALSE, 0);

  /* weekly hbox3 */
  /*gtk_box_pack_start_show (GTK_BOX (s->weeklybox), weeklyhbox3, FALSE, FALSE, 0);*/
  gtk_box_pack_start_show (GTK_BOX (weeklyhbox3), weeklylabelevery, 
			     FALSE, FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (weeklyhbox3), weeklyspin, 
			     FALSE, FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (weeklyhbox3), weeklylabelweeks, 
			     FALSE, FALSE, 0);

/* weekly hbox1 */
  gtk_box_pack_start_show (GTK_BOX (s->weeklybox), weeklyhbox1, FALSE, FALSE, 0);
  for (i = 0; i < 4; i++)
    {
      GtkWidget *b = gtk_check_button_new_with_label (nl_langinfo(days[i]));
      gtk_box_pack_start_show (GTK_BOX (weeklyhbox1), b, FALSE, FALSE, 0);
      s->checkbuttonwday[i] = b;
    }

/* weekly hbox2 */
  gtk_box_pack_start_show (GTK_BOX (s->weeklybox), weeklyhbox2, FALSE, FALSE, 0);
  for (i = 4; i < 7; i++)
    {
      GtkWidget *b = gtk_check_button_new_with_label (nl_langinfo(days[i]));
      gtk_box_pack_start_show (GTK_BOX (weeklyhbox2), b, FALSE, FALSE, 0);
      s->checkbuttonwday[i] = b;
    }

/* monthly hbox */
  

  s->monthlybox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->monthlybox, FALSE, FALSE, 0);

  gtk_box_pack_start_show (GTK_BOX (s->monthlybox), monthlyhbox1, TRUE, FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (s->monthlybox), monthlyhbox2, TRUE, FALSE, 0);

  /* "every" label */
  monthlylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start_show (GTK_BOX (monthlyhbox1), monthlylabelevery, 
  			   FALSE, FALSE, 0);

  /* monthly spinner */
  monthlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 12, 1, 2, 2);
  monthlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (monthlyspin_adj), 1, 0);
  gtk_box_pack_start_show (GTK_BOX (monthlyhbox1), monthlyspin, 
  			   FALSE, FALSE, 0);

  /* months label */
  monthlylabels = gtk_label_new (_("month(s)"));
  gtk_box_pack_start_show (GTK_BOX (monthlyhbox1), monthlylabels, 
  			   FALSE, FALSE, 0);


  snprintf (buf, sizeof(buf), _("on day %d"), 5);
  daybutton = gtk_radio_button_new_with_label (NULL, buf);
  /*gtk_box_pack_start_show (GTK_BOX (s->monthlybox), daybutton, FALSE, FALSE, 0);*/

  radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (daybutton));
  weekbutton = gtk_radio_button_new_with_label (radiogroup, _("on the"));

  /*gtk_box_pack_start_show (GTK_BOX (monthlyhbox2), weekbutton, FALSE, FALSE, 0);*/

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

      /*gtk_box_pack_start_show (GTK_BOX (monthlyhbox2), optionmenu1, FALSE, FALSE, 0);
    gtk_box_pack_start_show (GTK_BOX (monthlyhbox2), optionmenu2, FALSE, FALSE, 0);*/
    
  /* yearly hbox */
  s->yearlybox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (vboxrepeat), s->yearlybox, TRUE, FALSE, 0);

  /* "every" label */
  yearlylabelevery = gtk_label_new (_("Every"));
  gtk_box_pack_start_show (GTK_BOX (s->yearlybox), yearlylabelevery, 
			   FALSE, FALSE, 0);
  
  /* yearly spinner */
  yearlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  yearlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (yearlyspin_adj), 1, 0);
  gtk_box_pack_start_show (GTK_BOX (s->yearlybox), yearlyspin, FALSE, FALSE, 0);

  /* years label */
  yearlylabels = gtk_label_new (_("year(s)"));
  gtk_box_pack_start_show (GTK_BOX (s->yearlybox), yearlylabels, 
			   FALSE, FALSE, 0);
  
  gtk_widget_hide (s->dailybox);
  gtk_widget_hide (s->weeklybox);
  gtk_widget_hide (s->monthlybox);
  gtk_widget_hide (s->yearlybox);

/* Begin end-date vbox */
 
  gtk_widget_show (vboxend);

  recurendborder = gtk_frame_new (_("End Date:"));
  gtk_container_add (GTK_CONTAINER (recurendborder), vboxend);
  gtk_box_pack_end (GTK_BOX (vboxrepeattop), recurendborder, 
		    FALSE, FALSE, 0);
  gtk_widget_show (recurendborder);
  
  /* forever radio button */
  radiobuttonforever = gtk_radio_button_new_with_label (NULL, _("forever"));
  gtk_box_pack_start_show (GTK_BOX (vboxend), radiobuttonforever, 
			   FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonforever , FALSE);

  /* endafter hbox */
  /*gtk_box_pack_start_show (GTK_BOX (vboxend), hboxendafter, FALSE, FALSE, 0);*/

  /* end after radio button */
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonforever));
  radiobuttonendafter = gtk_radio_button_new_with_label (vboxend_group, 
							 _("end after"));
  gtk_box_pack_start_show (GTK_BOX (hboxendafter), radiobuttonendafter, 
			   FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonendafter, FALSE);
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonendafter));
  		  
  /* end after spinner */
  endspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  endspin = gtk_spin_button_new (GTK_ADJUSTMENT (endspin_adj), 1, 0);
  gtk_widget_set_usize (endspin, 50, 20);
  gtk_box_pack_start_show (GTK_BOX (hboxendafter), endspin, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endspin, FALSE);

  /* end on hbox */
  hboxendon = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start_show (GTK_BOX (vboxend), hboxendon, FALSE, FALSE, 0);
  radiobuttonendon = gtk_radio_button_new_with_label (vboxend_group,
						      _("end on"));
  gtk_box_pack_start_show (GTK_BOX (hboxendon), radiobuttonendon,
			   FALSE, FALSE, 0);
  datecomboendon = gtk_date_combo_new ();
  gtk_box_pack_start_show (GTK_BOX (hboxendon), datecomboendon,
			   TRUE, TRUE, 2);

  /* events label */
  endlabel= gtk_label_new (_("event(s)"));
  gtk_box_pack_start_show (GTK_BOX (hboxendafter), endlabel, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endlabel, FALSE);

  gtk_signal_connect (GTK_OBJECT (radiobuttonforever), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonendafter), "toggled", 
		      GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  		  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonforever), TRUE);

  /* begin alarm page */
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("minute(s)")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("hour(s)")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("day(s)")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("week(s)")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("month(s)")));
  gtk_menu_append (GTK_MENU (alarmmenu), 
		   gtk_menu_item_new_with_label (_("year(s)")));
		
  gtk_option_menu_set_menu (GTK_OPTION_MENU (alarmoption), alarmmenu);
  gtk_widget_set_usize (alarmoption, 120, -1);

  gtk_signal_connect (GTK_OBJECT (alarmbutton), "clicked",
		     GTK_SIGNAL_FUNC (recalculate_sensitivities), window);

  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmspin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmoption, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmbefore, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vboxalarm), alarmbutton, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxalarm), alarmhbox, FALSE, FALSE, 0);

  gtk_widget_show_all (vboxalarm);

  gtk_widget_show (labeleventpage);
  gtk_widget_show (labelalarmpage);
  gtk_widget_show (labelrecurpage);
  
  gtk_box_pack_start_show (GTK_BOX (buttonbox), buttondelete, TRUE, FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (buttonbox), buttoncancel, TRUE, FALSE, 4);
  gtk_box_pack_start_show (GTK_BOX (buttonbox), buttonok, TRUE, FALSE, 4);

  gtk_box_pack_start_show (GTK_BOX (vboxtop), notebookedit, TRUE, TRUE, 2);
  gtk_box_pack_start_show (GTK_BOX (vboxtop), buttonbox, FALSE, FALSE, 2);

  gtk_widget_show (vboxtop);
  gtk_container_add (GTK_CONTAINER (window), vboxtop);

  scrolledwindowevent = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindowevent);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindowevent),
					 vboxevent);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindowevent),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebookedit), scrolledwindowevent, 
			    labeleventpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebookedit), vboxalarm,
			    labelalarmpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebookedit), vboxrepeattop, 
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
   
  gtk_object_set_data_full (GTK_OBJECT (window), "edit_state", s, 
			    destroy_user_data);

  recalculate_sensitivities (NULL, window);

  gpe_set_window_icon (window, "icon");

  gtk_window_set_default_size (GTK_WINDOW (window), 280, 320);
  
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (edit_finished), NULL);

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
  GtkWidget *w = edit_event_window ();
  
  if (w)
    {
      struct tm tm;
      char buf[32];
      struct edit_state *s = gtk_object_get_data (GTK_OBJECT (w), 
						  "edit_state");

      gtk_window_set_title (GTK_WINDOW (w), _("Calendar: New event"));
  
      s->ev = NULL;
      gtk_widget_set_sensitive (s->deletebutton, FALSE);
      
      localtime_r (&t, &tm);
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->starttime)->entry), buf);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
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
#if GTK_MAJOR_VERSION >= 2
      gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description)), "", -1);
#else
#error needs fixing
#endif
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
      struct edit_state *s = gtk_object_get_data (GTK_OBJECT (w), 
						  "edit_state");
      
      gtk_window_set_title (GTK_WINDOW (w), _("Calendar: Edit event"));

      gtk_widget_set_sensitive (s->deletebutton, TRUE);
      gtk_signal_connect (GTK_OBJECT (s->deletebutton), "clicked",
			  GTK_SIGNAL_FUNC (click_delete), ev);
      evd = event_db_get_details (ev);
#if GTK_MAJOR_VERSION < 2
      gtk_text_insert (GTK_TEXT (s->description), NULL, NULL, NULL, 
		       evd->description, -1);
#else
      gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (s->description)),
				evd->description, -1);
#endif
      gtk_entry_set_text (GTK_ENTRY (s->summary), evd->summary);
      event_db_forget_details (ev);
      
      localtime_r (&(ev->start), &tm);
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->starttime)->entry), buf);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->startdate),
			       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      
      end=ev->start+ev->duration;
      localtime_r (&end, &tm);
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (s->endtime)->entry), buf);
      gtk_notebook_set_page (GTK_NOTEBOOK (s->notebookedit), 0);
      gtk_date_combo_set_date (GTK_DATE_COMBO (s->enddate),
			       tm.tm_year + 1900, tm.tm_mon, tm.tm_mday);
      
      if (ev->flags & FLAG_ALARM) 
	{
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (s->alarmbutton), TRUE);
	  gtk_spin_button_set_value (GTK_SPIN_BUTTON (s->alarmspin), ev->alarm);
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
	  
	s->ev = ev;
    }    
  
  return w;
}
