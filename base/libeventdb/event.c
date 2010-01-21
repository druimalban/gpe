/*
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006, 2008 Neal H. Walfield <neal@walfield.org>
 * Copyright (C) 2007, 2008, 2010 Graham R. Cobb <g+gpe@cobb.uk.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib-object.h>
#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "gpe/event-db.h"
#include "event.h"
#include "event-db.h"
#include "event-cal.h"

static void event_class_init (gpointer klass, gpointer klass_data);
static void event_init (GTypeInstance *instance, gpointer klass);
static void event_dispose (GObject *obj);
static void event_finalize (GObject *object);

static GObjectClass *event_parent_class;

GType
event_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventClass),
	NULL,
	NULL,
	event_class_init,
	NULL,
	NULL,
	sizeof (struct _Event),
	0,
	event_init
      };

      type = g_type_register_static (G_TYPE_OBJECT, "Event", &info, 0);
    }

  return type;
}

static void
event_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  EventClass *event_class;

  event_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = event_finalize;
  object_class->dispose = event_dispose;

  event_class = (EventClass *) klass;
}

static void
event_init (GTypeInstance *instance, gpointer klass)
{
}

static void
event_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (event_parent_class)->dispose (obj);
}

static void
event_finalize (GObject *object)
{
  Event *event = EVENT (object);

  if (event->clone_source)
    g_object_unref (event->clone_source);

  G_OBJECT_CLASS (event_parent_class)->finalize (object);
}

static void event_source_class_init (gpointer klass, gpointer klass_data);
static void event_source_init (GTypeInstance *instance, gpointer klass);
static void event_source_dispose (GObject *obj);
static void event_source_finalize (GObject *object);

static GObjectClass *event_source_parent_class;

GType
event_source_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventSourceClass),
	NULL,
	NULL,
	event_source_class_init,
	NULL,
	NULL,
	sizeof (struct _EventSource),
	0,
	event_source_init
      };

      type = g_type_register_static (TYPE_EVENT, "EventSource", &info, 0);
    }

  return type;
}

static void
event_source_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  EventSourceClass *event_source_class;

  event_source_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = event_source_finalize;
  object_class->dispose = event_source_dispose;

  event_source_class = (EventSourceClass *) klass;
}

static void
event_source_init (GTypeInstance *instance, gpointer klass)
{
  g_object_add_toggle_ref (G_OBJECT (instance),
			   event_source_toggle_ref_notify,
			   NULL);
}

static void
event_source_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (event_source_parent_class)->dispose (obj);
}

static void
event_source_finalize (GObject *object)
{
  EventSource *ev = RESOLVE_CLONE (EVENT (object));

  if (ev->modified)
    /* Flush to disk.  */
    /* XXX: Can this error be propagated better?  */
    EVENT_DB_GET_CLASS (ev->edb)->event_flush (ev, NULL);

  /* Free any details.  */
  if (ev->description)
    g_free (ev->description);
  if (ev->location)
    g_free (ev->location);
  if (ev->summary)
    g_free (ev->summary);
  g_slist_free (ev->categories);

  g_free (ev->eventid);
  g_slist_free (ev->exceptions);

  event_recurrence_byday_free (ev->byday);

  /* EV may not have been inserted into the hash if we created a
     new event and were unable to insert it into the DB.  In this
     case, EV->UID will be 0.  */
  if (ev->uid)
    {
      gboolean removed = g_hash_table_remove (ev->edb->events,
					      (gpointer) ev->uid);
  /* Unfortunately, if event_db_find_by_uid cannot find the specified UID
     it creates an event with the UID (but no data).  This means that the
     removal from the hash table can fail.  This check can be re-instated when
     the event_db_find_by_uid bug is fixed */
      // g_assert (removed);
    }

  G_OBJECT_CLASS (event_parent_class)->finalize (object);
}

gint
event_compare_func (gconstpointer a, gconstpointer b)
{
  Event *i = EVENT (a);
  Event *j = EVENT (b);

  if (i->start < j->start)
    return -1;
  if (j->start < i->start)
    return 1;

  if (event_get_duration (i) < event_get_duration (j))
    return -1;
  if (event_get_duration (j) < event_get_duration (i))
    return 1;
  return 0;
}

gint
event_alarm_compare_func (gconstpointer a, gconstpointer b)
{
  Event *i = EVENT (a);
  Event *j = EVENT (b);
  EventSource *is = RESOLVE_CLONE (i);
  EventSource *js = RESOLVE_CLONE (j);

  return (i->start - is->alarm) - (j->start - js->alarm);
}

gboolean
event_flush (Event *event, GError **error)
{
  if (event->dead)
    return TRUE;

  EventSource *ev = RESOLVE_CLONE (event);

  g_assert (!ev->edb->readonly);

  if (! EVENT_DB_GET_CLASS (ev->edb)->event_flush (ev, error))
    return FALSE;

  g_signal_emit
    (ev->edb, EVENT_DB_GET_CLASS (ev->edb)->event_modified_signal, 0, ev);

  return TRUE;
}

/* Remove any instantiations of EV's source from the upcoming alarm
   list.  */
static void
event_remove_upcoming_alarms (EventSource *ev)
{
  GSList *i;
  GSList *next = ev->edb->upcoming_alarms;
  while (next)
    {
      i = next;
      next = i->next;

      Event *e = EVENT (i->data);
      if (RESOLVE_CLONE (e) == ev)
	{
	  g_object_unref (e);
	  ev->edb->upcoming_alarms
	    = g_slist_delete_link (ev->edb->upcoming_alarms, i);
	}
    }
}

gboolean
event_remove (Event *event, GError **error)
{
  if (event->dead)
    return TRUE;

  EventSource *ev = RESOLVE_CLONE (event);

  /* Do our best to remove the event.  */
  GError *alarm_e = NULL;
  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      event_acknowledge (EVENT (ev), &alarm_e);
      /* And remove it from the upcoming alarm list.  */
      if (! alarm_e)
	event_remove_upcoming_alarms (ev);
    }

  GError *e = NULL;
  EVENT_DB_GET_CLASS (ev->edb)->event_remove (ev, &e);
  if (e)
    {
      if (alarm_e)
	g_error_free (alarm_e);
      SIGNAL_ERROR_GERROR (ev->edb, error, e);
      return FALSE;
    }

  EVENT (ev)->dead = TRUE;
  g_signal_emit (ev->edb,
		 EVENT_DB_GET_CLASS (ev->edb)->event_removed_signal,
		 0, ev);

  if (alarm_e)
    SIGNAL_ERROR_GERROR (ev->edb, error, alarm_e);

  return TRUE;
}

Event *
event_new (EventDB *edb, EventCalendar *ec, const char *eventid,
	   GError **error)
{
  EventSource *ev = EVENT_SOURCE (g_object_new (event_source_get_type (),
						NULL));

  ev->edb = edb;
  if (ec)
    ev->calendar = ec->uid;
  else
    ev->calendar = edb->default_calendar;

  if (eventid)
    {
      GError *e = NULL;
      Event *t = event_db_find_by_eventid (edb, eventid, &e);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (edb, error, e);
	  g_object_unref (ev);
	  return NULL;
	}
      if (t)
	{
	  SIGNAL_ERROR (edb, error,
			"Attempted to add event with an eventid "
			"which is already present in the database.");
	  g_object_unref (t);
	  return NULL;
	}
	  
      ev->eventid = g_strdup (eventid);
    }
  else
    {
      static int seeded;
      if (! seeded)
	{
	  srand (time (NULL));
	  seeded = 1;
	}

      static char *hostname;
      static char buffer[512];
      if (! hostname)
	{
	  if (gethostname (buffer, sizeof (buffer) - 1) == 0 && buffer[0])
	    hostname = buffer;
	  else if (errno == ENAMETOOLONG)
	    {
	      buffer[sizeof (buffer)] = 0;
	      hostname = buffer;
	    }
	  else
	    hostname = "localhost";
	}

      ev->eventid = g_strdup_printf ("%lu.%lu%d@%s",
				     (unsigned long) time (NULL),
				     (unsigned long) getpid(), rand (),
				     hostname);
    }

  if (! EVENT_DB_GET_CLASS (edb)->event_new (ev, error))
    goto error;

  g_hash_table_insert (edb->events, (gpointer) ev->uid, ev);

  g_signal_emit (edb, EVENT_DB_GET_CLASS (edb)->event_new_signal, 0, ev);

  return EVENT (ev);

 error:
  g_object_unref (ev);

  return NULL;
}

void
event_acknowledge (Event *event, GError **error)
{
  if (event->dead)
    return;

  EventSource *ev = RESOLVE_CLONE (event);
  EVENT_DB_GET_CLASS (ev->edb)->event_mark_acknowledged (ev, error);
}

/* EV is new or has recently changed: check to see if it has an alarm
   which will go off in the near future.  */
static void
event_add_upcoming_alarms (EventSource *ev, GError **error)
{
  if (! ev->edb->alarm)
    /* Alarms have not yet been activated.  */
    return;

  time_t now = time (NULL);
  GSList *list = event_list (ev, now, ev->edb->period_end, 0, TRUE, error);
  if (! list)
    return;

  ev->edb->upcoming_alarms = g_slist_concat (list, ev->edb->upcoming_alarms);

  /* We remove the timeout source and call buzzer.  Although no alarm
     has fired, it will calculate when the next timeout needs to
     fire.  */
  if (ev->edb->alarm)
    g_source_remove (ev->edb->alarm);
  buzzer (ev->edb);
}

/* Makes sure that #ev's details structure is in core.  */
static void
event_details (EventSource *ev, gboolean fill_from_disk, GError **error)
{
  if (ev->details)
    return;

  if (! fill_from_disk)
    return;

  GError *e = NULL;
  EVENT_DB_GET_CLASS (ev->edb)->event_load_details (ev, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ev->edb, error, e);
      return;
    }

  ev->details = TRUE;
}

Event *
event_duplicate (EventDB *edb, Event *src, GError **error)
{
  EventCalendar *ec = event_get_calendar (EVENT (src), error);
  if (! ec)
    return NULL;

  Event *ev = event_new (edb, ec, NULL, error);
  if (! ev)
    goto out;

  /* Now duplicate the data.  */
  ev->start = src->start;

  EventSource *srcsrc = RESOLVE_CLONE (src);
  EventSource *evsrc = RESOLVE_CLONE (ev);
  g_assert (! evsrc->details);

  event_details (srcsrc, TRUE, error);

  /* We can memcpy everything but the event id and uid, which we need
     to preserve, and the byday field and the details, which we need
     to clone.  */
  char *eventid = evsrc->eventid;
  unsigned long uid = evsrc->uid;
  int offset = offsetof (struct _EventSource, edb);
  memcpy ((void *) evsrc + offset, (void *) srcsrc + offset,
	  sizeof (*evsrc) - offset);
  evsrc->eventid = eventid;
  evsrc->uid = uid;

  evsrc->details = TRUE;
  if (srcsrc->summary)
    evsrc->summary = g_strdup (srcsrc->summary);
  if (srcsrc->description)
    evsrc->description = g_strdup (srcsrc->description);
  if (srcsrc->location)
    evsrc->location = g_strdup (srcsrc->location);

  GSList *l;
  evsrc->byday = NULL;
  for (l = srcsrc->byday; l; l = l->next)
    evsrc->byday = g_slist_append (evsrc->byday, g_strdup (l->data));

  evsrc->categories = NULL;
  for (l = srcsrc->categories; l; l = l->next)
    evsrc->categories = g_slist_append (evsrc->categories, l->data);

  if (evsrc->alarm)
    event_add_upcoming_alarms (evsrc, error);

  evsrc->modified = FALSE;
  STAMP (evsrc, error);

 out:
  g_object_unref (ec);
  return ev;
}

void
event_list_unref (GSList *l)
{
  GSList *iter;
    
  for (iter = l; iter; iter = g_slist_next (iter))
    {
      Event *ev = iter->data;
      g_object_unref (ev);
    }
	  
  g_slist_free (l);
}

/**
 * event_clone:
 * @ev: Event to clone.
 * 
 * Clones a given event for localized instantiation (e.g. clone a
 * recurring event and change the clone's start time to a particular
 * recurrence).
 *
 * Returns: The event clone.
 */
static Event *
event_clone (EventSource *ev)
{
  Event *n = EVENT (g_object_new (event_get_type (), NULL));

  n->clone_source = ev;
  g_object_ref (ev);

  return n;
}

/* Interpret T as a localtime and advance it by ADVANCE days.  */
static time_t
time_add_days (time_t t, int advance)
{
  struct tm tm;
  int days;

  localtime_r (&t, &tm);
  tm.tm_isdst = -1;
  while (advance > 0)
    {
      days = g_date_get_days_in_month (tm.tm_mon + 1, 1900 + tm.tm_year);
      if (tm.tm_mday + advance > days)
	{
	  advance -= days - tm.tm_mday + 1;
	  tm.tm_mday = 1;
	  tm.tm_mon ++;
	  if (tm.tm_mon == 12)
	    {
	      tm.tm_mon = 0;
	      tm.tm_year ++;
	    }
	}
      else
	{
	  tm.tm_mday += advance;
	  break;
	}
    }

  return mktime (&tm);
}

GSList *
event_list (EventSource *ev, time_t period_start, time_t period_end, int max,
	    gboolean per_alarm, GError **error)
{
  int event_count = 0;

  if (per_alarm && ! ev->alarm)
    /* We are looking for alarms but this event doesn't have
       one.  */
    return NULL;

  /* End of recurrence period.  */
  time_t recur_end;
  if (event_is_recurrence (EVENT (ev)))
    {
      if (! ev->end)
	/* Never ends.  */
	recur_end = 0;
      else
	recur_end = ev->end;
    }
  else
    recur_end = ev->event.start + event_get_duration (EVENT (ev));

  if (recur_end && recur_end <= period_start)
    /* Event finishes prior to PERIOD_START.  */
    return NULL;

  /* Start of first instance.  */
  time_t recur_start = ev->event.start;
      
  if (period_end && period_end < recur_start - (per_alarm ? ev->alarm : 0))
    /* Event starts after PERIOD_END.  */
    return NULL;

  if (event_is_recurrence (EVENT (ev)))
    {
      /* Cache the representation of S.  */
      struct tm orig;
      localtime_r (&recur_start, &orig);

      /* Days in the current month.  */
      int days_in_month;

      /* The effective daymask, 0 based.  */
      int daymask = 0;

      /* Build the daymask for the current month.  */
      void bydaymonthly (void)
	{
	  struct tm start;
	  localtime_r (&recur_start, &start);
	  start.tm_mday = 1;
	  start.tm_isdst = -1;
	  time_t t = mktime (&start);
	  localtime_r (&t, &start);

	  days_in_month = g_date_get_days_in_month (start.tm_mon + 1,
						    1900 + start.tm_year);
	  struct tm end = start;
	  end.tm_mday = days_in_month;
	  end.tm_isdst = -1;
	  t = mktime (&end);
	  localtime_r (&t, &end);

	  daymask = 0;
	  GSList *i;
	  for (i = ev->byday; i; i = i->next)
	    {
	      char *tail;
	      int prefix = strtol (i->data, &tail, 10);

	      while (*tail == ' ')
		tail ++;

	      int day = -1;
	      if (strcmp (tail, "SU") == 0)
		day = 0;
	      else if (strcmp (tail, "MO") == 0)
		day = 1;
	      else if (strcmp (tail, "TU") == 0)
		day = 2;
	      else if (strcmp (tail, "WE") == 0)
		day = 3;
	      else if (strcmp (tail, "TH") == 0)
		day = 4;
	      else if (strcmp (tail, "FR") == 0)
		day = 5;
	      else if (strcmp (tail, "SA") == 0)
		day = 6;

	      if (day == -1)
		continue;

	      if (prefix == 0)
		/* Every week.  */
		{
		  int d = day - start.tm_wday;
		  if (d < 0)
		    d += 7;
		  for (; d < days_in_month; d += 7)
		    daymask |= 1 << d;
		}
	      else if (prefix > 0)
		/* From start.  */
		{
		  int d = day - start.tm_wday;
		  if (d < 0)
		    d += 7;
		  d += 7 * (prefix - 1);
		  if (d < days_in_month)
		    daymask |= 1 << d;
		}
	      else
		/* From end.  */
		{
		  int d = days_in_month - 1 + day - end.tm_wday;
		  if (d >= days_in_month)
		    d -= 7;
		  d += 7 * (prefix + 1);
		  if (d >= 0)
		    daymask |= 1 << d;
		}
	    }
	}

      int increment = ev->increment > 0 ? ev->increment : 1;

      if (ev->type == RECUR_MONTHLY && ev->byday)
	{
	  bydaymonthly ();

	  struct tm tm = orig;
	  if (daymask && ! (daymask & (1 << (orig.tm_mday - 1))))
	    /* The start date is not included in the mask.  */
	    {
	      int s;
	      if (~((1 << (orig.tm_mday - 1 + 1)) - 1) & daymask)
		/* There is a day in this month following the
		   start.  */
		s = orig.tm_mday - 1 + 1;
	      else
		/* Advance to the first day of the next month.  */
		{
		  tm.tm_mday = 1;
		  recur_start = mktime (&tm);

		  int i;
		  for (i = increment; i > 0; i --)
		    {
		      int d = g_date_get_days_in_month (tm.tm_mon + 1,
							1900 + tm.tm_year);
		      recur_start = time_add_days (recur_start, d);
		      localtime_r (&recur_start, &tm);
		    }
		  bydaymonthly ();
		  s = 0;
		}

	      int j;
	      for (j = s; j < days_in_month; j ++)
		if ((1 << j) & daymask)
		  break;
	      g_assert (j != days_in_month);

	      tm.tm_mday = j + 1;
	      recur_start = mktime (&tm);
	    }
	}
      else if (ev->type == RECUR_WEEKLY && ev->byday)
	/* This is a weekly recurrence with a byday field: find the
	   first day which, starting with RECUR_START, occurs in
	   DAYMASK.  */
	{
	  GSList *l;
	  for (l = ev->byday; l; l = l->next)
	    if (strcmp (l->data, "SU") == 0)
	      daymask |= 1 << 0;
	    else if (strcmp (l->data, "MO") == 0)
	      daymask |= 1 << 1;
	    else if (strcmp (l->data, "TU") == 0)
	      daymask |= 1 << 2;
	    else if (strcmp (l->data, "WE") == 0)
	      daymask |= 1 << 3;
	    else if (strcmp (l->data, "TH") == 0)
	      daymask |= 1 << 4;
	    else if (strcmp (l->data, "FR") == 0)
	      daymask |= 1 << 5;
	    else if (strcmp (l->data, "SA") == 0)
	      daymask |= 1 << 6;

	  struct tm tm;
	  localtime_r (&recur_start, &tm);

	  int i;
	  for (i = tm.tm_wday; i < tm.tm_wday + 7; i ++)
	    if ((1 << (i % 7)) & daymask)
	      break;
	  if (i != tm.tm_wday)
	    recur_start = time_add_days (recur_start, i - tm.tm_wday);
	}

      GSList *list = NULL;
      int count;
      for (count = 0;
	   (! period_end
	    || recur_start - (per_alarm ? ev->alarm : 0) <= period_end)
	     && (! recur_end || recur_start < recur_end)
	     && (ev->count == 0 || count < ev->count);
	   count ++)
	{
	  if ((per_alarm && period_start <= recur_start - ev->alarm)
	      || (! per_alarm && period_start
		  < recur_start + event_get_duration (EVENT (ev))))
	    /* This instance occurs during the period.  Add
	       it to LIST...  */
	    {
	      GSList *i;

	      /* ... unless there happens to be an exception.  */
	      for (i = ev->exceptions; i; i = g_slist_next (i))
		if ((long) i->data == recur_start)
		  break;

	      if (! i)
		/* No exception found: instantiate this recurrence
		   and add it to LIST.  */
		{
		  Event *clone = event_clone (ev);
		  clone->start = recur_start;
		  list = g_slist_prepend (list, clone);

		  event_count ++;
		  if (event_count == max)
		    break;
		}
	    }

	  /* Advance to the next recurrence.  */
	  switch (ev->type)
	    {
	    case RECUR_DAILY:
	      /* Advance S by INCREMENT days.  */
	      recur_start = time_add_days (recur_start, increment);
	      break;

	    case RECUR_WEEKLY:
	      if (! daymask)
		/* Empty day mask, simply advance S by
		   INCREMENT weeks.  */
		recur_start = time_add_days (recur_start, 7 * increment);
	      else
		{
		  struct tm tm;
		  localtime_r (&recur_start, &tm);
		  int i;
		  for (i = tm.tm_wday + 1; i < tm.tm_wday + 1 + 7; i ++)
		    {
		      if ((i % 7) == orig.tm_wday)
			/* We wrapped a week: increment by
			   INCREMENT - 1 weeks as well.  */
			recur_start
			  = time_add_days (recur_start,
					   7 * (increment - 1));
		      if ((1 << (i % 7)) & daymask)
			{
			  recur_start
			    = time_add_days (recur_start, i - tm.tm_wday);
			  break;
			}
		    }
		}
	      break;

	    case RECUR_MONTHLY:
	      {
		int s = 0;

		struct tm tm;
		localtime_r (&recur_start, &tm);

		if (daymask
		    /* Is there another day in this month?  */
		    && (~((1 << (tm.tm_mday - 1 + 1)) - 1) & daymask))
		  s = tm.tm_mday - 1 + 1;

		if (! s)
		  /* Advance by increment months.  */
		  {
		    if (daymask)
		      /* Start at the beginning of the month.  */
		      {
			tm.tm_mday = 1;
			recur_start = mktime (&tm);
		      }

		    int i;
		    for (i = increment; i > 0; i --)
		      {
			int d = g_date_get_days_in_month (tm.tm_mon + 1,
							  1900 + tm.tm_year);
			recur_start = time_add_days (recur_start, d);
			localtime_r (&recur_start, &tm);
		      }

		    if (daymask)
		      /* Recalculate the mask.  */
		      bydaymonthly ();
		  }

		if (daymask)
		  /* Apply the mask.  */
		  {
		    int j;
		    for (j = s; j < days_in_month; j ++)
		      if ((1 << j) & daymask)
			break;
		    g_assert (j != days_in_month);

		    tm.tm_mday = j + 1;
		    recur_start = mktime (&tm);
		  }

		break;
	      }

	    case RECUR_YEARLY:
	      {
		struct tm tm;
		localtime_r (&recur_start, &tm);
		tm.tm_year += increment;
		if (tm.tm_mon == 1 && tm.tm_mday == 29
		    && g_date_get_days_in_month (tm.tm_mon + 1,
						 tm.tm_year + 1900) == 28)
		  /* XXX: If the recurrence is Feb 29th and there
		     is no Feb 29th this year then we simply clamp
		     to the 28th.  */
		  tm.tm_mday = 28;
		else
		  {
		    if (tm.tm_mon == 1 && tm.tm_mday == 28)
		      /* This recurrence is Feb 28th.  Are we
			 supposed to recur on the 29th?  */
		      {
			if (orig.tm_mday == 29
			    && g_date_get_days_in_month
			        (tm.tm_mon + 1, tm.tm_year + 1900) == 29)
			  /* Yes, and moreover, this year, Feb has
			     a 29th.  */
			  tm.tm_mday = 29;
		      }
		  }
		
		recur_start = mktime (&tm);
		break;
	      }

	    default:
	      g_critical ("Event %s has an invalid recurrence type: %d\n",
			  ev->eventid, ev->type);
	      return NULL;
	    }
	}
      return list;
    }
  else
    /* Not a recurrence.  */
    {
      if (! per_alarm
	  || (per_alarm
	      && period_start <= recur_start - ev->alarm
	      && (! period_end || recur_start - ev->alarm <= period_end)))
	{
	  g_object_ref (ev);
	  return g_slist_append (NULL, ev);
	}

      return NULL;
    }
}

EventDB *
event_get_event_db (Event *e)
{
  EventSource *ev = RESOLVE_CLONE (e);

  return ev->edb;
}

EventCalendar *
event_get_calendar (Event *event, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GError *e = NULL;
  EventCalendar *ec
    = event_db_find_calendar_by_uid (ev->edb, ev->calendar, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ev->edb, error, e);
      return NULL;
    }
  if (! ec)
    {
      g_warning ("%s: Encountered orphaned event %s (%ld):"
		 " being adopted by default calendar",
		 __func__, ev->summary ?: "", ev->uid);
      
      ec = event_db_get_default_calendar (ev->edb, NULL, &e);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (ev->edb, error, e);
	  return NULL;
	}

      event_set_calendar (EVENT (ev), ec, &e);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (ev->edb, error, e);
	  g_object_unref (ec);
	  return NULL;
	}

      return ec;
    }
  else
    {
      g_object_ref (ec);
      return ec;
    }
}

void
event_set_calendar (Event *event, EventCalendar *ec, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);
  if (! ec)
    {
      GError *e = NULL;
      ec = event_db_get_default_calendar (ev->edb, NULL, &e);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (ev->edb, error, e);
	  return;
	}
    }

  if (ev->calendar == ec->uid)
    return;

  ev->calendar = ec->uid;
  STAMP (ev, error); 
}

gboolean
event_get_color (Event *event, struct _GdkColor *color, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GError *e = NULL;
  EventCalendar *ec = event_get_calendar (event, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ev->edb, error, e);
      return FALSE;
    }

  do
    {
      int ret = event_calendar_get_color (ec, color, &e);
      g_object_unref (ec);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (ev->edb, error, e);
	  return FALSE;
	}
      if (ret)
	return ret;

      ec = event_calendar_get_parent (ec, &e);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (ev->edb, error, e);
	  return FALSE;
	}
    }
  while (ec);

  return FALSE;
}

gboolean
event_get_visible (Event *event, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GError *e = NULL;
  EventCalendar *ec = event_get_calendar (event, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ev->edb, error, e);
      /* The better default is to show the event on error.  */
      return TRUE;
    }

  do
    {
      int v = event_calendar_get_visible (ec, &e);
      g_object_unref (ec);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (ev->edb, error, e);
	  return TRUE;
	}
      if (! v)
	return FALSE;

      ec = event_calendar_get_parent (ec, &e);
      if (e)
	{
	  SIGNAL_ERROR_GERROR (ev->edb, error, e);
	  return TRUE;
	}
    }
  while (ec);

  return TRUE;
}


time_t
event_get_start (Event *ev)
{
  /* This is local, don't resolve the clone.  */
  return ev->start;
}

unsigned long
event_get_duration (Event *event)
{
  EventSource *ev = RESOLVE_CLONE (event);

  if (ev->untimed && ev->duration == 0)
    /* This is a special case: its an all day event.  */
    return 24 * 60 * 60;
  else
    return ev->duration;
}

void
event_set_duration (Event *event, unsigned long duration, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);
  if (ev->duration == duration)
    return;

  ev->duration = duration;
  STAMP (ev, error);
}

#define GET(type, name, field) \
  type \
  event_get_##name (Event *event) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    return ev->field; \
  }

#define GET_ERROR(type, name, field, detail) \
  type \
  event_get_##name (Event *event, GError **error) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    if (detail) \
      { \
        GError *e = NULL; \
        event_details (ev, TRUE, &e); \
        if (e) \
          { \
            SIGNAL_ERROR_GERROR (ev->edb, error, e); \
            return 0; \
          } \
      } \
    return ev->field; \
  }

#define SET(type, name, field, alarm_hazard, detail) \
  void \
  event_set_##name (Event *event, type value, GError **error) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    if (detail) \
      { \
        GError *e = NULL; \
        event_details (ev, TRUE, &e); \
        if (e) \
          { \
            SIGNAL_ERROR_GERROR (ev->edb, error, e); \
            return; \
          } \
      } \
    \
    if (ev->field == value) \
      return; \
    \
    if ((alarm_hazard) && ev->alarm) \
      { \
        GError *e = NULL; \
        event_acknowledge (EVENT (ev), &e); \
        if (e) \
          { \
            SIGNAL_ERROR_GERROR (ev->edb, error, e); \
            return; \
          } \
        event_remove_upcoming_alarms (ev); \
      } \
    ev->field = value; \
    \
    GError *e = NULL; \
    if ((alarm_hazard) && ev->alarm) \
      event_add_upcoming_alarms (ev, &e); \
    \
    STAMP (ev, error); \
    if (! error && e) \
      SIGNAL_ERROR_GERROR (ev->edb, error, e); \
    else if (e) \
      g_error_free (e); \
  }

#define GET_SET(type, name, field, alarm_hazard, detail) \
  GET (type, name, field) \
  SET (type, name, field, alarm_hazard, detail)

#define GET_ERROR_SET(type, name, field, alarm_hazard, detail) \
  GET_ERROR (type, name, field, detail) \
  SET (type, name, field, alarm_hazard, detail)

GET(time_t, last_modification, last_modified)
GET_SET (unsigned long, alarm, alarm, TRUE, FALSE)
GET_ERROR_SET (guint32, sequence, sequence, FALSE, TRUE)
GET_SET (enum event_recurrence_type, recurrence_type, type, TRUE, FALSE)
GET (time_t, recurrence_start, event.start)

void
event_set_recurrence_start (Event *event, time_t start, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  if (ev->event.start == start)
    return;

  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      GError *e = NULL;
      event_acknowledge (EVENT (ev), &e);

      /* And remove it from the upcoming alarm list.  */
      event_remove_upcoming_alarms (ev);
    }
  ev->event.start = start;
  if (ev->alarm)
    /* And remove it from the upcoming alarm list.  */
    event_add_upcoming_alarms (ev, error);

  STAMP (ev, error);
}

GET_SET (time_t, recurrence_end, end, TRUE, FALSE)
GET_SET (guint32, recurrence_count, count, TRUE, FALSE)
GET_SET (guint32, recurrence_increment, increment, TRUE, FALSE)

GET_SET (gboolean, untimed, untimed, FALSE, FALSE);

GET (unsigned long, uid, uid)

char *
event_get_eventid (Event *event, GError **error) \
{
  EventSource *ev = RESOLVE_CLONE (event);
  return g_strdup (ev->eventid);
}

#define GET_SET_STRING(field) \
  char * \
  event_get_##field (Event *event, GError **error) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    GError *e = NULL; \
    event_details (ev, TRUE, &e); \
    if (e) \
      { \
        SIGNAL_ERROR_GERROR (ev->edb, error, e); \
        return NULL; \
      } \
    return g_strdup (ev->field); \
  } \
 \
  void \
  event_set_##field (Event *event, const char *field, GError **error) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    \
    GError *e = NULL; \
    event_details (ev, TRUE, &e); \
    if (e) \
      { \
        SIGNAL_ERROR_GERROR (ev->edb, error, e); \
        return; \
      } \
    \
    if ((ev->field && field \
         && strcmp (ev->field, field) == 0) \
        || (! ev->field && ! field)) \
      /* Identical.  */ \
      return; \
    \
    if (ev->field) \
      free (ev->field); \
    if (field) \
      ev->field = g_strdup (field); \
    else \
      ev->field = NULL; \
    STAMP (ev, error); \
  }

GET_SET_STRING(summary)
GET_SET_STRING(location)
GET_SET_STRING(description)

GSList *
event_get_categories (Event *event, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);
  event_details (ev, TRUE, error);
  /* If we failed to load the details, EV->CATEGORIES will be
     NULL.  */
  return g_slist_copy (ev->categories);
}

void
event_add_category (Event *event, int category, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GError *e = NULL;
  event_details (ev, TRUE, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ev->edb, error, e);
      return;
    }

  ev->categories = g_slist_prepend (ev->categories, GINT_TO_POINTER(category));

  STAMP (ev, error);
}

void
event_set_categories (Event *event, GSList *categories, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GError *e = NULL;
  event_details (ev, TRUE, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ev->edb, error, e);
      return;
    }

  g_slist_free (ev->categories);
  ev->categories = categories;

  STAMP (ev, error);
}

GSList *
event_get_recurrence_byday (Event *event, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GSList *list = NULL;
  GSList *l;
  for (l = ev->byday; l; l = l->next)
    list = g_slist_prepend (list, g_strdup (l->data));

  return list;
}

void
event_set_recurrence_byday (Event *event, GSList *list, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GSList *l;
  for (l = ev->byday; l; l = l->next)
    g_free (l->data);
  g_slist_free (ev->byday);

  ev->byday = list;

  STAMP (ev, error);
}

void
event_add_recurrence_exception (Event *event, time_t start, GError **error)
{
  EventSource *ev = RESOLVE_CLONE (event);

  ev->exceptions = g_slist_append (ev->exceptions, (void *) start);

  STAMP (ev, error);
}
