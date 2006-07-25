/*
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <libintl.h>
#include <glib-object.h>
#include <sqlite.h>
#include <gpe/errorbox.h>
#include <gpe/event-db.h>

#include <obstack.h>

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#define _(x) gettext(x)

/* Execute the sqlite_exec (or sqlite_exec_printf) statement.  If it
   fails because the database or table is locked, try a few times
   before completely failing.  */
#define SQLITE_TRY(statement) \
  ({ \
    int _tries = 0; \
    int _ret; \
    for (;;) \
      { \
        _ret = statement; \
        if ((_ret == SQLITE_BUSY || _ret == SQLITE_LOCKED) && _tries < 3) \
  	{ \
  	  g_main_context_iteration (NULL, FALSE); \
  	  sleep (1); \
  	  _tries ++; \
  	} \
        else \
  	  break; \
      } \
    _ret; \
  })


/* The only thing we use from Gdk is the GdkColor structure.  Avoid a
   dependency and include it here.  */
struct _GdkColor {
  guint32 pixel;
  guint16 red;
  guint16 green;
  guint16 blue;
};

typedef struct
{
  GObjectClass gobject_class;

  /* Signals.  */
  guint calendar_new_signal;
  EventCalendarNew calendar_new;
  guint calendar_deleted_signal;
  EventCalendarDeleted calendar_deleted;
  guint calendar_reparented_signal;
  EventCalendarReparented calendar_reparented;
  guint calendar_changed_signal;
  EventCalendarChanged calendar_changed;
  guint calendar_modified_signal;
  EventCalendarModified calendar_modified;
  
  guint event_new_signal;
  EventNew event_new;
  guint event_removed_signal;
  EventRemoved event_removed;
  guint event_modified_signal;
  EventModified event_modified;

  guint alarm_fired_signal;
  EventDBAlarmFiredFunc alarm_fired;
} EventDBClass;

struct _EventDB
{
  GObject object;

  sqlite *sqliteh;

  GHashTable *events;
  guint default_calendar;
  GSList *calendars;

  /* A list of events that need to be flushed to backing store.  */
  GSList *laundry_list;
  /* The idle source.  */
  guint laundry_buzzer;

  /* A list of events which have no user references.  */
  GSList *cache_list;
  guint cache_buzzer;

  /* The list of events with upcoming alarms.  We hold a reference to
     each.  */
  GSList *upcoming_alarms;
  /* EVENTS contains alarms until this point in time.  */
  time_t period_end;

  /* The alarm source.  */
  guint alarm;

  /* The point through which alarms have been fired (when an alarm
     fires, it is entered into the alarms_unacknowledged table and
     only removed once it has been acknowledged).  */
  time_t alarms_fired_through;
};

typedef struct
{
  GObjectClass gobject_class;
} EventCalendarClass;

struct _EventCalendar
{
  GObject object;

  EventDB *edb;

#define EVENT_CALENDAR_NO_PARENT ((guint) -1)
  guint parent_uid;
  /* This is a cache of the resolved PARENT_UID; it holds a
     reference.  */
  EventCalendar *parent;

  guint uid;

  gboolean hidden;

  char *title;
  char *description;
  char *url;
  char *username;
  char *password;

  gboolean has_color;
  guint16 red;
  guint16 green;
  guint16 blue;

  int mode;
  int sync_interval;
  time_t last_pull;
  time_t last_push;

  gboolean modified;
  time_t last_modified;

  /* True if a calendar attribute has changed which does not affect
     how the calendar contents (e.g. color).  */
  gboolean changed;
};

#define MODIFIED(_ec) \
  do \
    { \
      time_t now = time (NULL); \
      EventCalendar *_p = (_ec); \
      do \
        { \
          _p->last_modified = now; \
	  if (! _p->modified) \
            { \
              _p->modified = TRUE; \
              add_to_laundry_pile (G_OBJECT (_p)); \
            } \
          if ((_ec) != _p) \
            g_object_unref (_p); \
        } \
      while ((_p = event_calendar_get_parent (_p))); \
    } \
  while (0)
       
struct _EventClass
{
  GObjectClass gobject_class;
};

struct _Event
{
  GObject object;
  struct _EventSource *clone_source;
  gboolean dead;

  time_t start;
};

typedef struct _EventSourceClass EventSourceClass;
typedef struct _EventSource EventSource;

extern GType event_source_get_type (void);

#define TYPE_EVENT_SOURCE             (event_source_get_type ())
#define EVENT_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EVENT_SOURCE, EventSource))
#define EVENT_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EVENT_SOURCE, EventSourceClass))
#define IS_EVENT_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EVENT_SOURCE))
#define IS_EVENT_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EVENT_SOURCE))
#define EVENT_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EVENT_SOURCE, EventSourceClass))

struct _EventSourceClass
{
  GObjectClass gobject_class;
};

struct _EventSource
{
  Event event;

  /* The EventDB to which event belongs.  */
  EventDB *edb;

  gboolean untimed;

  /* If this in memory version has been modified (and thus needs to be
     flushed to disk).  */
  gboolean modified;
  time_t last_modified;

  /* When the last extant user reference was removed.  */
  time_t dead_time;

  /* The event calendar to which this event belongs.  */
  guint calendar;

  unsigned long uid;

  unsigned long duration;	/* 0 == instantaneous */
  unsigned long alarm;		/* seconds before event */

  /* iCal's FREQ property.  */
  enum event_recurrence_type type;

  /* No recurrences beyond this time.  0 means forever.  */
  time_t end;

  char *eventid;

  /* Recurrence properties.  */

  /* The number of times this recurrence set is expanded.  0 means
     there is no limit.  */
  unsigned int count;
  /* iCal's interval property: the number of units to skip.  If the
     recurrence type is RECUR_YEARLY then the first recurrence occurs
     INCREMENT years after the initial start.  */
  unsigned int increment;

  /* List of strings of the form:
     [+-]([0-9]*[1-9]|)(MO|TU|WE|TH|FR|SA|SU).  */
  GSList *byday;

  /* A list of start times to exclude.  Must match the start of a
     recurrence exactly.  */
  GSList *exceptions;

  gboolean details;
  /* Fields below here are only valid if DETAILS is true.  */

  gchar *summary;
  gchar *description;
  gchar *location;  
  /* List of integers.  */
  GSList *categories;

  /* Sequence number.  */
  unsigned long sequence;
};

#define LIVE(ev) (g_assert (! EVENT (ev)->dead))
#define STAMP(ev) \
  do \
    { \
      event_details (ev, TRUE); \
      ev->last_modified = time (NULL); \
      if (! ev->modified) \
        { \
          ev->modified = TRUE; \
          add_to_laundry_pile (G_OBJECT (ev)); \
        } \
      EventCalendar *ec = event_get_calendar (EVENT (ev)); \
      MODIFIED (ec); \
      g_object_unref (ec); \
    } \
  while (0)
static void add_to_laundry_pile (GObject *e);
#define NO_CLONE(ev) g_return_if_fail (! ev->clone_source)
#define RESOLVE_CLONE(ev) \
  ((ev)->clone_source ? EVENT_SOURCE (ev->clone_source) : EVENT_SOURCE (ev))

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

static gboolean do_laundry (gpointer data);
static void event_source_toggle_ref_notify (gpointer data, GObject *object,
					    gboolean is_last_ref);

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

/* Acknowledge that event EV has fired.  If EV has no alarm, has not
   yet fired or already been acknowledged, does nothing.  */
void
event_acknowledge (Event *event)
{
  if (event->dead)
    return;

  EventSource *ev = RESOLVE_CLONE (event);
  char *err;

  if (SQLITE_TRY (sqlite_exec_printf (ev->edb->sqliteh,
				      "delete from alarms_unacknowledged"
				      " where uid=%d and start=%d",
				      NULL, NULL, &err,
				      ev->uid, ev->event.start)))
    {
      g_warning ("%s: removing event %ld from unacknowledged list: %s",
		 __func__, ev->uid, err);
      g_free (err);
    }
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

static GSList *event_list (EventSource *ev,
			   time_t period_start, time_t period_end,
			   int max, gboolean per_alarm);
static gboolean buzzer (gpointer data);

/* EV is new or has recently changed: check to see if it has an alarm
   which will go off in the near future.  */
static void
event_add_upcoming_alarms (EventSource *ev)
{
  if (! ev->edb->alarm)
    /* Alarms have not yet been activated.  */
    return;

  time_t now = time (NULL);
  GSList *list = event_list (ev, now, ev->edb->period_end, 0, TRUE);
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

/* Invoked by a timeout source when either an alarm should go off or
   when we need to look for additional upcoming events.  */
static gboolean
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
  Event *event = EVENT (instance);

  event->dead = FALSE;
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

  event->dead = TRUE;

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
      if (ev->dead_time + 60 <= now)
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

static void
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
	ev->edb->cache_buzzer = g_timeout_add (120 * 1000,
					       flush_cache, ev->edb);
    }
  else
    /* Someone just grabbed a reference.  Remove it from the kill
       list.  */
    ev->edb->cache_list = g_slist_remove (ev->edb->cache_list, ev);
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

static void event_details (EventSource *ev, gboolean fill_from_disk);
static gboolean event_write (EventSource *, char **);

static void
event_source_finalize (GObject *object)
{
  EventSource *event = EVENT_SOURCE (object);

  if (event->modified)
    /* Flush to disk.  */
    {
      char *err;
      if (! event_write (event, &err))
	{
	  g_critical ("%s: %s", __func__, err);
	  free (err);
	}
    }

  /* Free any details.  */
  if (event->description)
    g_free (event->description);
  if (event->location)
    g_free (event->location);
  if (event->summary)
    g_free (event->summary);
  g_slist_free (event->categories);

  g_free (event->eventid);
  g_slist_free (event->exceptions);

  event_recurrence_byday_free (event->byday);

  /* EVENT may not have been inserted into the hash if we created a
     new event and were unable to insert it into the DB.  In this
     case, EVENT->UID will be 0.  */
  if (event->uid)
    {
      gboolean removed = g_hash_table_remove (event->edb->events,
					      (gpointer) event->uid);
      g_assert (removed);
    }

  G_OBJECT_CLASS (event_parent_class)->finalize (object);
}

/* Here we create a globally unique eventid, which we
 * can use to reference this event in a vcal, etc. */
static gchar *
event_db_make_eventid (void)
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

  return g_strdup_printf ("%lu.%lu%d@%s",
			  (unsigned long) time (NULL),
			  (unsigned long) getpid(), rand (),
			  hostname);
}

static gboolean
parse_date (char *s, time_t *t, gboolean *date_only)
{
  struct tm tm;
  char *p;

  memset (&tm, 0, sizeof (tm));
  tm.tm_isdst = -1;
  p = strptime (s, "%Y-%m-%d", &tm);
  if (p == NULL)
    {
      fprintf (stderr, "Unable to parse date: %s\n", s);
      return FALSE;
    }

  p = strptime (p, " %H:%M", &tm);
  if (p && *p == ':')
    /* Seconds used to be optional.  */
    p = strptime (p, ":%S", &tm);

  if (date_only)
    *date_only = (p == NULL) ? TRUE : FALSE;

  if (p)
    /* The time is in UTC.  */
    *t = timegm (&tm);
  else
    /* There is no time component but we want to identify the start of
       the day in the local time zone.  */
    *t = mktime (&tm);

  return TRUE;
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
		 " (uid INTEGER, eventid STRING NOT NULL, calendar INTEGER);",
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

static int
event_load_callback (void *arg, int argc, char **argv, char **names)
{
  EventSource *ev = EVENT_SOURCE (arg);

  /* argv[0] is the UID and that is already set.  */
  int i = 1;
  parse_date (argv[i], &ev->event.start, &ev->untimed);

  i ++;
  if (argv[i])
    ev->duration = atoi (argv[i]);

  i ++;
  if (argv[i])
    ev->type = atoi (argv[i]);
  if (ev->type < 0 || ev->type >= RECUR_COUNT)
    {
      g_warning ("Event %ld has unknown recurrence type %d",
		 ev->uid, ev->type);
      ev->type = 0;
    }

  i ++;
  if (argv[i])
    parse_date (argv[i], &ev->end, NULL);
  i ++;
  if (argv[i])
    ev->alarm = atoi (argv[i]);
  i ++;
  if (argv[i])
    ev->calendar = atoi (argv[i]);

  i ++;
  ev->eventid = g_strdup (argv[i]);

  i ++;
  if (argv[i])
    ev->count = atoi (argv[i]);
  i ++;
  if (argv[i])
    ev->increment = atoi (argv[i]);

  i ++;
  if (argv[i])
    {
      if (strchr (argv[i], '-'))
	parse_date (argv[i], &ev->last_modified, NULL);
      else
	ev->last_modified = strtoul (argv[i], NULL, 10);
    }

  i ++;
  char *p = argv[i];
  while (p)
    {
      char *end = strchr (p, ',');
      char *token;
      if (end)
	token = g_strdup_printf ("%*s", end - p, p);
      else
	token = g_strdup (p);
      ev->byday = g_slist_prepend (ev->byday, token);

      if (end)
	p = end + 1;
      else
	break;
    }

  i ++;
  p = argv[i];
  while (p)
    {
      long rmtime = (long)atoi (p);
      ev->exceptions = g_slist_prepend (ev->exceptions, (void *) rmtime);

      p = strchr (p, ',');
      if (p)
	p ++;
    }

  return 0;
}

static EventSource *
event_load (EventDB *edb, guint uid)
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
  g_hash_table_insert (edb->events, (gpointer) ev->uid, ev);

  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (edb->sqliteh,
	"select * from events where uid=%d",
	event_load_callback, ev, &err, uid)))
    {
      g_warning ("%s:%d: Loading data for event %ld: %s",
		 __func__, __LINE__, ev->uid, err);
      free (err);
      g_object_unref (ev);
      return NULL;
    }

  return ev;
}


static int
load_details_callback (void *arg, int argc, char *argv[], char **names)
{
  if (argc == 2)
    {
      EventSource *ev = arg;
      if (!strcmp (argv[0], "summary") && !ev->summary)
        ev->summary = g_strdup (argv[1]);
      else if (!strcmp (argv[0], "description") && !ev->description)
        ev->description = g_strdup (argv[1]);
      else if (!strcmp (argv[0], "location") && !ev->location)
        ev->location = g_strdup (argv[1]);
      else if (!strcmp (argv[0], "sequence"))
        ev->sequence = atoi (argv[1]);
      else if (!strcmp (argv[0], "category"))
        ev->categories = g_slist_prepend (ev->categories,
					  (gpointer)atoi (argv[1]));
    }
  return 0;
}

/**
 * event_details:
 * @ev: Event to get details for.
 * 
 * Makes sure that #ev's details structure is in core.
 */
static void
event_details (EventSource *ev, gboolean fill_from_disk)
{
  char *err;

  if (ev->details)
    return;

  if (! fill_from_disk)
    return;

  if (SQLITE_TRY
      (sqlite_exec_printf (ev->edb->sqliteh,
			   "select tag,value from calendar"
			   " where uid=%d"
			   "  and tag in ('summary',"
			   "              'description', 'location',"
			   "              'category', 'sequence')",
			   load_details_callback, ev, &err, ev->uid)))
    {
      gpe_error_box (err);
      free (err);
      return;
    }
  ev->details = TRUE;
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

/**
 * event_list_unref:
 * @l: Event list
 *
 * This function frees a given list of events including the list itself. 
 */
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

/* List up to MAX (0 means unlimited) instances of EV which occur
   between PERIOD_START and PERIOD_END, inclusive.  If PER_ALARM is
   true then the instances which have an alarm which goes off between
   PERIOD_START and PERIOD_END are returned.  */
static GSList *
event_list (EventSource *ev, time_t period_start, time_t period_end, int max,
	    gboolean per_alarm)
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

/**
 * event_db_list_for_period:
 * @start: Start time (inclusive)
 * @end: End time (inclusive)
 * 
 * Create a list of all events for a given period.
 *
 * Returns: A list of events.
 */
GSList *
event_db_list_for_period (EventDB *edb, time_t start, time_t end)
{
  return event_db_list_for_period_internal (edb, start, end, FALSE, FALSE);
}

/**
 * event_db_list_alarms_for_period:
 * @start: Start time
 * @end: End time
 * 
 * Create a list of all alarm events for a given period.
 *
 * Returns: A list of events.
 */
GSList *
event_db_list_alarms_for_period (EventDB *edb, time_t start, time_t end)
{
  return event_db_list_for_period_internal (edb, start, end, FALSE, TRUE);
}

/**
 * event_db_untimed_list_for_period:
 * @start: Start time
 * @end: End time
 * 
 * Create a list of all untimed events for a given period.
 *
 * Returns: A list of events.
 */
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

/**
 * event_db_find_calendar_by_name:
 * @edb: Event database
 * @name: Calendar name
 *
 * Get a desired calendar by name.
 * 
 * Returns: An #EventCalendar object or NULL if not found.
 */
EventCalendar *
event_db_find_calendar_by_name (EventDB *edb, const gchar *name)
{
  GSList *iter;

  g_return_val_if_fail (name, NULL);
    
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

static void
event_db_calendar_new (EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (ec->edb->sqliteh,
	"insert into calendars values"
	" ('%q', '%q', '%q', '%q', '%q', %d, %d, %d, %d, %d,"
	"  %d, %d, %d, %d, %d, %d)",
	NULL, NULL, &err,
	ec->title ?: "", ec->description ?: "",
	ec->url ?: "",
	ec->username ?: "", ec->password ?: "",
	ec->parent_uid, ec->hidden,
	ec->has_color, ec->red, ec->green, ec->blue,
	ec->mode, ec->sync_interval,
	ec->last_pull, ec->last_push,
	ec->last_modified)))
    {
      g_warning ("%s: %s", __func__, err);
      g_free (err);
    }

  ec->uid = sqlite_last_insert_rowid (ec->edb->sqliteh);
}

static void event_calendar_class_init (gpointer klass, gpointer klass_data);
static void event_calendar_init (GTypeInstance *instance, gpointer klass);
static void event_calendar_dispose (GObject *obj);
static void event_calendar_finalize (GObject *object);

static GObjectClass *event_calendar_parent_class;

GType
event_calendar_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventCalendarClass),
	NULL,
	NULL,
	event_calendar_class_init,
	NULL,
	NULL,
	sizeof (struct _EventCalendar),
	0,
	event_calendar_init
      };

      type = g_type_register_static (G_TYPE_OBJECT, "EventCalendar", &info, 0);
    }

  return type;
}

static void
event_calendar_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;

  event_calendar_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = event_calendar_finalize;
  object_class->dispose = event_calendar_dispose;
}

static void
event_calendar_init (GTypeInstance *instance, gpointer klass)
{
}

static void
event_calendar_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (event_calendar_parent_class)->dispose (obj);
}

static void
event_calendar_flush (EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf (ec->edb->sqliteh,
			   "update calendars set"
			   "  title='%q', description='%q',"
			   "  url='%q', username='%q', password='%q',"
			   "  parent=%d, hidden=%d,"
			   "  has_color=%d, red=%d, green=%d, blue=%d,"
			   "  mode=%d, sync_interval=%d,"
			   "  last_pull=%d, last_push=%d,"
			   "  last_modified=%d"
			   " where ROWID=%d;",
			   NULL, NULL, &err,
			   ec->title ?: "", ec->description ?: "",
			   ec->url ?: "", ec->username ?: "",
			   ec->password ?: "",
			   ec->parent_uid, ec->hidden,
			   ec->has_color, ec->red, ec->green, ec->blue,
			   ec->mode, ec->sync_interval,
			   ec->last_push, ec->last_pull, ec->last_modified,
			   ec->uid)))
    {
      g_critical ("%s: updating %s (%d): %s", __func__,
		  ec->description, ec->uid, err);
      g_free (err);
    }

  ec->modified = FALSE;
}

static void
event_calendar_finalize (GObject *object)
{
  EventCalendar *ec = EVENT_CALENDAR (object);

  if (ec->parent)
    g_object_unref (ec->parent);

  g_free (ec->title);
  g_free (ec->description);
  g_free (ec->url);
  g_free (ec->username);
  g_free (ec->password);

  G_OBJECT_CLASS (event_calendar_parent_class)->finalize (object);
}

EventCalendar *
event_calendar_new (EventDB *edb)
{
  EventCalendar *ec = EVENT_CALENDAR (g_object_new (event_calendar_get_type (),
						    NULL));

  ec->edb = edb;
  event_db_calendar_new (ec);

  g_object_ref (ec);
  edb->calendars = g_slist_prepend (edb->calendars, ec);

  g_signal_emit (edb, EVENT_DB_GET_CLASS (edb)->calendar_new_signal, 0, ec);

  return ec;
}

EventCalendar *
event_calendar_new_full (EventDB *edb,
			 EventCalendar *parent,
			 gboolean visible,
			 const char *title,
			 const char *description,
			 const char *url,
			 struct _GdkColor *color,
			 int mode,
			 int sync_interval)
{
  g_return_val_if_fail (mode >= 0, NULL);
  g_return_val_if_fail (mode <= 4, NULL);

  EventCalendar *ec = EVENT_CALENDAR (g_object_new (event_calendar_get_type (),
						    NULL));

  ec->edb = edb;
  if (parent)
    {
      ec->parent_uid = parent->uid;
      g_object_ref (parent);
      ec->parent = parent;
    }
  else
    ec->parent_uid = EVENT_CALENDAR_NO_PARENT;

  ec->hidden = ! visible;

  ec->title = title ? g_strdup (title) : NULL;
  ec->description = description ? g_strdup (description) : NULL;
  ec->url = url ? g_strdup (url) : NULL;
  if (color)
    {
      ec->has_color = TRUE;
      ec->red = color->red;
      ec->green = color->green;
      ec->blue = color->blue;
    }
  ec->mode = mode;
  ec->sync_interval = sync_interval;

  event_db_calendar_new (ec);

  g_object_ref (ec);
  edb->calendars = g_slist_prepend (edb->calendars, ec);

  g_signal_emit (edb, EVENT_DB_GET_CLASS (edb)->calendar_new_signal, 0, ec);

  return ec;
}

gboolean
event_calendar_valid_parent (EventCalendar *ec, EventCalendar *new_parent)
{
  EventCalendar *i;
  for (i = new_parent; i; i = event_calendar_get_parent (i))
    if (i == ec)
      return FALSE;
  return TRUE;
}

void
event_calendar_delete (EventCalendar *ec,
		       gboolean delete_events,
		       EventCalendar *new_parent)
{
  if (! delete_events && ! event_calendar_valid_parent (ec, new_parent))
    {
      g_critical ("%s: I refuse to create a cycle.", __func__);
      return;
    }

  GSList *link = NULL;

  /* Remove the calendars which are descendents of EC.  */
  GSList *i;
  GSList *next = ec->edb->calendars;
  while (next)
    {
      i = next;
      next = next->next;

      EventCalendar *c = EVENT_CALENDAR (i->data);

      if (c == ec)
	link = i;

      if (c->parent_uid == ec->uid)
	{
	  if (delete_events)
	    event_calendar_delete (c, TRUE, 0);
	  else
	    event_calendar_set_parent (c, new_parent);
	}
    }

  g_assert (link);
  ec->edb->calendars = g_slist_delete_link (ec->edb->calendars, link);

  GSList *events = event_calendar_list_events (ec);
  for (i = events; i; i = i->next)
    {
      Event *ev = EVENT (i->data);
      if (delete_events)
	event_remove (ev);
      else
	event_set_calendar (ev, new_parent);
    }
  event_list_unref (events);

  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (ec->edb->sqliteh,
	"begin transaction;"
	/* Remove the events.  */
	"delete from calendar where uid"
	" in (select uid from events where calendar=%d);"
	"delete from events where calendar=%d;"
	/* And the calendar.  */
	"delete from calendars where ROWID=%d;"
	"commit transaction",
	NULL, NULL, &err, ec->uid, ec->uid, ec->uid)))
    {
      sqlite_exec (ec->edb->sqliteh, "rollback transaction;",
		   NULL, NULL, NULL);
      g_critical ("%s: %s", __func__, err);
      g_free (err);
    }

  g_signal_emit (ec->edb,
		 EVENT_DB_GET_CLASS (ec->edb)->calendar_deleted_signal, 0, ec);
}

#define GET(type, name) \
  type \
  event_calendar_get_##name (EventCalendar *ec) \
  { \
    return ec->name; \
  }

#define GET_SET(type, name, is_modification) \
  GET(type, name) \
  \
  void \
  event_calendar_set_##name (EventCalendar *ec, type name) \
  { \
    if (ec->name == name) \
      return; \
    ec->name = name; \
    if (is_modification) \
      { \
        ec->modified = TRUE; \
        ec->last_modified = time (NULL); \
      } \
    ec->changed = TRUE; \
    add_to_laundry_pile (G_OBJECT (ec)); \
  }

GET(guint, uid)

EventCalendar *
event_calendar_get_parent (EventCalendar *ec)
{
  if (ec->parent_uid == EVENT_CALENDAR_NO_PARENT)
    return NULL;

  if (! ec->parent)
    {
      ec->parent = event_db_find_calendar_by_uid (ec->edb, ec->parent_uid);
      g_object_ref (ec->parent);
      if (! ec->parent)
	{
	  g_warning ("Calendar (%s) %d contains a dangling parent %d!",
		     ec->description, ec->uid, ec->parent_uid);
	  return NULL;
	}
    }

  g_object_ref (ec->parent);
  return ec->parent;
}

void
event_calendar_set_parent (EventCalendar *ec, EventCalendar *p)
{
  if ((! p && ec->parent_uid == EVENT_CALENDAR_NO_PARENT)
      || (p && p->uid == ec->uid))
    return;

  g_return_if_fail (event_calendar_valid_parent (ec, p));

  if (ec->parent)
    g_object_unref (ec->parent);
  ec->parent = p;
  if (p)
    g_object_ref (p);

  ec->parent_uid = p ? p->uid : EVENT_CALENDAR_NO_PARENT;

  ec->modified = TRUE;
  ec->last_modified = time (NULL);
  ec->changed = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));

  g_signal_emit (ec->edb,
		 EVENT_DB_GET_CLASS (ec->edb)->calendar_reparented_signal,
		 0, ec);
}

gboolean
event_calendar_get_visible (EventCalendar *ec)
{
  return ! ec->hidden;
}

void
event_calendar_set_visible (EventCalendar *ec, gboolean visible)
{
  if (ec->hidden == ! visible)
    return;
  ec->hidden = ! visible;
  ec->changed = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));
}

#define GET_SET_STRING(name, is_modification) \
  char * \
  event_calendar_get_##name (EventCalendar *ec) \
  { \
    return g_strdup (ec->name); \
  } \
  \
  void \
  event_calendar_set_##name (EventCalendar *ec, const char *name) \
  { \
    if (ec->name && strcmp (ec->name, name) == 0) \
      return; \
    g_free (ec->name); \
    ec->name = g_strdup (name); \
    if (is_modification) \
      { \
        ec->modified = TRUE; \
        ec->last_modified = time (NULL); \
      } \
    ec->changed = TRUE; \
    add_to_laundry_pile (G_OBJECT (ec)); \
  }

GET_SET_STRING(title, TRUE)
GET_SET_STRING(description, TRUE)
GET_SET_STRING(url, TRUE)
GET_SET_STRING(username, TRUE)
GET_SET_STRING(password, TRUE)

GET_SET(int, mode, TRUE)

gboolean
event_calendar_get_color (EventCalendar *ec, struct _GdkColor *color)
{
  if (! ec->has_color)
    return FALSE;

  color->red = ec->red;
  color->green = ec->green;
  color->blue = ec->blue;

  return TRUE;
}

void
event_calendar_set_color (EventCalendar *ec, const struct _GdkColor *color)
{
  if (color)
    {
      ec->has_color = TRUE;
      ec->red = color->red;
      ec->green = color->green;
      ec->blue = color->blue;
    }
  else
    ec->has_color = FALSE;

  ec->changed = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));
}

GET_SET(int, sync_interval, FALSE)
GET_SET(time_t, last_pull, FALSE)
GET_SET(time_t, last_push, FALSE)

time_t
event_calendar_get_last_modification (EventCalendar *ec)
{
  return ec->last_modified;
}

GSList *
event_calendar_list_events (EventCalendar *ec)
{
  GSList *list = NULL;

  int callback (void *arg, int argc, char *argv[], char **names)
    {
      Event *ev = event_db_find_by_uid (ec->edb, atoi (argv[0]));
      if (ev)
        list = g_slist_prepend (list, ev);
      return 0;
    }
  SQLITE_TRY
    (sqlite_exec_printf (ec->edb->sqliteh,
			 "select uid from events where calendar=%d;",
			 callback, NULL, NULL, ec->uid));

  return list;
}

/**
 * event_calendar_list_deleted:
 * @ec: An #EventCalendar
 *
 * Retrieve a list of deleted events since the last event_calendar_flush_deleted()
 * call. The events contain a very basic set of information only and should 
 * be used to indicate wether a particular event was deleted or not only.
 * The returned list of events needs to be freed by the caller.
 *
 * Returns: A list of events.
 */
GSList *
event_calendar_list_deleted (EventCalendar *ec)
{
  GSList *list = NULL;

  int callback (void *arg, int argc, char *argv[], char **names)
    {
      EventSource *ev;
      
      if (argc != 3) 
          return 0;
      ev = EVENT_SOURCE (g_object_new (event_source_get_type (), NULL));
      ev->edb = ec->edb;
      ev->uid = atoi (argv[0]);
      ev->eventid = g_strdup (argv[1]);
      ev->calendar = atoi (argv[2]);

      list = g_slist_prepend (list, ev);
      return 0;
    }
  SQLITE_TRY
    (sqlite_exec_printf (ec->edb->sqliteh,
			 "select uid, eventid, calendar from events_deleted where calendar=%d;",
			 callback, NULL, NULL, ec->uid));

  return list;
}

GSList *
event_calendar_list_calendars (EventCalendar *p)
{
  GSList *c = NULL;
  GSList *i;
  for (i = p->edb->calendars; i; i = i->next)
    {
      EventCalendar *ec = EVENT_CALENDAR (i->data);
      if (ec->parent_uid == p->uid)
        {
          g_object_ref (ec);
          c = g_slist_prepend (c, ec);
        }
    }

  return c;
}

void 
event_calendar_flush_deleted (EventCalendar *ec)
{
   SQLITE_TRY (sqlite_exec_printf (ec->edb->sqliteh,
			     "delete from events_deleted where calendar=%d;",
			     NULL, NULL, NULL, ec->uid));
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

/* Dump an event to the SQL database.  */
static gboolean
event_write (EventSource *ev, char **err)
{
  struct tm tm;

  if (ev->untimed)
    localtime_r (&ev->event.start, &tm);
  else
    gmtime_r (&ev->event.start, &tm);
  char start[64];
  strftime (start, sizeof (start), 
	    ev->untimed ? "%Y-%m-%d" : "%Y-%m-%d %T", &tm);  

  char end[64];
  if (! ev->end)
    sprintf (end, "NULL");
  else
    {
      gmtime_r (&ev->end, &tm);
      strftime (end, sizeof (end), 
		ev->untimed ? "'%Y-%m-%d'" : "'%Y-%m-%d %T'", &tm); 
    }

  char *byday = NULL;
  if (ev->byday)
    {
      GSList *l;
      for (l = ev->byday; l; l = l->next)
	if (byday)
	  {
	    char *t = g_strdup_printf ("%s,%s", byday, (char *) l->data);
	    g_free (byday);
	    byday = t;
	  }
	else
	  byday = g_strdup (l->data);
    }

  char *exceptions = NULL;
  if (ev->exceptions)
    {
      GSList *l;
      for (l = ev->exceptions; l; l = l->next)
	if (exceptions)
	  {
	    char *t = g_strdup_printf ("%s,%ld", exceptions, (long) l->data);
	    g_free (exceptions);
	    exceptions = t;
	  }
	else
	  exceptions = g_strdup_printf ("%ld", (long) l->data);
    }

  if (sqlite_exec_printf
      (ev->edb->sqliteh,
       "update events set start='%q', duration=%u, recur=%d, rend=%s,"
       "  alarm=%d, calendar=%d, eventid='%q', rcount=%d,"
       "  rincrement=%d, modified=%u, byday='%q', rexceptions='%q'"
       " where uid=%u;",
       NULL, NULL, err,
       start, ev->duration, ev->type, end, ev->alarm, ev->calendar,
       ev->eventid, ev->count, ev->increment, ev->last_modified,
       byday, exceptions, ev->uid))
    goto error;

  if (ev->details)
    {
      if (sqlite_exec_printf
	  (ev->edb->sqliteh,
	   "delete from calendar where uid=%d;"
	   "insert or replace into calendar values (%d, 'sequence', %d);"
	   "insert or replace into calendar values (%d, 'summary', '%q');"
	   "insert or replace into calendar values (%d, 'description', '%q');"
	   "insert or replace into calendar values (%d, 'location', '%q');",
	   NULL, NULL, err,
	   ev->uid,
	   ev->uid, ev->sequence,
	   ev->uid, ev->summary ?: "",
	   ev->uid, ev->description ?: "",
	   ev->uid, ev->location ?: ""))
	goto error;

      GSList *i;
      for (i = ev->categories; i; i = i->next)
	if (sqlite_exec_printf
	    (ev->edb->sqliteh,
	     "insert into calendar values (%d, 'category', '%d');",
	     NULL, NULL, err, ev->uid, (int) i->data))
	  goto error;
    }

  ev->modified = FALSE;
  return TRUE;
error:
  return FALSE;
}

/**
 * event_flush
 * @ev: Event
 * 
 * Commit the changes to an event to the database.
 *
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_flush (Event *event)
{
  if (event->dead)
    return TRUE;

  EventSource *ev = RESOLVE_CLONE (event);
  char *err;

  if (SQLITE_TRY
      (sqlite_exec
       (ev->edb->sqliteh, "begin transaction", NULL, NULL, &err)))
    goto error;

  if (event_write (ev, &err))
    sqlite_exec (ev->edb->sqliteh, "commit transaction",
		 NULL, NULL, &err);
  else
    {
      sqlite_exec (ev->edb->sqliteh, "rollback transaction",
		   NULL, NULL, NULL);
      goto error;
    }

  g_signal_emit
    (ev->edb, EVENT_DB_GET_CLASS (ev->edb)->event_modified_signal,
     0, ev);

  return TRUE;

 error:
  if (err)
    {
      g_critical ("%s: %s", __func__, err);
      gpe_error_box (err);
      free (err);
    }
  return FALSE;
}

/* Write all dirty events EDB->LAUNDRY_LIST to backing store.  Called
   by the idle loop and set in add_to_laundry_pile.  */
static gboolean
do_laundry (gpointer data)
{
  EventDB *edb = EVENT_DB (data);
  GSList *l;

  /* Don't run again.  */
  if (edb->laundry_buzzer)
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

/* E is an Event or an EventCalendar and is dirty (i.e. needs to be
   written to disk).  Add it to the laundry list and do it when we are
   idle.  */
static void
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

gboolean
event_remove (Event *event)
{
  if (event->dead)
    return TRUE;

  EventSource *ev = RESOLVE_CLONE (event);

  if (SQLITE_TRY
      (sqlite_exec_printf (ev->edb->sqliteh,
			   "begin transaction;"
               "insert into events_deleted values ('%d', '%q', '%d');"
			   "delete from calendar where uid=%d;"
			   "delete from events where uid=%d;"
			   "commit transaction;",
			   NULL, NULL, NULL, ev->uid, ev->eventid, ev->calendar, 
               ev->uid, ev->uid)))
    {
      sqlite_exec (ev->edb->sqliteh, "rollback transaction;",
		           NULL, NULL, NULL);
      return FALSE;
    }

  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      event_acknowledge (EVENT (ev));
      /* And remove it from the upcoming alarm list.  */
      event_remove_upcoming_alarms (ev);
    }

  EVENT (ev)->dead = TRUE;
  g_signal_emit (ev->edb,
		 EVENT_DB_GET_CLASS (ev->edb)->event_removed_signal,
		 0, ev);

  return TRUE;
}

Event *
event_new (EventDB *edb, EventCalendar *ec, const char *eventid)
{
  EventSource *ev = EVENT_SOURCE (g_object_new (event_source_get_type (),
						NULL));
  gchar *err = NULL;

  ev->edb = edb;
  if (ec)
    ev->calendar = ec->uid;
  else
    ev->calendar = edb->default_calendar;

  if (eventid)
    {
      Event *t = event_db_find_by_eventid (edb, eventid);
      if (t)
	{
	  g_critical ("Attempted to add event with an eventid "
		      "which is already present in the database.");
	  g_object_unref (t);
	  return NULL;
	}
	  
      ev->eventid = g_strdup (eventid);
    }
  else
    ev->eventid = event_db_make_eventid ();

  if (SQLITE_TRY (sqlite_exec (ev->edb->sqliteh,
			       "begin transaction;",
			       NULL, NULL, &err)))
    goto error_no_roll_back;

  if (sqlite_exec (ev->edb->sqliteh,
		   "insert into events (start) values (NULL);",
		   NULL, NULL, &err))
    goto error;

  ev->uid = sqlite_last_insert_rowid (ev->edb->sqliteh);
  g_hash_table_insert (edb->events, (gpointer) ev->uid, ev);

  if (event_write (ev, &err) == FALSE
      || sqlite_exec (edb->sqliteh, "commit transaction", NULL, NULL, &err))
    goto error;

  g_signal_emit (edb, EVENT_DB_GET_CLASS (edb)->event_new_signal, 0, ev);

  return EVENT (ev);

 error:
  sqlite_exec (edb->sqliteh, "rollback transaction", NULL, NULL, NULL);
 error_no_roll_back:
  g_object_unref (ev);
  gpe_error_box (err);
  g_free (err);

  return NULL;
}

EventDB *
event_get_event_db (Event *e)
{
  EventSource *ev = RESOLVE_CLONE (e);

  return ev->edb;
}

EventCalendar *
event_get_calendar (Event *e)
{
  EventSource *ev = RESOLVE_CLONE (e);
  EventCalendar *ec = event_db_find_calendar_by_uid (ev->edb, ev->calendar);
  if (! ec)
    {
      g_warning ("%s: Encountered orphaned event %s (%ld):"
		 " being adopted by default calendar",
		 __func__, ev->summary ?: "", ev->uid);
      
      ec = event_db_get_default_calendar (ev->edb, NULL);
      event_set_calendar (EVENT (ev), ec);
      return ec;
    }
  else
    {
      g_object_ref (ec);
      return ec;
    }
}

void
event_set_calendar (Event *e, EventCalendar *ec)
{
  EventSource *ev = RESOLVE_CLONE (e);
  if (! ec)
    ec = event_db_get_default_calendar (ev->edb, NULL);
  if (ev->calendar == ec->uid)
    return;

  ev->calendar = ec->uid;
  STAMP (ev); 
}

gboolean
event_get_color (Event *ev, struct _GdkColor *color)
{
  EventCalendar *ec = event_get_calendar (ev);
  do
    {
      int ret = event_calendar_get_color (ec, color);
      g_object_unref (ec);
      if (ret)
	return ret;

      ec = event_calendar_get_parent (ec);
    }
  while (ec);

  return FALSE;
}

gboolean
event_get_visible (Event *ev)
{
  EventCalendar *ec = event_get_calendar (ev);
  do
    {
      int v = event_calendar_get_visible (ec);
      g_object_unref (ec);
      if (! v)
	return FALSE;

      ec = event_calendar_get_parent (ec);
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
event_set_duration (Event *event, unsigned long duration)
{
  EventSource *ev = RESOLVE_CLONE (event);
  if (ev->duration == duration)
    return;

  ev->duration = duration;
  STAMP (ev);
}

#undef GET
#define GET(type, name, field, detail) \
  type \
  event_get_##name (Event *event) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    if (detail) \
      event_details (ev, TRUE); \
    return ev->field; \
  }

#undef GET_SET
#define GET_SET(type, name, field, alarm_hazard, detail) \
  GET (type, name, field, detail) \
  \
  void \
  event_set_##name (Event *event, type value) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    if (detail) \
      event_details (ev, TRUE); \
    if (ev->field == value) \
      return; \
    \
    if ((alarm_hazard) && ev->alarm) \
      { \
        event_acknowledge (EVENT (ev)); \
        event_remove_upcoming_alarms (ev); \
      } \
    ev->field = value; \
    if ((alarm_hazard) && ev->alarm) \
      event_add_upcoming_alarms (ev); \
    \
    STAMP (ev); \
  }

GET_SET (unsigned long, alarm, alarm, TRUE, FALSE)
GET_SET (guint32, sequence, sequence, FALSE, TRUE)
GET_SET (enum event_recurrence_type, recurrence_type, type, TRUE, FALSE)
GET (time_t, recurrence_start, event.start, FALSE)

void
event_set_recurrence_start (Event *event, time_t start)
{
  EventSource *ev = RESOLVE_CLONE (event);

  if (ev->event.start == start)
    return;

  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      event_acknowledge (EVENT (ev));
      /* And remove it from the upcoming alarm list.  */
      event_remove_upcoming_alarms (ev);
    }
  ev->event.start = start;
  if (ev->alarm)
    /* And remove it from the upcoming alarm list.  */
    event_add_upcoming_alarms (ev);

  STAMP (ev);
}

GET_SET (time_t, recurrence_end, end, TRUE, FALSE)
GET_SET (guint32, recurrence_count, count, TRUE, FALSE)
GET_SET (guint32, recurrence_increment, increment, TRUE, FALSE)

GET_SET (gboolean, untimed, untimed, FALSE, FALSE);

GET (unsigned long, uid, uid, FALSE)

char *
event_get_eventid (Event *event) \
{
  EventSource *ev = RESOLVE_CLONE (event);
  return g_strdup (ev->eventid);
}

#undef GET_SET_STRING
#define GET_SET_STRING(field) \
  char * \
  event_get_##field (Event *event) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    event_details (ev, TRUE); \
    return g_strdup (ev->field); \
  } \
 \
  void \
  event_set_##field (Event *event, const char *field) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    \
    event_details (ev, TRUE); \
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
    STAMP (ev); \
  }

GET_SET_STRING(summary)
GET_SET_STRING(location)
GET_SET_STRING(description)

GSList *
event_get_categories (Event *event)
{
  EventSource *ev = RESOLVE_CLONE (event);
  event_details (ev, TRUE);
  return g_slist_copy (ev->categories);
}

void
event_add_category (Event *event, int category)
{
  EventSource *ev = RESOLVE_CLONE (event);
  event_details (ev, TRUE);
  ev->categories = g_slist_prepend (ev->categories, (gpointer) category);

  STAMP (ev);
}

void
event_set_categories (Event *event, GSList *categories)
{
  EventSource *ev = RESOLVE_CLONE (event);

  event_details (ev, TRUE);
  g_slist_free (ev->categories);
  ev->categories = categories;

  STAMP (ev);
}

GSList *
event_get_recurrence_byday (Event *event)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GSList *list = NULL;
  GSList *l;
  for (l = ev->byday; l; l = l->next)
    list = g_slist_prepend (list, g_strdup (l->data));

  return list;
}

void
event_set_recurrence_byday (Event *event, GSList *list)
{
  EventSource *ev = RESOLVE_CLONE (event);

  GSList *l;
  for (l = ev->byday; l; l = l->next)
    g_free (l->data);
  g_slist_free (ev->byday);

  ev->byday = list;

  STAMP (ev);
}

void
event_add_recurrence_exception (Event *event, time_t start)
{
  EventSource *ev = RESOLVE_CLONE (event);

  ev->exceptions = g_slist_append (ev->exceptions, (void *) start);

  STAMP (ev);
}
