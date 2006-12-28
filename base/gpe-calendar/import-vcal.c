/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define ERROR_DOMAIN() g_quark_from_static_string ("gpevtype")

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>

#include <mimedir/mimedir-vcal.h>
#include <mimedir/mimedir-valarm.h>

#include <sqlite.h>
#include <gpe/vevent.h>
#include <gpe/vtodo.h>
#include <gpe/event-db.h>

#include "globals.h"
#include "calendar-edit-dialog.h"
#include "calendars-widgets.h"
#include "import-vcal.h"

#ifdef IS_HILDON
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#endif

#define TODO_DB_NAME "/.gpe/todo"

static time_t
extract_time (MIMEDirDateTime *dt)
{
  struct tm tm;
  memset (&tm, 0, sizeof (tm));

  tm.tm_year = dt->year - 1900;
  tm.tm_mon = dt->month - 1;
  tm.tm_mday = dt->day;

  if ((dt->flags & MIMEDIR_DATETIME_TIME))
    {
      tm.tm_hour = dt->hour;
      tm.tm_min = dt->minute;
      tm.tm_sec = dt->second;
    }

  /* Get mktime to figure out if dst is in effect during this time or
     not.  */
  tm.tm_isdst = -1;

  if ((dt->flags & MIMEDIR_DATETIME_TIME) == 0)
    /* Untimed: return the start of the day in the local time time zone
       in UTC.  */
    return mktime (&tm);
  else if (dt->timezone == MIMEDIR_DATETIME_NOTZ)
    /* No time zone: return this time in the local time zone in
       UTC.  */
    return mktime (&tm);
  else
    /* The time is in UTC or the time zone relative to UTC is
       provided.  */
    {
      time_t t = timegm (&tm);
      if (dt->timezone != MIMEDIR_DATETIME_UTC)
	t += dt->timezone;
      return t;
    }
}

gboolean
import_vevent (EventCalendar *ec, MIMEDirVEvent *event, Event **new_ev,
	       GError **gerror)
{
  EventDB *event_db = event_calendar_get_event_db (ec);

  gboolean error = FALSE;;
  gboolean created = FALSE;;

  Event *ev = NULL;
  char *uid = NULL;
  g_object_get (event, "uid", &uid, NULL);
  if (uid)
    ev = event_db_find_by_eventid (event_db, uid);
  if (! ev)
    {
      created = TRUE;
      ev = event_new (event_db, ec, uid);
    }
  else
    event_set_calendar (ev, ec);

  char *summary = NULL;
  g_object_get (event, "summary", &summary, NULL);

  MIMEDirDateTime *dtstart = NULL;
  g_object_get (event, "dtstart", &dtstart, NULL);
  if (! dtstart)
    {
      error = TRUE;
      g_set_error (gerror, ERROR_DOMAIN (), 0,
		   "Not important malformed event %s (%s):"
		   " lacks required field dtstart",
		   summary, uid);
      goto out;
    }

  time_t start = extract_time (dtstart);
  event_set_recurrence_start (ev, start);
  event_set_untimed (ev, (dtstart->flags & MIMEDIR_DATETIME_TIME) == 0);
  g_object_unref (dtstart);

#if 0
  /* XXX: What should we do if the event is marked as an all day
     event?  */
  mimedir_vcomponent_get_allday (MIMEDIR_VCOMPONENT (event));
#endif

  MIMEDirDateTime *dtend = NULL;
  g_object_get (event, "dtend", &dtend, NULL);
  time_t end;
  if (dtend)
    {
      end = extract_time (dtend);
      event_set_duration (ev, end - start);
      g_object_unref (dtend);
    }
  else
    {
      int duration;
      g_object_get (event, "duration", &duration, NULL);

      if (duration)
	{
	  event_set_duration (ev, duration);
	  end = start + duration;
	}
      else
	end = start;
    }


  /* Handle alarms, if any */
  const GList *alarm_list, *l;

  /* Get the alarm list.

     Note the comments in mimedri-vcomponent.c: the list must not be modified
     by the caller and the MIMEDirVAlarm objects in the list must be
     g_object_ref()'ed when they are to be used by the caller. */
  alarm_list = mimedir_vcomponent_get_alarm_list (MIMEDIR_VCOMPONENT (event));

  /* XXX: A VEvent can have multiple alarms but we only support a
     single alarm.  Take the earliest and hope for the best.  */
  int trigger_min = INT_MAX;
  for (l = alarm_list; l; l = g_list_next (l))
    {
      MIMEDirVAlarm *valarm = MIMEDIR_VALARM (l->data);

      int trigger;
      g_object_get (valarm, "trigger", &trigger, NULL);
      if (trigger)
	{
	  int alarm;
	  gboolean trigger_end;

	  g_object_get (valarm, "trigger-end", &trigger_end, NULL);
	  if (trigger_end)
	    alarm = end - trigger;
	  else
	    alarm = start - trigger;
	
	  if (alarm < 0)
	    alarm = 1;

	  trigger_min = MIN (alarm, trigger_min);
	}

      MIMEDirDateTime *triggerdt = NULL;
      g_object_get (valarm, "trigger-datetime", &triggerdt, NULL);
      if (triggerdt)
	{
	  trigger_min = MIN (extract_time (triggerdt), trigger_min);
	  g_object_unref (triggerdt);
	}
    }

  if (trigger_min != INT_MAX)
    /* Set the alarm.  */
    event_set_alarm (ev, trigger_min);

#if 0
  GList *categories = NULL;
  g_object_get (event, "category-list", &categories, NULL);

  GList *i;
  for (i = categories; i; i = i->next)
    {
      event_add_category (ev, map category name to integer...);
      g_free (i->data);
    }
  g_list_free (categories);
#endif

  if (summary)
    event_set_summary (ev, summary);

  char *description = NULL;
  g_object_get (event, "description", &description, NULL);
  if (description)
    event_set_description (ev, description);
  g_free (description);

  const char *location_uri;
  const char *location
    = mimedir_vcomponent_get_location (MIMEDIR_VCOMPONENT (event),
				       &location_uri);
  if (location && location_uri)
    {
      char *s = NULL;
      s = g_strdup_printf ("%s\n%s", location, location_uri);
      event_set_location (ev, s);
      g_free (s);
    }
  else if (location)
    event_set_location (ev, location);
  else if (location_uri)
    event_set_location (ev, location_uri);

  int sequence;
  g_object_get (event, "sequence", &sequence, NULL);
  event_set_sequence (ev, sequence);

  MIMEDirRecurrence *recurrence
    = mimedir_vcomponent_get_recurrence (MIMEDIR_VCOMPONENT (event));
  if (recurrence)
    {
      /* XXX: Add EXDATE (exceptions) when libmimedir supports it.  */
      /* XXX: Add RDATE (BYDAY, etc.) when libmimedir supports it.  */

      MIMEDirDateTime *until = NULL;
      g_object_get (recurrence, "until", &until, NULL);
      if (until)
	{
	  mimedir_datetime_to_utc (until);
	  event_set_recurrence_end (ev, mimedir_datetime_get_time_t (until));
	}

      int frequency = 0;
      g_object_get (recurrence, "frequency", &frequency, NULL);

      enum event_recurrence_type type = RECUR_NONE;
      switch (frequency)
	{
	case RECURRENCE_DAILY:
	  type = RECUR_DAILY;
	  break;
	case RECURRENCE_WEEKLY:
	  type = RECUR_WEEKLY;
	  /* Fall through.  */
	case RECURRENCE_MONTHLY:
	  if (frequency == RECURRENCE_MONTHLY)
	    type = RECUR_MONTHLY;

	  int unit = 0;
	  g_object_get (recurrence, "unit", &unit, NULL);
	  if (unit == RECURRENCE_UNIT_DAY)
	    {
	      char *units;
	      g_object_get (recurrence, "units", &units, NULL);

	      GSList *byday = NULL;
	      char *p = units;
	      while (p && *p)
		{
		  while (*p == ' ' || *p == ',')
		    p ++;
		  int prefix = strtol (p, &p, 10);
		  while (*p == ' ')
		    p ++;

		  int check (char *day, int shift)
		    {
		      if (strncmp (p, day, 2) == 0
			  && (p[2] == ',' || p[2] == ' ' || p[2] == 0))
			{
			  if (prefix == 0)
			    byday = g_slist_prepend (byday, g_strdup (day));
			  else
			    {
			      char *s = g_strdup_printf ("%d%s", prefix, day);
			      byday = g_slist_prepend (byday, s);
			    }

			  p += 2;
			  return TRUE;
			}
		      return FALSE;
		    }

		  if (check ("MO", 0))
		    continue;
		  if (check ("TU", 1))
		    continue;
		  if (check ("WE", 2))
		    continue;
		  if (check ("TH", 3))
		    continue;
		  if (check ("FR", 4))
		    continue;
		  if (check ("SA", 5))
		    continue;
		  if (check ("SU", 6))
		    continue;
		  /* Invalid character?  */
		  p = strchr (p, ',');
		}

	      event_set_recurrence_byday (ev, byday);

	      g_free (units);
	    }

	  break;
	case RECURRENCE_YEARLY:
	  type = RECUR_YEARLY;
	  break;
	default:
	  {
	    error = TRUE;
	    char *s = mimedir_recurrence_write_to_string (recurrence);
	    g_set_error (gerror, ERROR_DOMAIN (), 0,
			 "%s (%s) has unhandeled recurrence"
			 " type: %s, not importing",
			 summary, uid, s);
	    g_free (s);
	    goto out;
	  }
	}
      event_set_recurrence_type (ev, type);

      int count = 0;
      g_object_get (recurrence, "count", &count, NULL);
      event_set_recurrence_count (ev, count);

      int interval = 0;
      g_object_get (recurrence, "interval", &interval, NULL);
      if (interval)
	event_set_recurrence_increment (ev, interval);

      if (until)
	g_object_unref (until);

      /* We don't own a reference to recurrence, don't unref it.  */
    }

  event_flush (ev);
 out:
  g_free (uid);

  if (! error && new_ev)
    /* Return the new event to the caller.  */
    {
      *new_ev = ev;
      g_object_ref (ev);
    }

  if (created && error)
    event_remove (ev);
  g_object_unref (ev);
  g_free (summary);

  return ! error;
}

static gboolean
do_import_vtodo (MIMEDirVTodo *todo, GError **error)
{
  sqlite *db;
  GSList *tags, *i;
  char *buf;
  const gchar *home;
  char *err = NULL;
  int id;

  home = g_get_home_dir ();
  
  buf = g_strdup_printf ("%s%s", home, TODO_DB_NAME);

  db = sqlite_open (buf, 0, &err);
  g_free (buf);

  if (db == NULL)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, err);
      free (err);
      return FALSE;
    }
 
  if (sqlite_exec (db, "insert into todo_urn values (NULL)", NULL, NULL, &err) != SQLITE_OK)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, err);
      free (err);
      sqlite_close (db);
      return FALSE;
    }

  id = sqlite_last_insert_rowid (db);

  tags = vtodo_to_tags (todo);

  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *t = i->data;

      sqlite_exec_printf (db, "insert into todo values ('%d', '%q', '%q')", NULL, NULL, NULL,
			  id, t->tag, t->value);
    }
  
  gpe_tag_list_free (tags);

  sqlite_close (db);

  return TRUE;
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

gboolean
parse_mimedir (EventCalendar *ec, GList *callist, GError **error)
{
  char *err_str = NULL;

  void add_message (GError *error)
    {
      gchar *tmp;
      tmp = g_strdup_printf ("%s%s%s",
			     err_str ?: "", err_str ? "\n" : "",
			     error->message);
      g_free (err_str);
      err_str = tmp;
      g_error_free (error);
    }

  GList *l;
  for (l = callist; l; l = l->next)
    if (l->data && MIMEDIR_IS_VCAL (l->data)) 
      {
	MIMEDirVCal *vcal = l->data;

	GSList *list = mimedir_vcal_get_event_list (vcal);
	GSList *iter;
	for (iter = list; iter; iter = iter->next)
	  {
	    MIMEDirVEvent *vevent = MIMEDIR_VEVENT (iter->data);

	    /* We collapse error messages into a single error message
	       as it can't be handled programatically anyway.  */
	    GError *import_error = NULL;
	    if (! import_vevent (ec, vevent, NULL, &import_error))
	      add_message (import_error);
	  }
	g_slist_free (list);

	list = mimedir_vcal_get_todo_list (vcal);
	for (iter = list; iter; iter = iter->next)
	  {
	    MIMEDirVTodo *vtodo = MIMEDIR_VTODO (iter->data);

	    GError *import_error = NULL;
	    if (! do_import_vtodo (vtodo, &import_error))
	      add_message (import_error);
	  }
	g_slist_free (list);
      }

  if (err_str)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, err_str);
      g_free (err_str);
      return FALSE;
    }
  return TRUE;
}

gboolean
import_vcal_from_channel (EventCalendar *ec, GIOChannel *channel,
			  GError **error)
{
  GList *callist = mimedir_vcal_read_channel (channel, error);
  if (! callist)
    return FALSE;

  gboolean result = parse_mimedir (ec, callist, error);
  mimedir_vcal_free_list (callist);
  return result;
}

gboolean
import_vcal (EventCalendar *ec, const char *files[], GError **gerror)
{
  GtkBox *box;
  GtkWidget *combo;
  GtkWidget *filesel = NULL;

  if (! files)
    /* No files were provided, prompt for some.  */
    {
#if IS_HILDON
      filesel = hildon_file_chooser_dialog_new (GTK_WINDOW (main_window), 
                        GTK_FILE_CHOOSER_ACTION_OPEN);
      gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (filesel), TRUE);
#else
      filesel = gtk_file_selection_new (_("Choose file"));
      gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (filesel), TRUE);
#endif

      gtk_window_set_transient_for (GTK_WINDOW (filesel), 
            GTK_WINDOW (main_window));
    }

  if (! ec)
    /* We need to know into which calendar we should place the events.
       Create a widget.  */
    {
      box = GTK_BOX (gtk_hbox_new (FALSE, 3));
      gtk_widget_show (GTK_WIDGET (box));

      /* We cannot integrate the calendar selection into Hildon's file
	 chooser.  We'll prompt for the calendar afterwards with a
	 separate dialog.  */
#if ! IS_HILDON
      if (! files)
         gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (filesel)->main_vbox),
			    GTK_WIDGET (box), FALSE, FALSE, 0);
#endif

      GtkWidget *w = gtk_button_new_from_stock (GTK_STOCK_NEW);
      gtk_widget_show (w);
      gtk_box_pack_end (box, w, FALSE, FALSE, 0);

      combo = calendars_combo_box_new (event_db);
      gtk_widget_show (combo);
      gtk_box_pack_end (box, combo, FALSE, FALSE, 0);

      g_signal_connect (G_OBJECT (w), "clicked",
			G_CALLBACK (new_calendar_clicked), combo);

      w = gtk_label_new (_("Import into calendar: "));
      gtk_widget_show (GTK_WIDGET (w));
      gtk_box_pack_end (box, w, FALSE, FALSE, 0);
    }

  if (! files)
    /* Run the file chooser.  */
    {
      if (gtk_dialog_run (GTK_DIALOG (filesel)) != GTK_RESPONSE_OK)
	{
	  gtk_widget_destroy (filesel);
	  return FALSE;
	}
      gtk_widget_hide (filesel); 

#if IS_HILDON
      files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (filesel));
#else		
      files = gtk_file_selection_get_selections (GTK_FILE_SELECTION (filesel));
#endif
    }

  GtkWidget *calsel = NULL;
  if (! ec && (! filesel || 0
#ifdef IS_HILDON
	       + 1
#endif
	       ))
    /* Prompt for the calendar.  */
    {
      char *title = g_strdup_printf (_("Select Calendar for %s%s"),
				     files[0], files[1] ? "..." : "");
      calsel = gtk_dialog_new_with_buttons
	(title, GTK_WINDOW (main_window),
	 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	 0);
      g_free (title);

      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (calsel)->vbox),
			  GTK_WIDGET (box), FALSE, FALSE, 0);
      if (gtk_dialog_run (GTK_DIALOG (calsel)) != GTK_RESPONSE_ACCEPT)
        {
          gtk_widget_destroy (calsel);
          return FALSE;
        }
    }

  if (! ec)
    ec = calendars_combo_box_get_active (GTK_COMBO_BOX (combo));
  g_object_ref (ec);
  if (filesel)
    gtk_widget_destroy (filesel);
  if (calsel)
    gtk_widget_destroy (calsel);

  int errors = 0;
  gchar *errstr = NULL;
  int i;
  for (i = 0; files[i]; i ++)
    {
      GError *error = NULL;
      GList *callist = mimedir_vcal_read_file (files[i], &error);
      if (error) 
        {
          gchar *tmp;
          errors ++;
          tmp = g_strdup_printf ("%s%s%s: %s",
				 errstr ?: "", errstr ? "\n" : "",
				 files[i], error->message);
          g_free (errstr);
          errstr = tmp;
          g_error_free (error);
          continue;
        }

      if (! parse_mimedir (ec, callist, &error))
	{
	  if (! errstr)
	    errstr = g_strdup (error->message);
	  else
	    {
	      char *tmp = g_strjoin ("\n", errstr, error->message, NULL);
	      g_free (errstr);
	      errstr = tmp;
	    }

	  g_error_free (error);
	}

      /* Cleanup */
      mimedir_vcal_free_list (callist);
    }

  g_object_unref (ec);

  return !!errors;
}
