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

#include <mimedir/mimedir-vcal.h>
#include <mimedir/mimedir-valarm.h>
#include <mimedir/mimedir-vevent.h>

#include "priv.h"

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
event_import_from_vevent (EventCalendar *ec, MIMEDirVEvent *event,
			  Event **new_ev,
			  GError **gerror)
{
  EventDB *event_db = event_calendar_get_event_db (ec);

  gboolean error = FALSE;;
  gboolean created = FALSE;;

  Event *ev = NULL;
  char *uid = NULL;
  g_object_get (event, "uid", &uid, NULL);
  if (uid)
    {
      GError *e = NULL;
      ev = event_db_find_by_eventid (event_db, uid, &e);
      if (e)
	{
	  ERROR_PROPAGATE (gerror, e);
	  return FALSE;
	}
    }
  if (! ev)
    {
      GError *e = NULL;
      ev = event_new (event_db, ec, uid, &e);
      if (e)
	{
	  ERROR_PROPAGATE (gerror, e);
	  return FALSE;
	}

      created = TRUE;
    }
  else
    event_set_calendar (ev, ec, NULL);

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
  event_set_recurrence_start (ev, start, NULL);
  event_set_untimed (ev, (dtstart->flags & MIMEDIR_DATETIME_TIME) == 0, NULL);
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
      event_set_duration (ev, end - start, NULL);
      g_object_unref (dtend);
    }
  else
    {
      int duration;
      g_object_get (event, "duration", &duration, NULL);

      if (duration)
	{
	  event_set_duration (ev, duration, NULL);
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
	    alarm = end + trigger;
	  else
	    alarm = start + trigger;
	
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
    event_set_alarm (ev, trigger_min, NULL);

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
    event_set_summary (ev, summary, NULL);

  char *description = NULL;
  g_object_get (event, "description", &description, NULL);
  if (description)
    event_set_description (ev, description, NULL);
  g_free (description);

  const char *location_uri;
  const char *location
    = mimedir_vcomponent_get_location (MIMEDIR_VCOMPONENT (event),
				       &location_uri);
  if (location && location_uri)
    {
      char *s = NULL;
      s = g_strdup_printf ("%s\n%s", location, location_uri);
      event_set_location (ev, s, NULL);
      g_free (s);
    }
  else if (location)
    event_set_location (ev, location, NULL);
  else if (location_uri)
    event_set_location (ev, location_uri, NULL);

  int sequence;
  g_object_get (event, "sequence", &sequence, NULL);
  event_set_sequence (ev, sequence, NULL);

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
	  event_set_recurrence_end (ev, mimedir_datetime_get_time_t (until),
				    NULL);
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

	      event_set_recurrence_byday (ev, byday, NULL);

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
      event_set_recurrence_type (ev, type, NULL);

      int count = 0;
      g_object_get (recurrence, "count", &count, NULL);
      event_set_recurrence_count (ev, count, NULL);

      int interval = 0;
      g_object_get (recurrence, "interval", &interval, NULL);
      if (interval)
	event_set_recurrence_increment (ev, interval, NULL);

      if (until)
	g_object_unref (until);

      /* We don't own a reference to recurrence, don't unref it.  */
    }

 out:
  g_free (uid);

  if (! error && new_ev)
    /* Return the new event to the caller.  */
    {
      *new_ev = ev;
      g_object_ref (ev);
    }

  if (created && error)
    event_remove (ev, NULL);
  g_object_unref (ev);
  g_free (summary);

  return ! error;
}

MIMEDirVEvent *
event_export_as_vevent (Event *ev)
{
  MIMEDirVEvent *event = mimedir_vevent_new ();

  char *uid = event_get_eventid (ev, NULL);
  g_object_set (event, "uid", uid, NULL);
  g_free (uid);

  time_t s = event_get_recurrence_start (ev);
  time_t e = s + event_get_duration (ev);
  struct tm tm;

  MIMEDirDateTime *dtstart;
  MIMEDirDateTime *dtend;
  if (event_get_untimed (ev))
    {
      localtime_r (&s, &tm);
      dtstart = mimedir_datetime_new_from_date (tm.tm_year + 1900,
						tm.tm_mon + 1,
						tm.tm_mday);
      localtime_r (&e, &tm);
      dtend = mimedir_datetime_new_from_date (tm.tm_year + 1900,
					      tm.tm_mon + 1,
					      tm.tm_mday);
      mimedir_vcomponent_set_allday (MIMEDIR_VCOMPONENT (event), TRUE);
    }
  else
    {
      gmtime_r (&s, &tm);
      dtstart = mimedir_datetime_new_from_datetime (tm.tm_year + 1900,
						    tm.tm_mon + 1,
						    tm.tm_mday,
						    tm.tm_hour,
						    tm.tm_min,
						    tm.tm_sec);
      dtstart->timezone = MIMEDIR_DATETIME_UTC;

      gmtime_r (&e, &tm);
      dtend = mimedir_datetime_new_from_datetime (tm.tm_year + 1900,
						  tm.tm_mon + 1,
						  tm.tm_mday,
						  tm.tm_hour,
						  tm.tm_min,
						  tm.tm_sec);
      dtend->timezone = MIMEDIR_DATETIME_UTC;
    }

  g_object_set (event, "dtstart", dtstart, NULL);
  g_object_unref (dtstart);
  g_object_set (event, "dtend", dtend, NULL);
  g_object_unref (dtend);

  time_t last_modification = event_get_last_modification (ev);
  MIMEDirDateTime *dmod;

  dmod = mimedir_datetime_new_from_time_t (last_modification);
  g_object_set (event, "last-modified", dmod, NULL);
  g_object_unref (dmod);
	
  int alarm = event_get_alarm (ev);
  if (alarm)
    /* Add a display alarm and an audio alarm.  */
    {
      MIMEDirVAlarm *valarm;
      GList *list = NULL;

      valarm = mimedir_valarm_new (MIMEDIR_VALARM_DISPLAY);
      g_object_set (valarm, "action", "DISPLAY", NULL);
      g_object_set (valarm, "trigger", alarm, NULL);
      list = g_list_prepend (list, valarm);

      /* XXX: Adding two alarms confuses some systems, assume the
	 display alarm is enough. */
#if 0
      valarm = mimedir_valarm_new (MIMEDIR_VALARM_AUDIO);
      g_object_set (valarm, "action", "AUDIO", NULL);
      g_object_set (valarm, "trigger", alarm, NULL);
      list = g_list_prepend (list, valarm);
#endif

      /* Add the valarms to the vevent.  As there is no
	 "mimedir_vcomponent_add_alarm_list" function, we replace the
	 whole list using mimedir_vcomponent_set_alarm_list.  */
      mimedir_vcomponent_set_alarm_list (MIMEDIR_VCOMPONENT (event), list);
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

  char *summary = event_get_summary (ev, NULL);
  if (summary)
    {
      if (*summary)
	g_object_set (event, "summary", summary, NULL);
      g_free (summary);
    }

  char *description = event_get_description (ev, NULL);
  if (description)
    {
      if (*description)
	g_object_set (event, "description", description, NULL);
      g_free (description);
    }

  char *location = event_get_location (ev, NULL);
  if (location)
    {
      if (*location)
	mimedir_vcomponent_set_location (MIMEDIR_VCOMPONENT (event),
					 location, "");
      g_free (location);
    }

  if (event_get_recurrence_type (ev) != RECUR_NONE)
    {
      MIMEDirRecurrence *recurrence = mimedir_recurrence_new ();

      MIMEDirRecurrenceFrequency freq;
      switch (event_get_recurrence_type (ev))
	{
	case RECUR_DAILY:
	  freq = RECURRENCE_DAILY;
	  break;
	case RECUR_WEEKLY:
	  freq = RECURRENCE_WEEKLY;
	  break;
	case RECUR_MONTHLY:
	  freq = RECURRENCE_MONTHLY;
	  break;
	case RECUR_YEARLY:
	  freq = RECURRENCE_YEARLY;
	  break;
	default:
	  g_assert_not_reached ();
	}

      g_object_set (G_OBJECT (recurrence), "frequency", freq, NULL);

      /* XXX: Add EXDATE (exceptions) when libmimedir supports it.  */
      /* XXX: Add RDATE (BYDAY, etc.) when libmimedir supports it.  */

      time_t e = event_get_recurrence_end (ev);
      if (e)
	{
	  MIMEDirDateTime *until;
	  if (event_get_untimed (ev))
	    {
	      struct tm tm;
	      localtime_r (&e, &tm);
	      until = mimedir_datetime_new_from_date (tm.tm_year + 1900,
						      tm.tm_mon + 1,
						      tm.tm_mday);
	    }
	  else
	    {
	      struct tm tm;
	      gmtime_r (&e, &tm);
	      until = mimedir_datetime_new_from_datetime (tm.tm_year + 1900,
							  tm.tm_mon + 1,
							  tm.tm_mday,
							  tm.tm_hour,
							  tm.tm_min,
							  tm.tm_sec);
	      until->timezone = MIMEDIR_DATETIME_UTC;
	    }

	  g_object_set (recurrence, "until", until, NULL);
	  g_object_unref (until);
	}

      int count = event_get_recurrence_count (ev);
      if (count)
	g_object_set (recurrence, "count", count, NULL);

      int interval = event_get_recurrence_increment (ev);
      if (interval > 1)
	g_object_set (recurrence, "interval", interval, NULL);

      GSList *byday = event_get_recurrence_byday (ev, NULL);
      if (byday)
	{
	  char *s[g_slist_length (byday) + 1];
	  int i;
	  GSList *l;
	  for (l = byday, i = 0; l; l = l->next, i ++)
	    s[i] = l->data;
	  s[i] = NULL;

	  char *units = g_strjoinv (",", s);

	  g_object_set (recurrence, "unit", RECURRENCE_UNIT_DAY, NULL);
	  g_object_set (recurrence, "units", units, NULL);
	  event_recurrence_byday_free (byday);
	  g_free (units);
	}

      g_object_set (event, "recurrence", recurrence, NULL);
      g_object_unref (recurrence);
    }

  return event;
}

char *
event_export_as_string (Event *ev)
{
  MIMEDirVCal *vcal = mimedir_vcal_new ();

  MIMEDirVEvent *vev = event_export_as_vevent (ev);
  mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vev));
  g_object_unref (vev);

  char *s = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);

  return s;
}
