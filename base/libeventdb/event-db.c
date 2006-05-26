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
#include <math.h>
#include <sys/stat.h>

#include <libintl.h>
#include <glib-object.h>
#include <sqlite.h>
#include <gpe/errorbox.h>
#include <gpe/event-db.h>

#define _(x) gettext(x)

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
  
  guint event_new_signal;
  void (*event_new) (EventDB *view, Event *event);
  guint event_removed_signal;
  void (*event_removed) (EventDB *view, Event *event);
  guint event_changed_signal;
  void (*event_changed) (EventDB *view, Event *event);

  guint alarm_fired_signal;
  EventDBAlarmFiredFunc alarm_fired;
} EventDBClass;

struct _EventDB
{
  GObject object;

  unsigned long dbversion;
  sqlite *sqliteh;

  GList *events;
  guint default_calendar;
  GSList *calendars;

  /* Largest UID which we know of.  */
  unsigned long uid;

  /* A list of events that need to be flushed to backing store.  */
  GSList *laundry_list;
  /* The idle source.  */
  guint laundry_buzzer;

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
};


/**
 * struct event_details
 *
 * Detail information for an event.
 */
struct event_details
{
  gchar *summary;
  gchar *description;
  gchar *location;  
  
  time_t modified;

  /* List of integers.  */
  GSList *categories;
};

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

  /* The event calendar to which this event belongs.  */
  guint calendar;

  unsigned long uid;

  unsigned long duration;	/* 0 == instantaneous */
  unsigned long alarm;		/* seconds before event */

  /* Sequence number.  */
  unsigned long sequence;

  struct event_details *details;
  char *eventid;

  /* Recurrence properties.  */

  /* iCal's FREQ property.  */
  enum event_recurrence_type type;

  /* The number of times this recurrence set is expanded.  0 means
     there is no limit.  */
  unsigned int count;
  /* iCal's interval property: the number of units to skip.  If the
     recurrence type is RECUR_YEARLY then the first recurrence occurs
     INCREMENT years after the initial start.  */
  unsigned int increment;
  /* Only meaningful when TYPE is RECUR_WEEKLY: if bit 0 is set then
     occur on Mon, bit 1, Tue, etc.  */
  unsigned long daymask;

  /* A list of start times to exclude.  Must match the start of a
     recurrence exactly.  */
  GSList *exceptions;

  /* No recurrences beyond this time.  0 means forever.  */
  time_t end;

  /* The EventDB to which event belongs.  */
  EventDB *edb;

  gboolean modified;
  gboolean untimed;
};

#define LIVE(ev) (g_assert (! EVENT (ev)->dead))
#define STAMP(ev) \
  do \
    { \
      event_details (ev, TRUE); \
      ev->details->modified = time (NULL); \
      if (! ev->modified) \
        { \
          ev->modified = TRUE; \
          add_to_laundry_pile (G_OBJECT (ev)); \
        } \
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

  edb_class->event_changed_signal
    = g_signal_new ("event-changed",
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
}

static void
event_db_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (event_db_parent_class)->dispose (obj);
}

static gboolean do_laundry (gpointer data);

static void
event_db_finalize (GObject *object)
{
  EventDB *edb = EVENT_DB (object);

  /* Cancel any outstanding timeouts.  */
  if (edb->alarm)
    g_source_remove (edb->alarm);
  if (edb->laundry_buzzer)
    {
      g_source_remove (edb->laundry_buzzer);
      do_laundry (edb);
    }

  {
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
  }

  {
    GList *i;
    GList *next = edb->events;
    while (next)
      {
	i = next;
	next = i->next;

	EventSource *ev = EVENT_SOURCE (i->data);
	g_object_unref (ev);
      }
    g_list_free (edb->events);
  }

  {
    GSList *i;
    GSList *next = edb->calendars;
    while (next)
      {
	i = next;
	next = i->next;

	EventCalendar *ec = EVENT_CALENDAR (i->data);
	g_object_unref (ec);
      }
    g_slist_free (edb->calendars);
  }

  sqlite_close (edb->sqliteh);

  G_OBJECT_CLASS (event_db_parent_class)->finalize (object);
}

static void
event_db_set_alarms_fired_through (EventDB *edb, time_t t)
{
  int err;
  char *str;

  sqlite_exec (edb->sqliteh,
	       "delete from alarms_fired_through", NULL, NULL, NULL);
  err = sqlite_exec_printf (edb->sqliteh,
			    "insert into alarms_fired_through"
			    " (time) values (%d)",
			    NULL, NULL, &str, t);
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

  err = sqlite_exec_printf (ev->edb->sqliteh,
			    "insert into alarms_unacknowledged"
			    " (uid, start) values (%d, %d)",
			    NULL, NULL, &str,
			    ev->uid, ev->event.start);
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

  if (sqlite_exec_printf (ev->edb->sqliteh,
			  "delete from alarms_unacknowledged"
			  " where uid=%d and start=%d",
			  NULL, NULL, &err,
			  ev->uid, ev->event.start))
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
  if (sqlite_exec (edb->sqliteh,
		   "select uid, start from alarms_unacknowledged",
		   callback, NULL, &err))
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
      if (sqlite_exec_printf (edb->sqliteh,
			      "delete from alarms_unacknowledged"
			      " where uid=%d and start=%d",
			      NULL, NULL, &err, r->uid, r->start))
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
  static GType type = 0;

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

static void
event_source_init (GTypeInstance *instance, gpointer klass)
{
}

static void
event_source_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (event_source_parent_class)->dispose (obj);
}

static void event_details (EventSource *ev, gboolean fill_from_disk);
static gboolean event_write (EventSource *, char **);
static void event_db_remove_internal (EventSource *ev);

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
	  g_critical ("event_write: %s", err);
	  free (err);
	}
    }

  /* Free any details.  */
  struct event_details *evd = event->details;
  if (evd)
    {
      if (evd->description)
	g_free (evd->description);
      if (evd->location)
	g_free (evd->location);
      if (evd->summary)
	g_free (evd->summary);
      g_slist_free (evd->categories);
      g_free (evd);
    }

  if (event->eventid)
    g_free (event->eventid);

  g_slist_free (event->exceptions);

  event_db_remove_internal (event);

  G_OBJECT_CLASS (event_parent_class)->finalize (object);
}

/* Add an event to the in-memory list */
static void
event_db_add_internal (EventSource *ev)
{
  if (ev->uid >= ev->edb->uid)
    ev->edb->uid = ev->uid + 1;

  ev->edb->events = g_list_insert_sorted (ev->edb->events, ev,
					  event_compare_func);
}

/* Remove an event from the in-memory list */
static void
event_db_remove_internal (EventSource *ev)
{
  g_assert (g_list_find (ev->edb->events, ev));
  ev->edb->events = g_list_remove (ev->edb->events, ev);
  g_assert (! g_list_find (ev->edb->events, ev));
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

  if ((gethostname (buffer, sizeof (buffer) -1) == 0) && (buffer[0] != 0))
    hostname = buffer;
  else
    hostname = "localhost";

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

static int
dbinfo_callback (void *arg, int argc, char **argv, char **names)
{
  EventDB *edb = EVENT_DB (arg);
  if (argc == 1)
    {
      edb->dbversion = atoi (argv[0]);
    }

  return 0;
}

/**
 * event_db_new:
 *
 * Initialise the event database for use. Needs to be called on application
 * start before acessing events.
 *
 * Returns: EventDB * on success, NULL otherwise.
 */
EventDB *
event_db_new (const char *fname)
{
  EventDB *edb = EVENT_DB (g_object_new (event_db_get_type (), NULL));
  char *err;

  edb->sqliteh = sqlite_open (fname, 0, &err);
  if (edb->sqliteh == NULL)
    goto error;

  /* Get the calendar db version.  */
  sqlite_exec (edb->sqliteh,
	       "create table calendar_dbinfo (version integer NOT NULL)",
	       NULL, NULL, &err);
  if (sqlite_exec (edb->sqliteh, "select version from calendar_dbinfo",
		   dbinfo_callback, edb, &err))
    goto error;

  /* A verion 1 database consists of several tables: calendar_urn and
     calendar.  */
  sqlite_exec (edb->sqliteh,
	       "create table calendar"
	       " (uid integer NOT NULL, tag text, value text)",
	       NULL, NULL, NULL);
  sqlite_exec (edb->sqliteh,
	       "create table calendar_urn (uid INTEGER PRIMARY KEY)",
	       NULL, NULL, NULL);

  /* Read EDB->ALARMS_FIRED_THROUGH.  */
  edb->alarms_fired_through = time (NULL);
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

  /* A table of events whose alarm fired before
     EDB->ALARMS_FIRED_THROUGH but were not yet acknowledged.  */
  sqlite_exec (edb->sqliteh,
	       "create table alarms_unacknowledged"
	       " (uid INTEGER, start INTEGER NOT NULL)",
	       NULL, NULL, NULL);

  /* Calendars.  */
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
  if (edb->default_calendar == EVENT_CALENDAR_NO_PARENT)
    {
      EventCalendar *ec = event_db_get_default_calendar (edb, NULL);
      g_object_unref (ec);
    }

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
      g_warning ("%s: %s", __func__, err);
      g_free (err);
    }

  if (edb->dbversion == 1) 
    {
      int load_data_callback (void *arg, int argc, char **argv,
			      char **names)
	{
	  if (argc == 2)
	    {
	      EventSource *ev = arg;
     
	      if (!strcasecmp (argv[0], "start"))
		{
		  gboolean untimed;

		  parse_date (argv[1], &ev->event.start, &untimed);

		  if (untimed)
		    ev->untimed = TRUE;
		}
	      else if (!strcasecmp (argv[0], "eventid"))
		ev->eventid = g_strdup (argv[1]);
	      else if (!strcasecmp (argv[0], "rend"))
		parse_date (argv[1], &ev->end, NULL);
	      else if (!strcasecmp (argv[0], "rcount"))
		ev->count = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "rincrement"))
		ev->increment = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "rdaymask"))
		ev->daymask = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "rexceptions"))
		{
		  long rmtime = (long)atoi (argv[1]);
		  ev->exceptions = g_slist_append (ev->exceptions,
						   (void *) rmtime);
		}
	      else if (!strcasecmp (argv[0], "recur"))
		ev->type = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "duration"))
		ev->duration = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "alarm"))
		ev->alarm = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "sequence"))
		ev->sequence = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "calendar"))
		ev->calendar = atoi (argv[1]);
	    }

	  return 0;
	}

      int uid_load_callback (void *arg, int argc, char **argv, char **names)
	{
	  EventDB *edb = arg;

	  if (argc == 1)
	    {
	      char *err;
	      guint uid = atoi (argv[0]);
	      EventSource *ev
		= EVENT_SOURCE (g_object_new (event_source_get_type (), NULL));
	      ev->edb = edb;
	      ev->uid = uid;
	      if (sqlite_exec_printf (edb->sqliteh,
				      "select tag,value from calendar where uid=%d", 
				      load_data_callback, ev, &err, uid))
		{
		  gpe_error_box (err);
		  free (err);
		  g_object_unref (ev);
		  return 1;
		}

	      event_db_add_internal (ev);
	      /* EDB holds a reference.  */
	      g_object_ref (ev);
	    }
	  return 0;
	}

      /* Load all the records into memory.  */
      if (sqlite_exec (edb->sqliteh, "select uid from calendar_urn",
		       uid_load_callback, edb, &err))
	goto error;
    }
  else if (edb->dbversion == 0)
    {
      int load_callback0 (void *arg, int argc, char **argv, char **names)
	{
	  EventDB *edb = arg;

	  if (argc == 7)
	    {
	      EventSource *ev
		= EVENT_SOURCE (g_object_new (event_source_get_type (), NULL));

	      ev->edb = edb;
	      ev->uid = atoi (argv[0]);

	      parse_date (argv[1], &ev->event.start, &ev->untimed);

	      ev->duration = argv[2] ? atoi(argv[2]) : 0;
	      ev->alarm = atoi (argv[3]);

	      event_details (ev, FALSE);
	      ev->details->summary = g_strdup (argv[5]);
	      ev->details->description = g_strdup (argv[6]);

	      event_db_add_internal (ev);
	      /* Take a reference for EDB.  */
	      g_object_ref (ev);
	    }

	  return 0;
	}

      sqlite_exec (edb->sqliteh,
		   "select uid, start, duration, alarmtime, recurring, summary, description from events",
		   load_callback0, edb, &err);

      if (sqlite_exec_printf (edb->sqliteh,
			      "insert into calendar_dbinfo (version) values (%d)", 
			      NULL, NULL, &err, 1 /* New version.  */))
	goto error;

      edb->dbversion = 1;
    }
  else
    {
      err = g_strdup (_("Unable to read database file: unknown version."));
      goto error;
    }
    
  return edb;
error:
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
load_details_callback (void *arg, int argc, char *argv[], char **names)
{
  if (argc == 2)
    {
      struct event_details *evd = arg;
      if (!strcasecmp (argv[0], "summary") && !evd->summary)
        evd->summary = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "description") && !evd->description)
        evd->description = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "location") && !evd->location)
        evd->location = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "modified"))
        {
          if (strchr (argv[1], '-'))
            parse_date (argv[1], &evd->modified, NULL);
          else
            evd->modified = strtoul (argv[1], NULL, 10);
        }
      else if (!strcasecmp (argv[0], "category"))
        evd->categories = g_slist_prepend (evd->categories,
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

  ev->details = g_malloc0 (sizeof (struct event_details));

  if (! fill_from_disk)
    return;

  if (sqlite_exec_printf (ev->edb->sqliteh,
			  "select tag,value from calendar where uid=%d",
			  load_details_callback, ev->details, &err, ev->uid))
    {
      gpe_error_box (err);
      free (err);
      g_free (ev->details);
      return;
    }
}

Event *
event_db_find_by_uid (EventDB *edb, guint uid)
{
  GList *iter;
    
  for (iter = edb->events; iter; iter = g_list_next (iter))
    {
      EventSource *ev = iter->data;
      
      if (ev->uid == uid)
	{
	  g_object_ref (ev);
	  return EVENT (ev);
	}
    }

  return NULL;
}

Event *
event_db_find_by_eventid (EventDB *edb, const char *eventid)
{
  GList *iter;
  
  g_return_val_if_fail (eventid, NULL);
  
  for (iter = edb->events; iter; iter = g_list_next (iter))
    {
      EventSource *ev = iter->data;
      
      if (ev->eventid && (strcmp (ev->eventid, eventid) == 0))
        {
          g_object_ref (ev);
          return EVENT (ev);
        }
    }

  return NULL;
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

/* Return the number of days in month MONTH in year YEAR.  */
static guint
days_in_month (guint year, guint month)
{
  static const guint nr_days[] = { 31, 28, 31, 30, 31, 30, 
				   31, 31, 30, 31, 30, 31 };

  g_assert (month <= 11);

  if (month == 1)
    {
      return ((year % 4) == 0
	      && ((year % 100) != 0
		  || (year % 400) == 0)) ? 29 : 28;
    }

  return nr_days[month];
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
      days = days_in_month (tm.tm_year, tm.tm_mon);
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
      if (ev->type == RECUR_WEEKLY && ev->daymask)
	/* This is a weekly recurrence with a day mask: find the
	   first day which, starting with S, occurs in
	   EV->DAYMASK.  */
	{
	  struct tm tm;
	  localtime_r (&recur_start, &tm);

	  int i;
	  for (i = tm.tm_wday; i < tm.tm_wday + 7; i ++)
	    /* EV->DAYMASK is Monday based, not Sunday.  */
	    if ((1 << ((i - 1) % 7)) & ev->daymask)
	      break;
	  if (i != tm.tm_wday)
	    recur_start = time_add_days (recur_start, i - tm.tm_wday);
	}

      /* Cache the representation of S.  */
      struct tm orig;
      localtime_r (&recur_start, &orig);

      GSList *list = NULL;
      int increment = ev->increment > 0 ? ev->increment : 1;
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
	      if (! ev->daymask)
		/* Empty day mask, simply advance S by
		   INCREMENT weeks.  */
		recur_start = time_add_days (recur_start, 7 * increment);
	      else
		{
		  struct tm tm;
		  localtime_r (&recur_start, &tm);
		  int i;
		  for (i = tm.tm_wday + 1; i < tm.tm_wday + 1 + 7; i ++)
		    /* EV->DAYMASK is Monday based, not Sunday.  */
		    if ((1 << ((i - 1) % 7)) & ev->daymask)
		      {
			if ((i % 7) == orig.tm_wday)
			  /* We wrapped a week: increment by
			     INCREMENT - 1 weeks as well.  */
			  recur_start
			    = time_add_days (recur_start,
					     7 * (increment - 1)
					     + i - tm.tm_wday);
			else
			  recur_start
			    = time_add_days (recur_start, i - tm.tm_wday);
			break;
		      }
		}
	      break;

	    case RECUR_MONTHLY:
	      {
		int i;
		struct tm tm;
		for (i = increment; i > 0; i --)
		  {
		    localtime_r (&recur_start, &tm);
		    recur_start
		      = time_add_days (recur_start,
				       days_in_month (tm.tm_year,
						      tm.tm_mon));
		  }
		break;
	      }

	    case RECUR_YEARLY:
	      {
		struct tm tm;
		localtime_r (&recur_start, &tm);
		tm.tm_year += increment;
		if (tm.tm_mon == 1 && tm.tm_mday == 29
		    && days_in_month (tm.tm_year, tm.tm_mon) == 28)
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
			    && days_in_month (tm.tm_year, tm.tm_mon) == 29)
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

Event *
event_db_next_alarm (EventDB *edb, time_t now)
{
  GList *iter;
  GSList *list;
  Event *next = NULL;

  for (iter = edb->events; iter; iter = iter->next)
    {
      list = event_list (iter->data, now, 0, 1, TRUE);
      if (list)
	{
	  Event *ev = list->data;

	  if (! next)
	    next = ev;
	  else if (event_get_start (ev) - event_get_alarm (ev)
		   < event_get_start (next) - event_get_alarm (next))
	    {
	      g_object_unref (next);
	      next = ev;
	    }
	  else
	    g_object_unref (ev);

	  g_slist_free (list);
	}
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
  GList *iter;

  for (iter = edb->events; iter; iter = iter->next)
    {
      EventSource *ev = iter->data;
      LIVE (ev);

      if (only_untimed && ! ev->untimed)
	continue;

      if (period_end < ev->event.start - (alarms ? ev->alarm : 0))
	/* Event starts after PERIOD_END.  */
	{
	  if (alarms)
	    continue;
	  else
	    /* (In which case all subsequent events start after
	       PERIOD_END.)  */
	    break;
	}

      GSList *l = event_list (ev, period_start, period_end, 0, alarms);
      list = g_slist_concat (list, l);
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
  if (sqlite_exec_printf (edb->sqliteh,
			  "begin transaction;"
			  "delete from default_calendar;"
			  "insert into default_calendar"
			  " (default_calendar) values (%d);"
			  "commit transaction;",
			  NULL, NULL, &err, ev->uid))
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
  if (sqlite_exec_printf (ec->edb->sqliteh,
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
			  ec->last_modified))
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
  if (sqlite_exec_printf (ec->edb->sqliteh,
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
			  ec->uid))
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

  /* And the events.  */
  {
    GList *j;
    GList *next = ec->edb->events;
    while (next)
      {
	j = next;
	next = next->next;

	EventSource *ev = EVENT_SOURCE (j->data);
	if (ev->calendar == ec->uid)
	  {
	    if (delete_events)
	      event_remove (EVENT (ev));
	    else
	      event_set_calendar (EVENT (ev), ec);
	  }
      }
  }

  char *err;
  if (sqlite_exec_printf (ec->edb->sqliteh,
			  "delete from calendars where ROWID=%d;",
			  NULL, NULL, &err, ec->uid))
    {
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

#define GET_SET(type, name) \
  GET(type, name) \
  \
  void \
  event_calendar_set_##name (EventCalendar *ec, type name) \
  { \
    if (ec->name == name) \
      return; \
    ec->name = name; \
    ec->modified = TRUE; \
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
  g_return_if_fail (event_calendar_valid_parent (ec, p));

  if (ec->parent)
    g_object_unref (ec->parent);
  ec->parent = p;
  if (p)
    g_object_ref (p);

  ec->parent_uid = p ? p->uid : EVENT_CALENDAR_NO_PARENT;
  ec->modified = TRUE;
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
  ec->modified = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));
}

#define GET_SET_STRING(name) \
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
    ec->modified = TRUE; \
    add_to_laundry_pile (G_OBJECT (ec)); \
  }

GET_SET_STRING(title)
GET_SET_STRING(description)
GET_SET_STRING(url)
GET_SET_STRING(username)
GET_SET_STRING(password)

GET_SET(int, mode)

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

  add_to_laundry_pile (G_OBJECT (ec));
}

GET_SET(int, sync_interval)
GET_SET(time_t, last_pull)
GET_SET(time_t, last_push)
GET(time_t, last_modified)


#define insert_values(db, id, key, format, value)	\
	sqlite_exec_printf (db, "insert into calendar values (%d, '%q', '" format "')", \
			    NULL, NULL, err, id, key, value)

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
  char buf_start[64], buf_end[64];
  struct tm tm;
  gboolean rc = FALSE;
  GSList *iter;

  if (ev->untimed)
    localtime_r (&ev->event.start, &tm);
  else
    gmtime_r (&ev->event.start, &tm);
  strftime (buf_start, sizeof (buf_start), 
	    ev->untimed ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
	    &tm);  

  if (!ev->eventid)
    {
      ev->eventid = event_db_make_eventid();
#ifdef DEBUG        
      printf ("Made new eventid: %s\n", ev->eventid);
#endif
    }
  
  struct event_details *evd = ev->details;
  if (evd)
    {
      if ((evd->summary
	   && insert_values (ev->edb->sqliteh, ev->uid,
			     "summary", "%q", evd->summary))
	  || (evd->description
	      && insert_values (ev->edb->sqliteh, ev->uid,
				"description", "%q", evd->description))
	  || (evd->location
	      && insert_values (ev->edb->sqliteh, ev->uid,
				"location", "%q", evd->location))
	  || insert_values (ev->edb->sqliteh, ev->uid,
			    "modified", "%lu", evd->modified))
	goto exit;

      for (iter = evd->categories; iter; iter = iter->next)
	{
	  if (insert_values (ev->edb->sqliteh, ev->uid,
			     "category", "%d", (int)iter->data))
	    goto exit;
	}
    }

  if (insert_values (ev->edb->sqliteh, ev->uid, "duration", "%d", ev->duration)
      || insert_values (ev->edb->sqliteh, ev->uid, "start", "%q", buf_start)
      || insert_values (ev->edb->sqliteh, ev->uid,
			"eventid", "%q", ev->eventid)
      || insert_values (ev->edb->sqliteh, ev->uid,
			"sequence", "%d", ev->sequence)
      || insert_values (ev->edb->sqliteh, ev->uid,
			"calendar", "%d", ev->calendar))
    goto exit;

  if (insert_values (ev->edb->sqliteh, ev->uid, "recur", "%d", ev->type)
      || insert_values (ev->edb->sqliteh, ev->uid, "rcount", "%d", ev->count)
      || insert_values (ev->edb->sqliteh, ev->uid, "rincrement", "%d",
			ev->increment)
      || insert_values (ev->edb->sqliteh, ev->uid, "rdaymask", "%d",
			ev->daymask))
    goto exit;

  if (ev->exceptions)
    {
      GSList *iter;
      for (iter = ev->exceptions; iter; iter = g_slist_next (iter))
	if (insert_values (ev->edb->sqliteh, ev->uid, "rexceptions", "%d",
			   (long) iter->data))
	  goto exit;
    }
      	      
  if (ev->end != 0)
    {
      gmtime_r (&ev->end, &tm);
      strftime (buf_end, sizeof (buf_end), 
                ev->untimed ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
                &tm); 
      if (insert_values (ev->edb->sqliteh, ev->uid, "rend", "%q", buf_end)) 
	goto exit;
    }


  if (insert_values (ev->edb->sqliteh, ev->uid, "alarm", "%d", ev->alarm))
    goto exit;

  ev->modified = FALSE;
  rc = TRUE;
exit:
  return rc;
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

  if (sqlite_exec (ev->edb->sqliteh, "begin transaction", NULL, NULL, &err))
    goto error;

  if (sqlite_exec_printf (ev->edb->sqliteh,
			  "delete from calendar where uid=%d", NULL,
			  NULL, &err, ev->uid))
    goto error;

  if (event_write (ev, &err) == FALSE
      || sqlite_exec (ev->edb->sqliteh, "commit transaction",
		      NULL, NULL, &err))
    goto error_and_rollback;

  return TRUE;

 error_and_rollback:
  sqlite_exec (ev->edb->sqliteh, "rollback transaction", NULL, NULL, NULL);
 error:
  gpe_error_box (err);
  free (err);
  return FALSE;
}

/* Write all dirty events EDB->LAUNDRY_LIST to backing store.  Called
   by the idle loop and set in add_to_laundry_pile.  */
static gboolean
do_laundry (gpointer data)
{
  EventDB *edb = EVENT_DB (data);
  GSList *l;

  for (l = edb->laundry_list; l; l = g_slist_next (l))
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
	    event_calendar_flush (e);
	  g_signal_emit (edb,
			 EVENT_DB_GET_CLASS (edb)->calendar_changed_signal,
			 0, e);
	  g_object_unref (e);
	}
    }

  /* Destroy the list.  */
  g_slist_free (edb->laundry_list); 
  edb->laundry_list = NULL;

  /* Don't run again.  */
  edb->laundry_buzzer = 0;
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
    g_idle_add (do_laundry, edb);
}

/**
 * event_db_remove:
 * @ev: Event
 * 
 * This function removes an event from the event database. 
 * 
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_remove (Event *event)
{
  if (event->dead)
    return TRUE;

  EventSource *ev = RESOLVE_CLONE (event);

  event_db_remove_internal (ev);

  sqlite_exec_printf (ev->edb->sqliteh, "delete from calendar where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  sqlite_exec_printf (ev->edb->sqliteh,
		      "delete from calendar_urn where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      event_acknowledge (EVENT (ev));
      /* And remove it from the upcoming alarm list.  */
      event_remove_upcoming_alarms (ev);
    }

  EVENT (ev)->dead = TRUE;

  return TRUE;
}

/**
 * event_new:
 *
 * Create and initialise a new #Event event structure with the event
 * id EVENTID.  If EVENTID is NULL, one is fabricated.
 *
 * Returns: A new event.
 */
Event *
event_new (EventDB *edb, const char *eventid)
{
  EventSource *ev = EVENT_SOURCE (g_object_new (event_source_get_type (),
						NULL));
  gchar *err = NULL;

  ev->edb = edb;

  if (sqlite_exec (edb->sqliteh, "begin transaction", NULL, NULL, &err))
    goto error_no_roll_back;

  if (sqlite_exec (edb->sqliteh, "insert into calendar_urn values (NULL)",
		   NULL, NULL, &err))
    goto error;

  ev->uid = sqlite_last_insert_rowid (edb->sqliteh);
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

  event_db_add_internal (ev);
  /* Take a reference for EDB.  */
  g_object_ref (ev);

  if (event_write (ev, &err) == FALSE
      || sqlite_exec (edb->sqliteh, "commit transaction", NULL, NULL, &err))
    goto error;

  return EVENT (ev);

 error:
  g_object_unref (ev);
  sqlite_exec (edb->sqliteh, "rollback transaction", NULL, NULL, NULL);
 error_no_roll_back:
  gpe_error_box (err);
  g_free (err);

  return NULL;
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
		 __func__, ev->details ? ev->details->summary : "", ev->uid);
      
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

  STAMP (ev);

  ev->duration = duration;
}

#undef GET
#define GET(type, name, field) \
  type \
  event_get_##name (Event *event) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    return ev->field; \
  }

#undef GET_SET
#define GET_SET(type, name, field, alarm_hazard) \
  GET (type, name, field) \
  \
  void \
  event_set_##name (Event *event, type value) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    if (ev->field == value) \
      return; \
    \
    STAMP (ev); \
    if ((alarm_hazard) && ev->alarm) \
      { \
        event_acknowledge (EVENT (ev)); \
        event_remove_upcoming_alarms (ev); \
      } \
    ev->field = value; \
    if ((alarm_hazard) && ev->alarm) \
      event_add_upcoming_alarms (ev); \
  }

GET_SET (unsigned long, alarm, alarm, TRUE)
GET_SET (guint32, sequence, sequence, FALSE)
GET_SET (enum event_recurrence_type, recurrence_type, type, TRUE)
GET (time_t, recurrence_start, event.start)

void
event_set_recurrence_start (Event *event, time_t start)
{
  EventSource *ev = RESOLVE_CLONE (event);
  STAMP (ev);

  if (ev->event.start == start)
    return;

  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      event_acknowledge (EVENT (ev));
      /* And remove it from the upcoming alarm list.  */
      event_remove_upcoming_alarms (ev);
    }
  event_db_remove_internal (ev);
  ev->event.start = start;
  event_db_add_internal (ev);
  if (ev->alarm)
    /* And remove it from the upcoming alarm list.  */
    event_add_upcoming_alarms (ev);
}

GET_SET (time_t, recurrence_end, end, TRUE)
GET_SET (guint32, recurrence_count, count, TRUE)
GET_SET (guint32, recurrence_increment, increment, TRUE)
GET_SET (guint64, recurrence_daymask, daymask, TRUE)

GET_SET (gboolean, untimed, untimed, FALSE);

GET (unsigned long, uid, uid)

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
    return g_strdup (ev->details->field); \
  } \
 \
  void \
  event_set_##field (Event *event, const char *field) \
  { \
    EventSource *ev = RESOLVE_CLONE (event); \
    \
    event_details (ev, TRUE); \
    if ((ev->details->field && field \
         && strcmp (ev->details->field, field) == 0) \
        || (! ev->details->field && ! field)) \
      /* Identical.  */ \
      return; \
    \
    STAMP (ev); \
    if (ev->details->field) \
      free (ev->details->field); \
    if (field) \
      ev->details->field = g_strdup (field); \
    else \
      ev->details->field = NULL; \
  }

GET_SET_STRING(summary)
GET_SET_STRING(location)
GET_SET_STRING(description)

GSList *
event_get_categories (Event *event)
{
  EventSource *ev = RESOLVE_CLONE (event);
  event_details (ev, TRUE);
  return g_slist_copy (ev->details->categories);
}

void
event_add_category (Event *event, int category)
{
  EventSource *ev = RESOLVE_CLONE (event);
  STAMP (ev);
  event_details (ev, TRUE);
  ev->details->categories = g_slist_prepend (ev->details->categories,
					     (gpointer) category);
}

void
event_set_categories (Event *event, GSList *categories)
{
  EventSource *ev = RESOLVE_CLONE (event);
  STAMP (ev);

  event_details (ev, TRUE);
  g_slist_free (ev->details->categories);
  ev->details->categories = categories;
}

void
event_add_recurrence_exception (Event *event, time_t start)
{
  EventSource *ev = RESOLVE_CLONE (event);
  STAMP (ev);

  ev->exceptions = g_slist_append (ev->exceptions, (void *) start);
}
