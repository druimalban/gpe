/* event-db.c: Event DB implementation.
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib-object.h>
#include <glib.h>
#include <sqlite.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <gpe/errorbox.h>
#include <obstack.h>
#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#include <libintl.h>
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
		    G_STRUCT_OFFSET (EventDBClass, event_new),
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
	event_flush (EVENT (ev));

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
	    event_flush (EVENT (e));
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
	      event_calendar_flush (e);
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
      event_flush (EVENT (ev));
      g_object_remove_toggle_ref (G_OBJECT (ev),
				  event_source_toggle_ref_notify, edb);
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

  sqlite_close (edb->sqliteh);

  G_OBJECT_CLASS (event_db_parent_class)->finalize (object);
}

static void
event_db_set_alarms_fired_through (EventDB *edb, time_t t)
{
  int err;
  char *str;

  err = SQLITE_TRY
    (sqlite_exec_printf
     (edb->sqliteh,
      "insert or replace into alarms_fired_through values (%d);",
      NULL, NULL, &str, t));
  if (err)
    {
      g_critical ("%s: %s", __func__, str);
      g_free (str);
    }

  edb->alarms_fired_through = t;
}

/* Mark EV's alarm as having fired but yet not been acknowledged.  */
static void
event_mark_unacknowledged (EventSource *ev)
{
  int err;
  char *str;

  err = SQLITE_TRY (sqlite_exec_printf (ev->edb->sqliteh,
					"insert into alarms_unacknowledged"
					" (uid, start) values (%d, %d)",
					NULL, NULL, &str,
					ev->uid, ev->event.start));
  if (err)
    {
      g_critical ("%s: %s", __func__, str);
      g_free (str);
    }
}

gboolean
buzzer (gpointer data)
{
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
      edb->upcoming_alarms
	= event_db_list_alarms_for_period (edb,
					   edb->alarms_fired_through + 1,
					   edb->period_end);
      edb->period_end = now + PERIOD_LENGTH;

      /* And advance alarms_fired_through to NOW.  */
      event_db_set_alarms_fired_through (edb, now);
    }

  GSList *next = edb->upcoming_alarms;
  GSList *i;
  while (next)
    {
      i = next;
      next = i->next;

      Event *ev = EVENT (i->data);

      /* Has this event gone off?  */
      if (event_get_start (ev) - event_get_alarm (ev) <= now)
	{
	  /* Mark it as unacknowledged.  */
	  event_mark_unacknowledged (i->data);

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
	next_alarm = MIN (next_alarm,
			  event_get_start (ev) - event_get_alarm (ev));
    }

  edb->alarm = g_timeout_add ((next_alarm - now) * 1000, buzzer, edb);

  /* Don't trigger this timeout again.  */
  return FALSE;
}

GSList *
event_db_list_unacknowledged_alarms (EventDB *edb)
{
  GSList *list = NULL;

  /* We can't remove stale rows in the callback as the database is
     locked.  We collect them here and then iterate over this list
     later.  */
  struct removal
  {
    unsigned int uid;
    time_t start;
  };
  GSList *removals = NULL;

  int callback (void *arg, int argc, char **argv, char **names)
    {
      if (argc != 2)
	{
	  g_warning ("%s: expected 2 arguments, got %d", __func__, argc);
	  return 0;
	}

      unsigned int uid = atoi (argv[0]);
      time_t t = atoi (argv[1]);

      EventSource *ev = EVENT_SOURCE (event_db_find_by_uid (edb, uid));
      if (! ev)
	{
	  g_warning ("%s: event %s not found", __func__, argv[0]);
	  goto remove;
	}

      if (t == 0)
	{
	  g_warning ("%s: unacknowledged event %s has 0 start time (%s)!",
		     __func__, argv[0], argv[1]);
	  goto remove;
	}

      GSList *l = event_list (ev, t, t, 0, FALSE);
      if (! l)
	{
	  g_warning ("%s: no instance of event %s at %s",
		     __func__, argv[0], argv[1]);
	  goto remove;
	}

      if (l->next)
	g_warning ("%s: multiple instantiations of event %s!",
		   __func__, argv[0]);

      list = g_slist_concat (list, l);

      return 0;

    remove:
      {
	struct removal *r = g_malloc (sizeof (struct removal));
	r->uid = uid;
	r->start = t;
	removals = g_slist_prepend (removals, r);

	return 0;
      }
    }

  char *err;
  if (SQLITE_TRY
      (sqlite_exec (edb->sqliteh,
		    "select uid, start from alarms_unacknowledged",
		    callback, NULL, &err)))
    {
      g_warning ("%s: %s", __func__, err);
      g_free (err);
    }

  /* Kill any stale entries.  */
  GSList *i;
  for (i = removals; i; i = g_slist_next (i))
    {
      struct removal *r = i->data;
      char *err;
      if (SQLITE_TRY
	  (sqlite_exec_printf (edb->sqliteh,
			       "delete from alarms_unacknowledged"
			       " where uid=%d and start=%d",
			       NULL, NULL, &err, r->uid, r->start)))
	{
	  g_warning ("%s: while removing stale entry uid=%d,start=%ld, %s",
		     __func__, r->uid, r->start, err);
	  g_free (err);
	}
      g_free (r);
    }
  g_slist_free (removals);

  buzzer (edb);

  return list;
}

/* Enumerates the events in the event database EDB which MAY occur
   between (PERIOD_START and PERIOD_END].  If ALARMS is true, returns
   those events which have an alarm which MAY go off between
   PERIOD_START and PERIOD_END.  Calls CB on each event until CB
   returns a non-zero value.  A reference to EV is allocate and the
   callback function must consume it.  If an SQLITE error occurs, a
   non-zero result is returned.  If ERR is not NULL, a string
   describing the error is returned in *ERR.  As normal, it must be
   freed by the caller.  */
static int
events_enumerate (EventDB *edb,
		  time_t period_start, time_t period_end,
		  gboolean alarms,
		  int (*cb) (EventSource *ev),
		  char **err)
{
  /* Make sure any in memory changes are flushed to disk.  */
  do_laundry (edb);

  struct obstack query;
  obstack_init (&query);

#define obstack_grow_string(o, string) \
  obstack_grow (o, string, strlen (string))

  obstack_grow_string
    (&query,
     "select * from"
     " (select *,"
     /* If the event is untimed (i.e. if there is a time
	component).  */
     "   (case substr (start, 12, 5)"
     "      when '' then"
     "        '0 seconds'"
     "      else"
     "        'localtime'"
     "    end)"
     "   as LOCALTIME,");

  /* The start of the event: only required if there is end.  */
  if (period_end)
    {
      if (alarms)
	obstack_grow_string
	  (&query,
	   " julianday (start, '-' || alarm || ' seconds')");
      else
	obstack_grow_string
	  (&query,
	   " julianday (start)");
      obstack_grow_string
	(&query,
	 " as EVENT_START,");
    }

  /* The end of the event: always required as we always have a
     start.  */
  obstack_grow_string
    (&query,
     "  (case"
     "     when recur == 0 then");
  if (alarms)
    obstack_grow_string
      (&query,
       /* We are looking for alarms on a single shot event.  */
       "       julianday (start, '-' || alarm || 'seconds', '1 second')");
  else
    obstack_grow_string
      (&query,
       "       julianday (start,"
       /* For historical reasons, an untimed event which is 0 seconds
	  long is consider 24 hours long.  */
       "                  (case duration"
       "                    when 0 then"
       "                      24 * 60 * 60"
       "                    else"
       "                      duration"
       "                    end)"
       "                   || ' seconds')");

  obstack_grow_string
    (&query,
     "     else"
     "       julianday (rend)"
     "    end)"
     "   as EVENT_END");

  obstack_grow_string
    (&query,
     "   from events"
     /* Does PERIOD_START occur before the end of the event?  */
     "   where (rend == 0 or rend ISNULL"
     "          or julianday (");

  char buffer[20];
  sprintf (buffer, "%ld", period_start);
  obstack_grow_string (&query, buffer);

  obstack_grow_string
    (&query,
     ", 'unixepoch', 'localtime')"
     "             < julianday (EVENT_END, LOCALTIME))");

  if (period_end)
    {
      /* Does the event start before or at PERIOD_END?  */
      obstack_grow_string
	(&query,
	 "     and julianday (EVENT_START, LOCALTIME)"
	 "         <= julianday (");

      sprintf (buffer, "%ld", period_end);
      obstack_grow_string (&query, buffer);

      obstack_grow_string
	(&query,
	 ", 'unixepoch', 'localtime')");
    }

  if (alarms)
    obstack_grow_string
      (&query,
       "     and alarm > 0");

  obstack_grow_string
    (&query,
     ");");
  /* Add a trailing NULL.  */
  obstack_1grow (&query, 0);
  char *q = obstack_finish (&query);

  int callback (void *arg, int argc, char **argv, char **names)
    {
      EventSource *ev;

      int uid = atoi (argv[0]);

      ev = EVENT_SOURCE (g_hash_table_lookup (edb->events, (gpointer) uid));
      if (ev)
	/* Already loaded, just add a reference and return it.  */
	g_object_ref (ev);
      else
	{
	  ev = EVENT_SOURCE (g_object_new (event_source_get_type (), NULL));
	  ev->edb = edb;
	  ev->uid = uid;
	  g_hash_table_insert (edb->events, (gpointer) ev->uid, ev);

	  event_load_callback (ev, argc, argv, names);
	}

      return cb (ev);
    }

  int ret = SQLITE_TRY (sqlite_exec_printf (edb->sqliteh, q,
					    callback, NULL, err));
  obstack_free (&query, NULL);
  return ret;
}

Event *
event_db_next_alarm (EventDB *edb, time_t now)
{
  Event *next = NULL;

  int callback (EventSource *ev)
    {
      GSList *list = event_list (ev, now, 0, 1, TRUE);
      g_object_unref (ev);
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

  char *err;
  if (events_enumerate (edb, now, 0, TRUE, callback, &err))
    {
      g_critical ("%s: %s", __func__, err);
      g_free (err);
    }

  return next;
}

EventDB *
event_db_new (const char *fname)
{
  EventDB *edb = EVENT_DB (g_object_new (event_db_get_type (), NULL));
  char *err;

  edb->sqliteh = sqlite_open (fname, 0, &err);
  if (edb->sqliteh == NULL)
    goto error_before_transaction;

  if (SQLITE_TRY
      (sqlite_exec (edb->sqliteh, "begin transaction;", NULL, NULL, &err)))
    goto error_before_transaction;

  /* Get the calendar db version.  */
  sqlite_exec (edb->sqliteh,
	       "create table calendar_dbinfo (version integer NOT NULL)",
	       NULL, NULL, &err);
  int version = -1;
  int dbinfo_callback (void *arg, int argc, char **argv, char **names)
    {
      if (argc == 1)
	version = atoi (argv[0]);

      return 0;
    }
  /* If the calendar_dbinfo table doesn't exist then we understand
     this to mean that this DB is uninitialized.  */
  if (sqlite_exec (edb->sqliteh, "select version from calendar_dbinfo",
		   dbinfo_callback, NULL, &err))
    goto error;

  if (version == 2)
    /* Databases with this version come from a relatively widely
       distributed pre-release of libeventdb with a bug such that the
       calendar table accumulates lots of duplicate entries because we
       kept appending modifications instead of replacing the records.
       Clean this up.  */
    {
      if (sqlite_exec
	  (edb->sqliteh,
	   "create temp table foo as"
	   "  select * from calendar"
	   "    where _ROWID_ in"
	   "      (select max(_ROWID_) from calendar"
	   "         where tag not in ('rexceptions', 'byday', 'category')"
	   "         group by uid, tag)"
	   "  union"
	   "    select DISTINCT * from calendar"
	   "      where tag in ('rexceptions', 'byday', 'category');"
	   "drop table calendar;"
	   "create table calendar as select * from foo;"
	   "drop table foo;",
	   NULL, NULL, &err))
	goto error;
    }

  if (version > 4)
    {
      err = g_strdup_printf
	(_("Unable to read database file: unknown version: %d"),
	 version);
      goto error;
    }

  if (version < 1)
    /* Create calendar table.  This name is actually a misnomer: it
       actually contains the ancillary data which accompanies
       events.  We use the name for historical reasons.  */
    sqlite_exec (edb->sqliteh,
		 "create table calendar"
		 " (uid INTEGER not NULL, tag STRING, value STRING);",
		 NULL, NULL, NULL);

  if (version == 0)
    /* Convert a version 0 DB to a verion 1 DB.  */
    {
      if (sqlite_exec
	  (edb->sqliteh,
	   /* Create calendar_urn.  */
	   "create table calendar_urn (uid INTEGER PRIMARY KEY);"
	   /* Populate it.  */
	   "insert into calendar_urn select _ROWID_ from events;"
	   /* And populate it.  */
	   "insert into calendar "
	   " select _ROWID_, 'start' string, start from events"
	   " union select _ROWID_, 'duration' string, duration from events"
	   " union select _ROWID_, 'alarm' string, alarmtime from events"
	   " union select _ROWID_, 'summary' string, summary from events"
	   " union select _ROWID_, 'description' string, description"
	   "  from events;"
	   "drop table events;"
	   "delete from calendar_dbinfo;"
	   "insert into calendar_dbinfo (version) values (1);",
	   NULL, NULL, &err))
	{
	  char *s = g_strdup_printf
	    ("%s: Converting a version 0 to a version 1 db: %s\n",
	     __func__, err);
	  g_free (err);
	  err = s;
	  goto error;
	}
      else
	version = 1;
    }

  /* Add an SQL convenience aggregate function, cat, which assembles a
     comma separated list of values.  */
  void cat_step (sqlite_func *context, int argc, const char **argv)
    {
      if (! argv[0])
	/* Ignore NULL values.  */
	return;

      char **s;
      s = sqlite_aggregate_context (context, sizeof (*s));

      if (*s)
	{
	  char *t;
	  t = g_strdup_printf ("%s,%s", *s, argv[0]);
	  g_free (*s);
	  *s = t;
	}
      else
	*s = g_strdup (argv[0]);
    }
  void cat_finalize (sqlite_func *context)
    {
      char **s = sqlite_aggregate_context (context, sizeof (*s));
      if (*s)
	{
	  printf ("Returning: %s\n", *s);
	  sqlite_set_result_string (context, *s, -1);
	  g_free (*s);
	}
    }

  sqlite_create_aggregate (edb->sqliteh, "cat", 1,
			   cat_step, cat_finalize, NULL);

#define SELECT_FIELD(field) \
  "(select uid, value from calendar where tag='" field "')"

  if (version == 1)
    /* Version 1 versions of the database stored some information in
       the calendar table.  We've since rearranged this.  */
    {
      if (sqlite_exec
	  (edb->sqliteh,
	   "create temp table events as select * from "
	   " ((((" SELECT_FIELD ("start")
	   "     left join " SELECT_FIELD ("duration") " using (uid))"
	   "    left join " SELECT_FIELD ("recur") " using (uid))"
	   "   left join " SELECT_FIELD ("rend") " using (uid))"
	   "  left join " SELECT_FIELD ("alarm") " using (uid))"
	   " left join " SELECT_FIELD ("calendar") " using (uid);"
	   "delete from calendar where tag='start'"
	   " or tag='duration' or tag='recur' or tag='rend'"
	   " or tag='alarm' or tag='calendar';"
	   "drop table calendar_urn;",
	   NULL, NULL, &err))
	{
	  char *s = g_strdup_printf
	    ("%s: Converting a version 1 to a version 2 db: %s\n",
	     __func__, err);
	  g_free (err);
	  err = s;
	  goto error;
	}

      /* Convert the "rdaymask" bitmask to a "byday" list.  */
      struct info
      {
	guint uid;
	char *s;
      };
      GSList *list = NULL;

      char *days[] = { "MO", "TU", "WE", "TH", "FR", "SA", "SU" };

      int callback (void *arg, int argc, char **argv, char **names)
	{
	  if (! argv[1])
	    return 0;

	  int daymask = atoi (argv[1]);
	  if (! daymask)
	    return 0;

	  struct info *info = g_malloc0 (sizeof (*info));
	  list = g_slist_prepend (list, info);
	  info->uid = atoi (argv[0]);

	  int i;
	  for (i = 0; i < 7; i ++)
	    if ((1 << i) & daymask)
	      {
		if (info->s)
		  {
		    char *t = g_strdup_printf ("%s,%s", info->s, days[i]);
		    g_free (info->s);
		    info->s = t;
		  }
		else
		  info->s = g_strdup (days[i]);
	      }

	  return 0;
	}
      sqlite_exec (edb->sqliteh,
		   "select uid, value from calendar where tag='rdaymask';"
		   "delete from calendar where tag='rdaymask'", 
		   callback, NULL, NULL);

      GSList *i;
      for (i = list; i; i = i->next)
	{
	  struct info *info = i->data;

	  sqlite_exec_printf
	    (edb->sqliteh,
	     "update events set byday='%q' where uid='%d';",
	     NULL, NULL, NULL, info->s, info->uid);

	  g_free (info->s);
	  g_free (info);
	}
      g_slist_free (list);
    }
  if (version == 1 || version == 2)
    /* In the version 3 format, we have moved even more data from the
       calendar table to the event table.  */
    {
      if (sqlite_exec
	  (edb->sqliteh,
	   "create temp table foo as select * from"
	   " (((((((select * from events)"
	   "       left join " SELECT_FIELD ("eventid") " using (uid))"
	   "      left join " SELECT_FIELD ("rcount") " using (uid))"
	   "     left join " SELECT_FIELD ("rincrement") " using (uid))"
	   "    left join " SELECT_FIELD ("modified") " using (uid))"
	   "   left join (select uid, cat(value) from calendar"
	   "              where tag='byday' group by uid, tag) using (uid))"
	   "  left join (select uid, cat(value) from calendar"
	   "             where tag='rexceptions' group by uid, tag)"
	   "  using (uid));"
	   "drop table events;"
	   "delete from calendar"
	   "  where tag in ('eventid', 'rcount', 'rincrement', 'modified',"
	   "                'byday', 'rexceptions');",
	   NULL, NULL, &err))
	goto error;
    }

  if (version < 3)
    /* Create the events table.  */
    {
      if (sqlite_exec
	  (edb->sqliteh,
	   "create table events"
	   " (uid INTEGER PRIMARY KEY, start DATE, duration INTEGER, "
	   "  recur INTEGER, rend DATE, alarm INTEGER, calendar INTEGER,"
	   "  eventid STRING, rcount INTEGER, rincrement INTEGER,"
	   "  modified DATE, byday STRING, rexceptions STRING);",
	   NULL, NULL, &err))
	goto error;

      sqlite_exec (edb->sqliteh,
		   "create index events_enumerate_index"
		   " on events (start, duration, rend, alarm);",
		   NULL, NULL, &err);
    }

  if (version == 1 || version == 2)
    {
      if (sqlite_exec
	  (edb->sqliteh,
	   "insert into events select * from foo;"
	   "drop table foo;",
	   NULL, NULL, &err))
	goto error;
    }

  /* Read EDB->ALARMS_FIRED_THROUGH.  */
  edb->alarms_fired_through = time (NULL);
  if (version < 2)
    sqlite_exec (edb->sqliteh,
		 "create table alarms_fired_through (time INTEGER)",
		 NULL, NULL, NULL);

  int alarms_fired_through_callback (void *arg, int argc, char **argv,
				     char **names)
    {
      EventDB *edb = EVENT_DB (arg);
      if (argc == 1)
	{
	  int t = atoi (argv[0]);
	  if (t > 0)
	    edb->alarms_fired_through = t;
	}

      return 0;
    }
  if (sqlite_exec (edb->sqliteh, "select time from alarms_fired_through",
		   alarms_fired_through_callback, edb, &err))
    goto error;

  /* Unacknowledged alarms.  */

  /* A table of events whose alarm fired before
     EDB->ALARMS_FIRED_THROUGH but were not yet acknowledged.  */
  if (version < 2)
    sqlite_exec (edb->sqliteh,
		 "create table alarms_unacknowledged"
		 " (uid INTEGER, start INTEGER NOT NULL)",
		 NULL, NULL, NULL);


  /* The default calendar.  */

  /* This table definately exists in a version 2 DB and may exist in a
     version 1 DB depending on the revision.  */
  if (version < 2)
    sqlite_exec (edb->sqliteh,
		 "create table default_calendar (default_calendar INTEGER)",
		 NULL, NULL, NULL);
  int default_calendar_callback (void *arg, int argc, char **argv,
				 char **names)
    {
      EventDB *edb = EVENT_DB (arg);
      if (argc == 1)
	edb->default_calendar = atoi (argv[0]);

      return 0;
    }
  edb->default_calendar = EVENT_CALENDAR_NO_PARENT;
  if (sqlite_exec (edb->sqliteh,
		   "select default_calendar from default_calendar",
		   default_calendar_callback, edb, &err))
    goto error;


  /* Calendars.  */

  sqlite_exec (edb->sqliteh,
	       "create table calendars"
	       " (title TEXT, description TEXT,"
	       "  url TEXT, username TEXT, password TEXT,"
	       "  parent INTEGER, hidden INTEGER,"
	       "  has_color INTEGER, red INTEGER, green INTEGER, blue INTEGER,"
	       "  mode INTEGER, sync_interval INTEGER,"
	       "  last_pull INTEGER, last_push INTEGER,"
	       "  last_modified)",
	       NULL, NULL, NULL);
  int load_calendars_callback (void *arg, int argc, char **argv,
			       char **names)
    {
      if (argc != 17)
	{
	  g_critical ("%s: Expected 17 arguments, got %d arguments",
		      __func__, argc);
	  return 0;
	}

      EventCalendar *ec
	= EVENT_CALENDAR (g_object_new (event_calendar_get_type (), NULL));
      ec->edb = edb;

      char **v = argv;
      ec->uid = atoi (*(v ++));
      ec->title = g_strdup (*(v ++));
      ec->description = g_strdup (*(v ++));
      ec->url = g_strdup (*(v ++));
      ec->username = g_strdup (*(v ++));
      ec->password = g_strdup (*(v ++));
      ec->parent_uid = atoi (*(v ++));
      ec->hidden = atoi (*(v ++));
      ec->has_color = atoi (*(v ++));
      ec->red = atoi (*(v ++));
      ec->green = atoi (*(v ++));
      ec->blue = atoi (*(v ++));
      ec->mode = atoi (*(v ++));
      ec->sync_interval = atoi (*(v ++));
      ec->last_pull = atoi (*(v ++));
      ec->last_push = atoi (*(v ++));
      ec->last_modified = atoi (*(v ++));

      edb->calendars = g_slist_prepend (edb->calendars, ec);

      return 0;
    }
  if (sqlite_exec (edb->sqliteh,
		   "select ROWID, title, description,"
		   "  url, username, password,"
		   "  parent, hidden,"
		   "  has_color, red, green, blue,"
		   "  mode, sync_interval, last_pull, last_push, last_modified"
		   " from calendars", load_calendars_callback, NULL, &err))
    {
      char *s = g_strdup_printf ("%s: Reading calendars: %s",
				 __func__, err);
      g_free (err);
      err = s;
      goto error;
    }

  /* If the default calendar was not set, set it now (after we've read
     in the calendars).  */
  if (edb->default_calendar == EVENT_CALENDAR_NO_PARENT)
    {
      EventCalendar *ec = event_db_get_default_calendar (edb, NULL);
      g_object_unref (ec);
    }
    
  if (version < 4)
    sqlite_exec (edb->sqliteh,
		 "create table events_deleted"
		 " (uid INTEGER PRIMARY KEY, eventid STRING NOT NULL, calendar INTEGER);",
		 NULL, NULL, NULL);
    

  /* Update the version information as appropriate.  */
  if (version < 4)
    {
      if (sqlite_exec (edb->sqliteh,
		       "delete from calendar_dbinfo;"
		       "insert into calendar_dbinfo (version) values (4);",
		       NULL, NULL, &err))
	goto error;
    }

  /* All done.  */
  sqlite_exec (edb->sqliteh, "commit transaction;", NULL, NULL, NULL);

  return edb;
 error:
  sqlite_exec (edb->sqliteh, "rollback transaction;", NULL, NULL, NULL);
 error_before_transaction:
  if (err)
    {
      gpe_error_box_fmt ("event_db_new: %s", err);
      free (err);
    }

  if (edb->sqliteh)
    sqlite_close (edb->sqliteh);

  g_object_unref (edb);

  return NULL;
}

Event *
event_db_find_by_uid (EventDB *edb, guint uid)
{
  return EVENT (event_load (edb, uid));
}

Event *
event_db_find_by_eventid (EventDB *edb, const char *eventid)
{
  g_return_val_if_fail (eventid, NULL);

  guint uid = -1;
  int callback (void *arg, int argc, char *argv[], char **names)
    {
      uid = atoi (argv[0]);
      return 1;
    }
  SQLITE_TRY (sqlite_exec_printf (edb->sqliteh,
				  "select uid from events"
				  " where eventid='%q';",
				  callback, NULL, NULL, eventid));
  if (uid == -1)
    return NULL;

  return event_db_find_by_uid (edb, uid);
}

static GSList *
event_db_list_for_period_internal (EventDB *edb,
				   time_t period_start, time_t period_end,
				   gboolean only_untimed, 
				   gboolean alarms)
{
  GSList *list = NULL;

  int callback (EventSource *ev)
    {
      LIVE (ev);

      if (only_untimed && ! ev->untimed)
	goto out;

      GSList *l = event_list (ev, period_start, period_end, 0, alarms);
      list = g_slist_concat (list, l);

    out:
      g_object_unref (ev);
      return 0;
    }

  char *err;
  if (events_enumerate (edb, period_start, period_end, alarms,
			callback, &err))
    {
      g_critical ("%s: %s", __func__, err);
      g_free (err);
    }

  return list;
}

GSList *
event_db_list_for_period (EventDB *edb, time_t start, time_t end)
{
  return event_db_list_for_period_internal (edb, start, end, FALSE, FALSE);
}

GSList *
event_db_list_alarms_for_period (EventDB *edb, time_t start, time_t end)
{
  return event_db_list_for_period_internal (edb, start, end, FALSE, TRUE);
}

GSList *
event_db_untimed_list_for_period (EventDB *edb, time_t start, time_t end)
{
  return event_db_list_for_period_internal (edb, start, end, TRUE, FALSE);
}

EventCalendar *
event_db_find_calendar_by_uid (EventDB *edb, guint uid)
{
  GSList *i;

  for (i = edb->calendars; i; i = i->next)
    if (event_calendar_get_uid (EVENT_CALENDAR (i->data)) == uid)
      {
	g_object_ref (i->data);
	return i->data;
      }

  return NULL;
}

EventCalendar *
event_db_find_calendar_by_name (EventDB *edb, const gchar *name)
{
  g_return_val_if_fail (name, NULL);
    
  GSList *iter;
  for (iter = edb->calendars; iter; iter = iter->next)
    {
      EventCalendar *ec = iter->data;
      gboolean found = FALSE;
      gchar *calendar_name = event_calendar_get_title (ec);
      
      if (!strcmp (calendar_name, name))
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
event_db_get_default_calendar (EventDB *edb, const char *title)
{
  EventCalendar *ec
    = event_db_find_calendar_by_uid (edb, edb->default_calendar);
  if (! ec)
    {
      /* There is no calendar associated with the default calendar id,
	 create it.  */
      ec = event_calendar_new_full (edb, NULL, TRUE, title ?: _("My Calendar"),
				    NULL, NULL, NULL, 0, 0);
      event_db_set_default_calendar (edb, ec);
    }

  return ec;
}

void
event_db_set_default_calendar (EventDB *edb, EventCalendar *ev)
{
  if (ev->uid == edb->default_calendar)
    return;

  edb->default_calendar = ev->uid;

  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (edb->sqliteh,
	"insert or replace into default_calendar values (%d);",
	NULL, NULL, &err, ev->uid)))
    {
      g_critical ("%s: %s", __func__, err);
      g_free (err);
    }
}

GSList *
event_db_list_event_calendars (EventDB *edb)
{
  GSList *l = g_slist_copy (edb->calendars);
  if (! l)
    /* Default calendar doesn't exit.  Create it.  */
    return g_slist_prepend (NULL, event_db_get_default_calendar (edb, NULL));

  GSList *i;
  for (i = l; i; i = i->next)
    g_object_ref (i->data);

  return l;
}
