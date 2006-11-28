/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *               2005, 2006 Florian Boor <florian@kernelconcepts.de> 
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <errno.h>

#include "export-vcal.h"

#include <gpe/errorbox.h>
#include <mimedir/mimedir.h>

#ifdef USE_DBUS
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

static DBusConnection *connection;

#define BLUETOOTH_SERVICE_NAME   "org.handhelds.gpe.bluez"
#define IRDA_SERVICE_NAME   "org.handhelds.gpe.irda"
#endif /* USE_DBUS */

#ifdef IS_HILDON /* Hildon includes */
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#endif

MIMEDirVEvent *
export_event_as_vevent (Event *ev)
{
  MIMEDirVEvent *event = mimedir_vevent_new ();

  char *uid = event_get_eventid (ev);
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
  if (alarm) {
    /* We need to create valarm components. 
       We add a display alarm and an audio alarm. */
    MIMEDirVAlarm *valarm;
    GList *list=NULL;

    valarm = mimedir_valarm_new (MIMEDIR_VALARM_DISPLAY);
    g_object_set (valarm, "action", "DISPLAY", NULL);
    g_object_set (valarm, "trigger", alarm, NULL);
    list = g_list_prepend(list, valarm);

    /* Adding two alarms confuses some systems, assume the display alarm is enough */
#if 0
    valarm = mimedir_valarm_new (MIMEDIR_VALARM_AUDIO);
    g_object_set (valarm, "action", "AUDIO", NULL);
    g_object_set (valarm, "trigger", alarm, NULL);
    list = g_list_prepend(list, valarm);
#endif

    /* Now we add the valarms to the vevent.
       Note that this as there is no "mimedir_vcomponent_add_alarm_list" function
       we replace the whole list using mimedir_vcomponent_set_alarm_list. */
    mimedir_vcomponent_set_alarm_list (event, list);
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

  char *summary = event_get_summary (ev);
  if (summary)
    {
      if (*summary)
	g_object_set (event, "summary", summary, NULL);
      g_free (summary);
    }

  char *description = event_get_description (ev);
  if (description)
    {
      if (*description)
	g_object_set (event, "description", description, NULL);
      g_free (description);
    }

  char *location = event_get_location (ev);
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

      GSList *byday = event_get_recurrence_byday (ev);
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
