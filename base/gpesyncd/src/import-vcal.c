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

#include <gpe/errorbox.h>
#include <gpe/question.h>

#include <mimedir/mimedir-vcal.h>
#include <mimedir/mimedir-valarm.h>

#include <sqlite.h>
#include <gpe/vevent.h>
#include <gpe/vtodo.h>
#include <gpe/event-db.h>

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

extern char *
do_import_vevent (EventDB *event_db, EventCalendar *ec, MIMEDirVEvent *event, Event **new_ev)
{
  char *status = NULL;
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
      status = g_strdup_printf ("Not important malformed event %s (%s):"
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
  alarm_list = mimedir_vcomponent_get_alarm_list(MIMEDIR_VCOMPONENT (event));

  for (l=alarm_list; l; l = g_list_next(l)) {
    MIMEDirVAlarm *valarm = l->data;
    int trigger;
    int alarm;

    g_object_get (valarm, "trigger", &trigger, NULL);
    if (trigger)
      {
	gboolean trigger_end;

	g_object_get (valarm, "trigger-end", &trigger_end, NULL);
	if (trigger_end)
	  alarm = end - trigger;
	else
	  alarm = start - trigger;
	
	if (alarm < 0)
	  alarm = 1;
	
	event_set_alarm (ev, alarm);
      }

    MIMEDirDateTime *triggerdt = NULL;
    g_object_get (valarm, "trigger-datetime", &triggerdt, NULL);
    if (triggerdt)
      {
	alarm = extract_time (triggerdt);
	event_set_alarm (ev, alarm);
	g_object_unref (triggerdt);
      }
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
	      fprintf (stderr, "%s: %s\n", summary, units);
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
			    {
			    byday = g_slist_prepend (byday, g_strdup (day));
			    fprintf (stderr, "%s", day);
			    }
			  else
			    {
			      char *s = g_strdup_printf ("%d%s", prefix, day);
			      byday = g_slist_prepend (byday, s);
			      fprintf (stderr, "%s\n", s);
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
	    status = g_strdup_printf ("%s (%s) has unhandeled recurrence"
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
  if (!error && new_ev) {
    /* Return the new event to the caller */
    *new_ev = ev;
    g_object_ref(ev);
  }
  if (created && error)
    event_remove (ev);
  g_object_unref (ev);
  g_free (summary);
  return status;
}
