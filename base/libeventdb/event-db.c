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

#define EVENT_DB_USE_MEMCHUNK
#ifdef EVENT_DB_USE_MEMCHUNK
static GMemChunk *recur_chunk;
#define event_db__alloc_recur()		\
	(recur_t)g_mem_chunk_alloc0 (recur_chunk)
#define event_db__free_recur(_x)	\
	g_mem_chunk_free (recur_chunk, _x)
#else
#define event_db__alloc_recur()		\
	(recur_t)g_malloc0 (sizeof (struct recur_s))
#define event_db__free_recur(_x)	\
	g_free (_x)
#endif

typedef struct
{
  GObjectClass gobject_class;
  GObjectClass parent_class;

  /* Signals.  */
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
  void *sqliteh;

  GList *events;

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
  
  unsigned long sequence;
  time_t modified;

  /* List of integers.  */
  GSList *categories;
};

struct _EventClass
{
  GObjectClass gobject_class;
  GObjectClass parent_class;
};

/**
 * event_t:
 *
 * This data type holds all basic information of an event. More detailed 
 * event information are covered by the 'details' and 'recur' members.
 */
struct _Event
{
  GObject object;

  unsigned long uid;

  time_t start;
  unsigned long duration;	/* 0 == instantaneous */
  unsigned long alarm;		/* seconds before event */

  recur_t recur;
  
  struct event_details *details;
  struct _Event *clone_source;
  char *eventid;

  /* The EventDB to which event belongs.  */
  EventDB *edb;

  gboolean dead;
  gboolean modified;
  gboolean untimed;
};

#define LIVE(ev) (g_assert (! ev->dead))
#define STAMP(ev) \
  do \
    { \
      event_details (ev, TRUE); \
      ev->details->modified = time (NULL); \
      if (! ev->modified) \
        { \
          ev->modified = TRUE; \
          g_object_ref (ev); \
          add_to_laundry_pile (ev); \
        } \
    } \
  while (0)
#define NO_CLONE(ev) g_return_if_fail (! ev->clone_source)
#define RESOLVE_CLONE(ev) \
  ({ \
    Event *_e = ev; \
    while (_e->clone_source) \
      _e = _e->clone_source; \
    _e; \
   })

static void event_db_class_init (gpointer klass, gpointer klass_data);
static void event_db_init (GTypeInstance *instance, gpointer klass);
static void event_db_dispose (GObject *obj);
static void event_db_finalize (GObject *object);

static GObjectClass *event_db_parent_class;

GType
event_db_get_type (void)
{
  static GType type = 0;

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
  EventDB *edb = EVENT_DB (instance);
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

	Event *ev = EVENT (i->data);

	if (ev->clone_source)
	  g_critical ("edb->events contains cloned event "
		      "(i.e. events still reference database)!");
	else
	  g_object_unref (ev);
      }
    g_list_free (edb->events);
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
event_mark_unacknowledged (Event *ev)
{
  int err;
  char *str;

  err = sqlite_exec_printf (ev->edb->sqliteh,
			    "insert into alarms_unacknowledged"
			    " (uid, start) values (%d, %d)",
			    NULL, NULL, &str,
			    event_get_uid (ev), event_get_start (ev));
  if (err)
    {
      g_critical ("%s: %s", __func__, str);
      g_free (str);
    }
}

/* Acknowledge that event EV has fired.  If EV has no alarm, has not
   yet fired or already been acknowledged, does nothing.  */
void
event_acknowledge (Event *ev)
{
  char *err;

  if (sqlite_exec_printf (ev->edb->sqliteh,
			  "delete from alarms_unacknowledged"
			  " where uid=%d and start=%d",
			  NULL, NULL, &err,
			  event_get_uid (ev), event_get_start (ev)))
    {
      g_warning ("%s: removing event %ld from unacknowledged list: %s",
		 __func__, event_get_uid (ev), err);
      g_free (err);
    }
}

/* Remove any instantiations of EV's source from the upcoming alarm
   list.  */
static void
event_remove_upcoming_alarms (Event *ev)
{
  ev = RESOLVE_CLONE (ev);

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

static GSList *event_list (Event *ev, time_t period_start, time_t period_end,
			   int max, gboolean per_alarm);
static gboolean buzzer (gpointer data);

/* EV is new or has recently changed: check to see if it has an alarm
   which will go off in the near future.  */
static void
event_add_upcoming_alarms (Event *ev)
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
	  event_mark_unacknowledged (EVENT (i->data));

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

      Event *ev = event_db_find_by_uid (edb, uid);
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
  event->modified = FALSE;
}

static void
event_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (event_parent_class)->dispose (obj);
}

static void event_details (Event *ev, gboolean fill_from_disk);
static gboolean event_write (Event *, char **);
static void event_db_remove_internal (Event *ev);

static void
event_finalize (GObject *object)
{
  Event *event = EVENT (object);

  LIVE (event);

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

  if (event->recur)
    event_db__free_recur (event->recur);
  if (event->eventid)
    g_free (event->eventid);

  if (event->clone_source)
    g_object_unref (event->clone_source);

  event_db_remove_internal (event);

  event->dead = TRUE;

  G_OBJECT_CLASS (event_parent_class)->finalize (object);
}


static gint
event_sort_func (const void *a, const void *b)
{
  Event *ev1 = EVENT (a);
  Event *ev2 = EVENT (b);

  return ev1->start - ev2->start;
}

/* Add an event to the in-memory list */
static void
event_db_add_internal (Event *ev)
{
  g_return_if_fail (! ev->clone_source);

  if (ev->uid >= ev->edb->uid)
    ev->edb->uid = ev->uid + 1;

  ev->edb->events = g_list_insert_sorted (ev->edb->events, ev,
					  event_sort_func); 
}

/* Remove an event from the in-memory list */
static void
event_db_remove_internal (Event *ev)
{
  NO_CLONE (ev);

  g_assert (g_list_find (ev->edb->events, ev));
  ev->edb->events = g_list_remove (ev->edb->events, ev);
  g_assert (! g_list_find (ev->edb->events, ev));
}

/* Here we create a globally unique eventid, which we
 * can use to reference this event in a vcal, etc. */
static gchar *
event_db_make_eventid (void)
{
  static char *hostname;
  static char buffer [512];

  if ((gethostname (buffer, sizeof (buffer) -1) == 0) &&
     (buffer [0] != 0))
    hostname = buffer;
  else
    hostname = "localhost";

  return g_strdup_printf ("%lu.%lu@%s",
                         (unsigned long) time (NULL),
                         (unsigned long) getpid(),
                         hostname); 
}

static gboolean
parse_date (char *s, time_t *t, gboolean *date_only)
{
  struct tm tm;
  char *p;

  memset (&tm, 0, sizeof (tm));
  p = strptime (s, "%Y-%m-%d", &tm);
  if (p == NULL)
    {
      fprintf (stderr, "Unable to parse date: %s\n", s);
      return FALSE;
    }

  p = strptime (p, " %H:%M", &tm);

  if (date_only)
    *date_only = (p == NULL) ? TRUE : FALSE;

  *t = timegm (&tm);
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

#ifdef EVENT_DB_USE_MEMCHUNK
  if (! recur_chunk)
    recur_chunk = g_mem_chunk_new ("recur", sizeof (struct recur_s), 4096,
				   G_ALLOC_AND_FREE);
#endif

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
	       NULL, NULL, &err);
  sqlite_exec (edb->sqliteh,
	       "create table calendar_urn (uid INTEGER PRIMARY KEY)",
	       NULL, NULL, &err);

  /* Read EDB->ALARMS_FIRED_THROUGH.  */
  edb->alarms_fired_through = time (NULL);
  sqlite_exec (edb->sqliteh,
	       "create table alarms_fired_through (time INTEGER)",
	       NULL, NULL, &err);

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
  sqlite_exec (edb->sqliteh, "select time from alarms_fired_through",
	       alarms_fired_through_callback, edb, &err);

  /* A table of events whose alarm fired before
     EDB->ALARMS_FIRED_THROUGH but were not yet acknowledged.  */
  sqlite_exec (edb->sqliteh,
	       "create table alarms_unacknowledged"
	       " (uid INTEGER, start INTEGER NOT NULL)",
	       NULL, NULL, &err);

  if (edb->dbversion == 1) 
    {
      int load_data_callback (void *arg, int argc, char **argv,
			      char **names)
	{
	  if (argc == 2)
	    {
	      Event *ev = arg;
     
	      if (!strcasecmp (argv[0], "start"))
		{
		  gboolean untimed;

		  parse_date (argv[1], &ev->start, &untimed);

		  if (untimed)
		    {
		      ev->untimed = TRUE;
		      ev->start += 12 * 60 * 60;
		    }
		}
	      else if (!strcasecmp (argv[0], "eventid"))
		ev->eventid = g_strdup (argv[1]);
	      else if (!strcasecmp (argv[0], "rend"))
		{
		  recur_t r = event_get_recurrence (ev);
		  parse_date (argv[1], &r->end, NULL);
		}
	      else if (!strcasecmp (argv[0], "rcount"))
		{
		  recur_t r = event_get_recurrence (ev);
		  r->count = atoi (argv[1]);
		}
	      else if (!strcasecmp (argv[0], "rincrement"))
		{
		  recur_t r = event_get_recurrence (ev);
		  r->increment = atoi (argv[1]);
		}
	      else if (!strcasecmp (argv[0], "rdaymask"))
		{
		  recur_t r = event_get_recurrence (ev);
		  r->daymask = atoi (argv[1]);
		}
	      else if (!strcasecmp (argv[0], "rexceptions"))
		{
		  recur_t r = event_get_recurrence (ev);
		  long rmtime = (long)atoi (argv[1]);
		  r->exceptions = g_slist_append(r->exceptions,
						 (void *) rmtime);
		}
	      else if (!strcasecmp (argv[0], "recur"))
		{
		  recur_t r = event_get_recurrence (ev);
		  r->type = atoi (argv[1]);
		}
	      else if (!strcasecmp (argv[0], "duration"))
		ev->duration = atoi (argv[1]);
	      else if (!strcasecmp (argv[0], "alarm"))
		ev->alarm = atoi (argv[1]);
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
	      Event *ev = EVENT (g_object_new (event_get_type (), NULL));
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

	      if (ev->recur && ev->recur->type == RECUR_NONE)
		{
		  /* Old versions of gpe-calendar dumped out a load of
		     recurrence tags even for a one-shot event.  */
		  event_db__free_recur (ev->recur);
		  ev->recur = NULL;
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
	      Event *ev = EVENT (g_object_new (event_get_type (), NULL));

	      ev->edb = edb;
	      ev->uid = atoi (argv[0]);

	      parse_date (argv[1], &ev->start, &ev->untimed);

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
      else if (!strcasecmp (argv[0], "sequence"))
        evd->sequence = atoi (argv[1]);
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
event_details (Event *ev, gboolean fill_from_disk)
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
      Event *ev = iter->data;
      
      if (ev->uid == uid)
	{
	  g_object_ref (ev);
	  return ev;
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
event_clone (Event *ev)
{
  Event *n = EVENT (g_object_new (event_get_type (), NULL));

  memcpy (n, ev, sizeof (struct _Event));
  n->clone_source = (void *)ev;
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
event_list (Event *ev, time_t period_start, time_t period_end, int max,
	    gboolean per_alarm)
{
  int event_count = 0;

  if (per_alarm && ! ev->alarm)
    /* We are looking for alarms but this event doesn't have
       one.  */
    return NULL;

  recur_t r = ev->recur;
  /* End of recurrence period.  */
  time_t recur_end;
  if (event_is_recurrence (ev))
    {
      if (! r->end)
	/* Never ends.  */
	recur_end = 0;
      else
	recur_end = r->end;
    }
  else
    recur_end = ev->start + ev->duration;

  if (recur_end && recur_end < period_start)
    /* Event finishes prior to PERIOD_START.  */
    return NULL;

  /* Start of first instance.  */
  time_t recur_start = ev->start;
      
  if (period_end && period_end < recur_start - (per_alarm ? ev->alarm : 0))
    /* Event starts after PERIOD_END.  */
    return NULL;

  if (event_is_recurrence (ev))
    {
      if (r->type == RECUR_WEEKLY && r->daymask)
	/* This is a weekly recurrence with a day mask: find the
	   first day which, starting with S, occurs in
	   R->DAYMASK.  */
	{
	  struct tm tm;
	  localtime_r (&recur_start, &tm);

	  int i;
	  for (i = tm.tm_wday; i < tm.tm_wday + 7; i ++)
	    /* R->DAYMASK is Monday based, not Sunday.  */
	    if ((1 << ((i - 1) % 7)) & r->daymask)
	      break;
	  if (i != tm.tm_wday)
	    recur_start = time_add_days (recur_start, i - tm.tm_wday);
	}

      /* Cache the representation of S.  */
      struct tm orig;
      localtime_r (&recur_start, &orig);

      GSList *list = NULL;
      int increment = r->increment > 0 ? r->increment : 1;
      int count;
      for (count = 0;
	   (! period_end
	    || recur_start - (per_alarm ? ev->alarm : 0) <= period_end)
	     && (! recur_end || recur_start <= recur_end)
	     && (r->count == 0 || count < r->count);
	   count ++)
	{
	  if ((per_alarm && period_start <= recur_start - ev->alarm)
	      || (! per_alarm && period_start <= recur_start + ev->duration))
	    /* This instance occurs during the period.  Add
	       it to LIST...  */
	    {
	      GSList *i;

	      /* ... unless there happens to be an exception.  */
	      for (i = r->exceptions; i; i = g_slist_next (i))
		if ((long) i->data == recur_start)
		  break;

	      if (! i)
		/* No exception found: instantiate this recurrence
		   and add it to LIST.  */
		{
		  Event *clone = event_clone (ev);
		  clone->start = recur_start;
		  list = g_slist_insert_sorted (list, clone,
						event_sort_func);

		  event_count ++;
		  if (event_count == max)
		    break;
		}
	    }

	  /* Advance to the next recurrence.  */
	  switch (r->type)
	    {
	    case RECUR_DAILY:
	      /* Advance S by INCREMENT days.  */
	      recur_start = time_add_days (recur_start, increment);
	      break;

	    case RECUR_WEEKLY:
	      if (! r->daymask)
		/* Empty day mask, simply advance S by
		   INCREMENT weeks.  */
		recur_start = time_add_days (recur_start, 7 * increment);
	      else
		{
		  struct tm tm;
		  localtime_r (&recur_start, &tm);
		  int i;
		  for (i = tm.tm_wday + 1; i < tm.tm_wday + 1 + 7; i ++)
		    /* R->DAYMASK is Monday based, not Sunday.  */
		    if ((1 << ((i - 1) % 7)) & r->daymask)
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
			  ev->eventid, r->type);
	      break;
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
      Event *ev = EVENT (iter->data);
      list = event_list (ev, now, 0, 1, TRUE);
      if (list)
	{
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
      Event *ev = iter->data;
      LIVE (ev);

      if (only_untimed && ! ev->untimed)
	continue;

      if (period_end < ev->start - (alarms ? ev->alarm : 0))
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

#define insert_values(db, id, key, format, value)	\
	sqlite_exec_printf (db, "insert into calendar values (%d, '%q', '" format "')", \
			    NULL, NULL, err, id, key, value)

/* Dump an event to the SQL database.  */
static gboolean
event_write (Event *ev, char **err)
{
  char buf_start[64], buf_end[64];
  struct tm tm;
  gboolean rc = FALSE;
  GSList *iter;

  gmtime_r (&ev->start, &tm);
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
			    "modified", "%lu", evd->modified)
	  || insert_values (ev->edb->sqliteh, ev->uid,
			    "sequence", "%d", evd->sequence))
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
      || insert_values (ev->edb->sqliteh, ev->uid, "eventid", "%q", ev->eventid))
    goto exit;

  recur_t r = ev->recur;
  if (r)
    {
      if (insert_values (ev->edb->sqliteh, ev->uid, "recur", "%d", r->type)
	  || insert_values (ev->edb->sqliteh, ev->uid, "rcount", "%d", r->count)
	  || insert_values (ev->edb->sqliteh, ev->uid, "rincrement", "%d", r->increment)
	  || insert_values (ev->edb->sqliteh, ev->uid, "rdaymask", "%d", r->daymask))
        goto exit;

      if (ev->recur->exceptions)
	{
	  GSList *iter;
	  for (iter = ev->recur->exceptions; iter; iter = g_slist_next (iter))
	    if (insert_values (ev->edb->sqliteh, ev->uid, "rexceptions", "%d",
			       (long) iter->data)) goto exit;
	}
      	      
      if (r->end != 0)
        {
          gmtime_r (&r->end, &tm);
          strftime (buf_end, sizeof (buf_end), 
                ev->untimed ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
                &tm); 
          if (insert_values (ev->edb->sqliteh, ev->uid, "rend", "%q", buf_end)) 
            goto exit;
        }
    }
  else
    {
      if (insert_values (ev->edb->sqliteh, ev->uid, "recur", "%d", RECUR_NONE))
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
event_flush (Event *ev)
{
  LIVE (ev);
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
      Event *e = EVENT (l->data);
      if (e->modified)
	event_flush (e);
      g_object_unref (e);
    }

  /* Destroy the list.  */
  g_slist_free (edb->laundry_list); 
  edb->laundry_list = NULL;

  /* Don't run again.  */
  edb->laundry_buzzer = 0;
  return FALSE;
}

/* EV is dirty (i.e. needs to be written to disk) but do it when we
   are idle.  */
static void
add_to_laundry_pile (Event *ev)
{
  ev->edb->laundry_list
    = g_slist_prepend (ev->edb->laundry_list, ev);
  if (! ev->edb->laundry_buzzer)
    g_idle_add (do_laundry, ev->edb);
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
event_remove (Event *ev)
{
  ev = RESOLVE_CLONE (ev);

  event_db_remove_internal (ev);

  sqlite_exec_printf (ev->edb->sqliteh, "delete from calendar where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  sqlite_exec_printf (ev->edb->sqliteh,
		      "delete from calendar_urn where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      event_acknowledge (ev);
      /* And remove it from the upcoming alarm list.  */
      event_remove_upcoming_alarms (ev);
    }

  ev->dead = TRUE;

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
  Event *ev = EVENT (g_object_new (event_get_type (), NULL));
  gchar *err = NULL;

  ev->edb = edb;

  if (sqlite_exec (edb->sqliteh, "begin transaction", NULL, NULL, &err))
    goto error_no_roll_back;

  if (sqlite_exec (edb->sqliteh, "insert into calendar_urn values (NULL)",
		   NULL, NULL, &err))
    goto error;

  ev->uid = sqlite_last_insert_rowid (edb->sqliteh);
  if (eventid)
    ev->eventid = g_strdup (eventid);
  else
    ev->eventid = event_db_make_eventid ();

  event_db_add_internal (ev);
  /* Take a reference for EDB.  */
  g_object_ref (ev);

  if (event_write (ev, &err) == FALSE
      || sqlite_exec (edb->sqliteh, "commit transaction", NULL, NULL, &err))
    goto error;

  return ev;

 error:
  g_object_unref (ev);
  sqlite_exec (edb->sqliteh, "rollback transaction", NULL, NULL, NULL);
 error_no_roll_back:
  gpe_error_box (err);
  g_free (err);

  return NULL;
}

/**
 * event_get_recurrence:
 * @ev: Event
 * 
 * Retrieves recurrence information for an event.
 *
 * Returns: Recurrence information for the given event.
 */
recur_t
event_get_recurrence (Event *ev)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);

  if (ev->recur)
    return ev->recur;

  ev->recur = event_db__alloc_recur ();
  ev->recur->type = RECUR_NONE;
  return ev->recur;
}

gboolean
event_is_recurrence (Event *ev)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  if (ev->recur == NULL)
    return FALSE;

  return !(ev->recur->type == RECUR_NONE);
}

void
event_clear_recurrence (Event *ev)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  STAMP (ev);

  if (ev->recur == NULL)
    return;

  event_db__free_recur (ev->recur);
  ev->recur = NULL;
}

time_t
event_get_start (Event *ev)
{
  LIVE (ev);
  /* This is local, don't resolve the clone.  */
  return ev->start;
}

void
event_set_start (Event *ev, time_t start)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  STAMP (ev);

  if (ev->start == start)
    return;

  if (ev->alarm)
    {
      /* If the event was unacknowledged, acknowledge it now.  */
      event_acknowledge (ev);
      /* And remove it from the upcoming alarm list.  */
      event_remove_upcoming_alarms (ev);
    }
  event_db_remove_internal (ev);
  ev->start = start;
  event_db_add_internal (ev);
  if (ev->alarm)
    /* And remove it from the upcoming alarm list.  */
    event_add_upcoming_alarms (ev);
}

#define GET(type, name, field) \
  type \
  event_get_##name (Event *ev) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    return ev->field; \
  } \

#define GET_SET(type, name, field, alarm_hazard) \
  GET (type, name, field) \
  \
  void \
  event_set_##name (Event *ev, type value) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    STAMP (ev); \
    if ((alarm_hazard) && ev->alarm) \
      { \
        event_acknowledge (ev); \
        event_remove_upcoming_alarms (ev); \
      } \
    ev->field = value; \
    if ((alarm_hazard) && ev->alarm) \
      event_add_upcoming_alarms (ev); \
  }

GET_SET (unsigned long, duration, duration, FALSE)
GET_SET (unsigned long, alarm, alarm, TRUE)
GET_SET (enum event_recurrence_type, recurrence_type, recur->type, FALSE)
GET_SET (time_t, recurrence_start, start, TRUE)
GET_SET (time_t, recurrence_end, recur->end, TRUE)

GET_SET (gboolean, untimed, untimed, FALSE);

GET (unsigned long, uid, uid)
GET (const char *, eventid, eventid)

#define GET_SET_STRING(field) \
  const char * \
  event_get_##field (Event *ev) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    event_details (ev, TRUE); \
    return ev->details->field; \
  } \
 \
  void \
  event_set_##field (Event *ev, const char *field) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    STAMP (ev); \
    event_details (ev, TRUE); \
    if (ev->details->field) \
      free (ev->details->field); \
    ev->details->field = g_strdup (field); \
  }

GET_SET_STRING(summary)
GET_SET_STRING(location)
GET_SET_STRING(description)

const GSList *
event_get_categories (Event *ev)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  event_details (ev, TRUE);
  return ev->details->categories;
}

void
event_add_category (Event *ev, int category)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  STAMP (ev);
  event_details (ev, TRUE);
  ev->details->categories = g_slist_prepend (ev->details->categories,
					     (gpointer) category);
}

void
event_set_categories (Event *ev, GSList *categories)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  STAMP (ev);
  event_details (ev, TRUE);
  g_slist_free (ev->details->categories);
  ev->details->categories = categories;
}

void
event_add_exception (Event *ev, time_t start)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  STAMP (ev);
  g_return_if_fail (ev->recur);

  ev->recur->exceptions = g_slist_append (ev->recur->exceptions,
					  (void *) start);
}
