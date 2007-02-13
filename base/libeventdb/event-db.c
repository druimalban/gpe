/* event-db.c: Event DB implementation.
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib-object.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <gpe/errorbox.h>
#include <libintl.h>
#include <string.h>

#define _(x) gettext(x)

#include "gpe/event-db.h"

#include "event-db.h"
#include "event.h"
#include "event-cal.h"

static void event_db_class_init (gpointer klass, gpointer klass_data);
static void event_db_init (GTypeInstance *instance, gpointer klass);
static void event_db_dispose (GObject *obj);
static void event_db_finalize (GObject *object);

static GObjectClass *event_db_parent_class;

GType
event_db_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventDBClass),
	NULL,
	NULL,
	event_db_class_init,
	NULL,
	NULL,
	sizeof (struct _EventDB),
	0,
	event_db_init
      };

      type = g_type_register_static (G_TYPE_OBJECT, "EventDB", &info, 0);
    }

  return type;
}

static void
event_db_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  EventDBClass *edb_class;

  event_db_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = event_db_finalize;
  object_class->dispose = event_db_dispose;

  edb_class = (EventDBClass *) klass;
  edb_class->error_signal
    = g_signal_new ("error",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, error),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
  edb_class->calendar_new_signal
    = g_signal_new ("calendar-new",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, calendar_new),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
  edb_class->calendar_deleted_signal
    = g_signal_new ("calendar-deleted",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, calendar_deleted),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
  edb_class->calendar_reparented_signal
    = g_signal_new ("calendar-reparented",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, calendar_reparented),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
  edb_class->calendar_changed_signal
    = g_signal_new ("calendar-changed",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, calendar_changed),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
  edb_class->calendar_modified_signal
    = g_signal_new ("calendar-modified",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, calendar_modified),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);

  edb_class->event_new_signal
    = g_signal_new ("event-new",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, event_new_func),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);

  edb_class->event_removed_signal
    = g_signal_new ("event-removed",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, event_removed),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);

  edb_class->event_modified_signal
    = g_signal_new ("event-modified",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, event_removed),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);

  edb_class->alarm_fired_signal
    = g_signal_new ("alarm-fired",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventDBClass, alarm_fired),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
}

static void
event_db_init (GTypeInstance *instance, gpointer klass)
{
  EventDB *edb = EVENT_DB (instance);

  edb->events = g_hash_table_new (NULL, NULL);
}

static void
event_db_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (event_db_parent_class)->dispose (obj);
}

/* Timer callback.  Checks for expired events on EDB->CACHE_LIST.  */
static gboolean
flush_cache (gpointer data)
{
  EventDB *edb = EVENT_DB (data);
  GSList *list = edb->cache_list;
  edb->cache_list = NULL;

  time_t now = time (NULL);
  GSList *i;
  for (i = list; i; i = i->next)
    {
      EventSource *ev = EVENT_SOURCE (i->data);
      if (ev->dead_time + CACHE_EXPIRE <= now)
	/* No new reference in at least the last 60 seconds.  Kill
	   it.  */
	g_object_remove_toggle_ref (G_OBJECT (ev),
				    event_source_toggle_ref_notify, NULL);
      else
	edb->cache_list = g_slist_prepend (edb->cache_list, ev);
    }
  g_slist_free (list);

  if (! edb->cache_list)
    /* Nothing more to do.  */
    {
      edb->cache_buzzer = 0;
      return FALSE;
    }
  else
    /* Still some live items.  Call back in a little while.  */
    return TRUE;
}

void
event_source_toggle_ref_notify (gpointer data,
				GObject *object,
				gboolean is_last_ref)
{
  EventSource *ev = EVENT_SOURCE (object);

  if (is_last_ref)
    /* Last user reference just went away.  */
    {
      if (ev->modified)
	/* XXX: How to propagate any error correctly?  */
	event_flush (EVENT (ev), NULL);

      time_t now = time (NULL);
      ev->dead_time = now;
      ev->edb->cache_list = g_slist_prepend (ev->edb->cache_list, ev);

      if (! ev->edb->cache_buzzer)
	/* In two minutes.  */
	ev->edb->cache_buzzer = g_timeout_add (CACHE_EXPIRE / 2 * 1000,
					       flush_cache, ev->edb);
    }
  else
    /* Someone just grabbed a reference to EV.  Remove it from the
       kill list.  */
    ev->edb->cache_list = g_slist_remove (ev->edb->cache_list, ev);
}

/* This function is called when the system is idle.  It takes the
   EventSources and EventCalendars which have been added to
   EDB->LAUNDRY_LIST in add_to_laundry_pile and writes them to backing
   store and sends a modification signal for each.  */
static gboolean
do_laundry (gpointer data)
{
  EventDB *edb = EVENT_DB (data);
  GSList *l;

  /* Don't run again.  */
  if (edb->laundry_buzzer)
    /* We don't simply return FALSE to say that we won't be run again
       as event_db_finalize calls us directly.  */
    g_source_remove (edb->laundry_buzzer);
  edb->laundry_buzzer = 0;

  GSList *list = edb->laundry_list;
  edb->laundry_list = NULL;

  for (l = list; l; l = g_slist_next (l))
    {
      if (IS_EVENT_SOURCE (l->data))
	{
	  EventSource *e = EVENT_SOURCE (l->data);
	  if (e->modified)
	    /* XXX: How to propagate any error correctly?  */
	    event_flush (EVENT (e), NULL);
	  g_object_unref (e);
	}
      else
	{
	  EventCalendar *e = EVENT_CALENDAR (l->data);

	  if (e->modified)
	    {
	      g_signal_emit
		(edb, EVENT_DB_GET_CLASS (edb)->calendar_modified_signal,
		 0, e);
	      e->modified = FALSE;
	    }
	  if (e->changed)
	    {
	      /* XXX: How to propagate any error correctly?  */
	      event_calendar_flush (e, NULL);
	      g_signal_emit
		(edb, EVENT_DB_GET_CLASS (edb)->calendar_changed_signal,
		 0, e);
	      e->changed = FALSE;
	    }
	  g_object_unref (e);
	}
    }

  /* Destroy the list.  */
  g_slist_free (list);

  return FALSE;
}

void
add_to_laundry_pile (GObject *e)
{
  EventDB *edb;
  if (IS_EVENT_SOURCE (e))
    edb = EVENT_SOURCE (e)->edb;
  else
    edb = EVENT_CALENDAR (e)->edb;

  g_object_ref (e);
  edb->laundry_list = g_slist_prepend (edb->laundry_list, e);
  if (! edb->laundry_buzzer)
    edb->laundry_buzzer = g_idle_add (do_laundry, edb);
}

static void
event_db_finalize (GObject *object)
{
  EventDB *edb = EVENT_DB (object);

  /* Cancel any outstanding timeouts.  */
  if (edb->alarm)
    g_source_remove (edb->alarm);
  if (edb->laundry_buzzer)
    do_laundry (edb);
  if (edb->cache_buzzer)
    g_source_remove (edb->cache_buzzer);

  GSList *i;
  GSList *next = edb->upcoming_alarms;
  while (next)
    {
      i = next;
      next = i->next;

      Event *ev = EVENT (i->data);
      g_object_unref (ev);
    }
  g_slist_free (edb->upcoming_alarms);

  next = edb->cache_list;
  while (next)
    {
      i = next;
      next = i->next;

      EventSource *ev = EVENT_SOURCE (i->data);
      /* XXX: How to propagate any error correctly?  */
      event_flush (EVENT (ev), NULL);
      g_object_remove_toggle_ref (G_OBJECT (ev),
				  event_source_toggle_ref_notify, NULL);
      ev->edb = NULL;
    }
  g_slist_free (edb->cache_list);

  g_hash_table_destroy (edb->events);

  next = edb->calendars;
  while (next)
    {
      i = next;
      next = i->next;

      EventCalendar *ec = EVENT_CALENDAR (i->data);
      g_object_unref (ec);
    }
  g_slist_free (edb->calendars);

  G_OBJECT_CLASS (event_db_parent_class)->finalize (object);
}

static void
event_db_set_alarms_fired_through (EventDB *edb, time_t t, GError **error)
{
  GError *e = NULL;
  EVENT_DB_GET_CLASS (edb)->acknowledge_alarms_through (edb, t, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (edb, error, e);
      return;
    }

  edb->alarms_fired_through = t;
}

gboolean
buzzer (gpointer data)
{
  /* XXX: How to propagate any error correctly?  */

  EventDB *edb = EVENT_DB (data);

  time_t now = time (NULL);
  if (edb->period_end < now)
    {
      g_assert (! edb->upcoming_alarms);
#define PERIOD_LENGTH 24 * 60 * 60
      edb->period_end = now + PERIOD_LENGTH;
    }

  /* When the next alarm fires (or when we need to refresh the
     upcoming event list).  */
  time_t next_alarm = edb->period_end;

  if (! edb->upcoming_alarms)
    {
      /* Get the alarms since EDB->ALARMS_FIRED_THROUGH and
	 until EDB_PERIOD_END.  */
      GError *e = NULL;
      edb->upcoming_alarms
	= event_db_list_alarms_for_period (edb,
					   edb->alarms_fired_through + 1,
					   edb->period_end, &e);
      if (e)
	SIGNAL_ERROR_GERROR (edb, NULL, e);
      else
	{
	  edb->period_end = now + PERIOD_LENGTH;

	  /* And advance alarms_fired_through to NOW.  */
	  event_db_set_alarms_fired_through (edb, now, NULL);
	}
    }

  GSList *next = edb->upcoming_alarms;
  GSList *i;
  while (next)
    {
      i = next;
      next = i->next;

      EventSource *ev = EVENT_SOURCE (i->data);

      /* Has this event gone off?  */
      time_t start = event_get_start (EVENT (ev));
      int alarm = event_get_alarm (EVENT (ev));
      if (start - alarm <= now)
	{
	  /* Mark it as unacknowledged.  */
	  EVENT_DB_GET_CLASS (ev->edb)->event_mark_unacknowledged (i->data,
								   NULL);

	  /* And signal the user a signal.  */
	  GValue args[2];
	  GValue rv;

	  args[0].g_type = 0;
	  g_value_init (&args[0], G_TYPE_FROM_INSTANCE (G_OBJECT (edb)));
	  g_value_set_instance (&args[0], edb);
        
	  args[1].g_type = 0;
	  g_value_init (&args[1], G_TYPE_POINTER);
	  g_value_set_pointer (&args[1], ev);

	  g_signal_emitv (args,
			  EVENT_DB_GET_CLASS (edb)->alarm_fired_signal,
			  0, &rv);

	  /* Remove from the upcoming alarms list.  */
	  edb->upcoming_alarms = g_slist_delete_link (edb->upcoming_alarms, i);
	  /* And drop our reference.  */
	  g_object_unref (ev);
	}
      else
	/* No, in which case will this be the next alarm to go
	   fire?  */
	next_alarm = MIN (next_alarm, start - alarm);
    }

  edb->alarm = g_timeout_add ((next_alarm - now) * 1000, buzzer, edb);

  /* Don't trigger this timeout again.  */
  return FALSE;
}

GSList *
event_db_list_unacknowledged_alarms (EventDB *edb, GError **error)
{
  GSList *list = EVENT_DB_GET_CLASS (edb)->list_unacknowledged_alarms (edb,
								       error);
  buzzer (edb);
  return list;
}

static void
events_enumerate (EventDB *edb,
		  time_t period_start, time_t period_end,
		  gboolean alarms,
		  int (*cb) (EventSource *ev),
		  GError **error)
{
  /* Make sure any in memory changes are flushed to disk.  */
  do_laundry (edb);

  EVENT_DB_GET_CLASS (edb)->events_enumerate
    (edb, period_start, period_end, alarms, cb, error);
}

Event *
event_db_next_alarm (EventDB *edb, time_t now, GError **error)
{
  Event *next = NULL;

  GError *e = NULL;

  int callback (EventSource *ev)
    {
      GSList *list = event_list (ev, now, 0, 1, TRUE, &e);
      g_object_unref (ev);
      if (e)
	return 1;
      if (! list)
	return 0;

      Event *e = EVENT (list->data);
      g_slist_free (list);

      if (! next)
	next = e;
      else if (event_get_start (e) - event_get_alarm (e)
	       < event_get_start (next) - event_get_alarm (next))
	{
	  g_object_unref (next);
	  next = e;
	}
      else
	g_object_unref (e);

      return 0;
    }

  events_enumerate (edb, now, 0, TRUE, callback, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (edb, error, e);

      g_object_unref (next);
      return NULL;
    }

  return next;
}

/* Return the event with uid UID in event database EDB from backing
   store or NULL if none exists.  */
static EventSource *
event_load (EventDB *edb, guint uid, GError **error)
{
  EventSource *ev;

  ev = EVENT_SOURCE (g_hash_table_lookup (edb->events, (gpointer) uid));
  if (ev)
    /* Already loaded, just add a reference and return it.  */
    {
      g_object_ref (ev);
      return ev;
    }

  ev = EVENT_SOURCE (g_object_new (event_source_get_type (), NULL));
  ev->edb = edb;
  ev->uid = uid;

  GError *e = NULL;
  int exists = EVENT_DB_GET_CLASS (edb)->event_load (ev, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (edb, error, e);
      g_object_unref (ev);
      return NULL;
    }
  if (exists)
    {
      g_hash_table_insert (edb->events, (gpointer) ev->uid, ev);
      return ev;
    }
  else
    {
      g_object_unref (ev);
      return NULL;
    }
}

Event *
event_db_find_by_uid (EventDB *edb, guint uid, GError **error)
{
  EventSource *ev = event_load (edb, uid, error);
  if (! ev)
    return NULL;
  else
    return EVENT (ev);
}

Event *
event_db_find_by_eventid (EventDB *edb, const char *eventid, GError **error)
{
  g_return_val_if_fail (eventid, NULL);

  int uid = EVENT_DB_GET_CLASS (edb)->eventid_to_uid (edb, eventid, error);
  if (uid == 0)
    return NULL;

  return event_db_find_by_uid (edb, uid, error);
}

static GSList *
event_db_list_for_period_internal (EventDB *edb,
				   time_t period_start, time_t period_end,
				   gboolean only_untimed, 
				   gboolean alarms,
				   GError **error)
{
  GSList *list = NULL;
  GError *e = NULL;

  int callback (EventSource *ev)
    {
      LIVE (ev);

      if (only_untimed && ! ev->untimed)
	goto out;

      GSList *l = event_list (ev, period_start, period_end, 0, alarms, &e);
      if (e)
	g_assert (! l);
      else
	list = g_slist_concat (list, l);

    out:
      g_object_unref (ev);
      return e ? 1 : 0;
    }

  events_enumerate (edb, period_start, period_end, alarms, callback, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (edb, error, e);
      g_slist_free (list);
      return NULL;
    }

  return list;
}

GSList *
event_db_list_for_period (EventDB *edb, time_t start, time_t end,
			  GError **error)
{
  return event_db_list_for_period_internal (edb, start, end, FALSE, FALSE,
					    error);
}

GSList *
event_db_list_alarms_for_period (EventDB *edb, time_t start, time_t end,
				 GError **error)
{
  return event_db_list_for_period_internal (edb, start, end, FALSE, TRUE,
					    error);
}

GSList *
event_db_untimed_list_for_period (EventDB *edb, time_t start, time_t end,
				  GError **error)
{
  return event_db_list_for_period_internal (edb, start, end, TRUE, FALSE,
					    error);
}

EventCalendar *
event_db_find_calendar_by_uid (EventDB *edb, guint uid, GError **error)
{
  GSList *i;

  for (i = edb->calendars; i; i = i->next)
    {
      GError *e = NULL;
      if (event_calendar_get_uid (EVENT_CALENDAR (i->data), &e) == uid)
	{
	  g_object_ref (i->data);
	  return i->data;
	}
      if (e)
	{
	  SIGNAL_ERROR_GERROR (edb, error, e);
	  return NULL;
	}
    }

  return NULL;
}

EventCalendar *
event_db_find_calendar_by_name (EventDB *edb, const gchar *name,
				GError **error)
{
  g_return_val_if_fail (name, NULL);
    
  GSList *iter;
  for (iter = edb->calendars; iter; iter = iter->next)
    {
      EventCalendar *ec = iter->data;
      gboolean found = FALSE;
      gchar *calendar_name = event_calendar_get_title (ec, error);
      if (! calendar_name)
	return NULL;

      if (strcmp (calendar_name, name) == 0)
	found = TRUE;
      g_free (calendar_name);
      
      if (found)
        { 
          g_object_ref (ec);
          return ec;
        }
    }

  return NULL;
}

EventCalendar *
event_db_get_default_calendar (EventDB *edb, const char *title,
			       GError **error)
{
  GError *e = NULL;
  EventCalendar *ec
    = event_db_find_calendar_by_uid (edb, edb->default_calendar, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (edb, error, e);
      return NULL;
    }

  if (! ec)
    {
      /* There is no calendar associated with the default calendar id,
	 create it.  */
      ec = event_calendar_new_full (edb, NULL, TRUE, title ?: _("My Calendar"),
				    NULL, NULL, NULL, 0, 0, error);
      if (ec)
	event_db_set_default_calendar (edb, ec, error);
    }

  return ec;
}

void
event_db_set_default_calendar (EventDB *edb, EventCalendar *ec,
			       GError **error)
{
  if (ec->uid == edb->default_calendar)
    return;

  edb->default_calendar = ec->uid;

  EVENT_DB_GET_CLASS (ec->edb)->set_default_calendar (edb, ec, error);
}

GSList *
event_db_list_event_calendars (EventDB *edb, GError **error)
{
  GSList *l = g_slist_copy (edb->calendars);
  if (! l)
    /* Default calendar doesn't exit.  Create it.  */
    return g_slist_prepend (NULL,
			    event_db_get_default_calendar (edb, NULL, error));

  GSList *i;
  for (i = l; i; i = i->next)
    g_object_ref (i->data);

  return l;
}
