/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include <mimedir/mimedir-vcal.h>

#include <sqlite.h>
#include <gpe/vevent.h>
#include <gpe/vtodo.h>
#include <gpe/event-db.h>

#include "globals.h"

#define _(x)  (x)

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

static void
do_import_vevent (MIMEDirVEvent *event)
{
  Event *ev = NULL;
  char *uid = NULL;
  g_object_get (event, "uid", &uid, NULL);
  if (uid)
    ev = event_db_find_by_eventid (event_db, uid);
  if (! ev)
    ev = event_new (event_db, uid);

  MIMEDirDateTime *dtstart = NULL;
  g_object_get (event, "dtstart", &dtstart, NULL);
  if (! dtstart)
    {
      g_critical ("%s: Event %s has no dtstart", __func__, uid);
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

  int trigger;
  g_object_get (event, "trigger", &trigger, NULL);
  if (trigger)
    {
      gboolean trigger_end;
      int alarm;

      g_object_get (event, "trigger-end", &trigger_end, NULL);
      if (trigger_end)
	alarm = end - trigger;
      else
	alarm = start - trigger;

      if (alarm < 0)
	alarm = 1;
	
      event_set_alarm (ev, alarm);
    }

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

  char *summary = NULL;
  g_object_get (event, "summary", &summary, NULL);
  if (summary)
    event_set_summary (ev, summary);
  g_free (summary);

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

	  int unit = 0;
	  g_object_get (recurrence, "unit", &unit, NULL);
	  if (unit == RECURRENCE_WEEKLY)
	    {
	      char *units;
	      g_object_get (recurrence, "units", &units, NULL);

	      int daymask = 0;
	      char *p = units;
	      while (p && *p)
		{
		  while (*p == ' ' || *p == ',')
		    p ++;

		  int check (char *day, int shift)
		    {
		      if (strncmp (p, day, 2) == 0
			  && (p[2] == ',' || p[2] == ' ' || p[2] == 0))
			{
			  daymask |= 1 << shift;
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

	      event_set_recurrence_daymask (ev, daymask);

	      g_free (units);
	    }

	  break;
	case RECURRENCE_MONTHLY:
	  type = RECUR_MONTHLY;
	  break;
	case RECURRENCE_YEARLY:
	  type = RECUR_YEARLY;
	  break;
	default:
	  {
	    char *s = mimedir_recurrence_write_to_string (recurrence);
	    g_warning ("%s: Unhandeled recurrence type: %s", __func__, s);
	    g_free (s);
	    break;
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
  g_object_unref (ev);
}

static void
do_import_vtodo (MIMEDirVTodo *todo)
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
      gpe_error_box (err);
      free (err);
      return;
    }
 
  if (sqlite_exec (db, "insert into todo_urn values (NULL)", NULL, NULL, &err) != SQLITE_OK)
    {
      gpe_error_box (err);
      free (err);
      sqlite_close (db);
      return;
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
}

static void
do_import_vcal (MIMEDirVCal *vcal)
{
  GSList *list, *iter;
    
  list = mimedir_vcal_get_event_list (vcal);
    
  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVEvent *vevent;
      vevent = MIMEDIR_VEVENT (iter->data);
      do_import_vevent (vevent);
    }

  g_slist_free (list);

  list = mimedir_vcal_get_todo_list (vcal);

  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVTodo *vtodo;
      vtodo = MIMEDIR_VTODO (iter->data);
      do_import_vtodo (vtodo);
    }

  g_slist_free (list);
}

int
import_vcal (const gchar *filename)
{
  MIMEDirVCal *cal = NULL;
  GError *error = NULL;
  GList *callist, *l;
  int result = 0;

  callist = mimedir_vcal_read_file (filename, &error);

  if (error) 
    {
      fprintf (stderr, "import_vcal : %s\n",
	       error->message);
      g_error_free (error);
      return -1;
    }

  for (l = callist; l != NULL && result == 0; l = g_list_next (l))
    {
      if( l->data != NULL && MIMEDIR_IS_VCAL (l->data)) 
        {
           cal = l->data;
           do_import_vcal (cal);
        }
      else
        result = -3;
    }

  /* Cleanup */
  mimedir_vcal_free_list (callist);
  
  return result;	
}
