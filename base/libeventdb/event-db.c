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
extern GMemChunk *recur_chunk;
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

/**
 * *event_details_t:
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
typedef struct event_details *event_details_t;

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

#define FLAG_UNTIMED   (1 << 0)
  unsigned long flags;

  recur_t recur;
  
  struct event_details *details;
  struct _Event *clone_source;
  char *eventid;

  /* The EventDB to which event belongs.  */
  EventDB *edb;

  gboolean dead;
  gboolean modified;
};

#define LIVE(ev) (g_assert (! ev->dead))
#define STAMP(ev) \
  do \
    { \
      event_details (ev, TRUE); \
      ev->details->modified = time (NULL); \
      ev->modified = TRUE; \
    } \
  while (0)
#define NO_CLONE(ev) g_return_if_fail (! ev->clone_source)
#define RESOLVE_CLONE(ev) \
  ({ \
    Event *e = ev; \
    while (e->clone_source) \
      e = e->clone_source; \
    e; \
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

static void
event_db_finalize (GObject *object)
{
  EventDB *edb = EVENT_DB (object);
  GList *iter;

  for (iter = edb->one_shot_events; iter; iter = g_list_next (iter))
    {
      Event *ev = EVENT (iter->data);

      if (ev->clone_source)
	g_critical ("edb->one_shot_events contains cloned event "
		    "(i.e. events still reference database)!");
      else
	g_object_unref (ev);
    }

  for (iter = edb->recurring_events; iter; iter = g_list_next (iter))
    {
      Event *ev = EVENT (iter->data);

      if (ev->clone_source)
	g_critical ("edb->recurring_events contains cloned event "
		    "(i.e. events still reference database)!");
      else
	g_object_unref (ev);
    }

  sqlite_close (edb->sqliteh);

  G_OBJECT_CLASS (event_db_parent_class)->finalize (object);
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
	  g_critical ("event_write: %s\n", err);
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


static const char *schema_str = 
"create table calendar (uid integer NOT NULL, tag text, value text)";
static const char *schema2_str = 
"create table calendar_urn (uid INTEGER PRIMARY KEY)";
static const char *schema_info = 
"create table calendar_dbinfo (version integer NOT NULL)";

#ifdef EVENT_DB_USE_MEMCHUNK
static GMemChunk *recur_chunk;
#endif

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
  GList **list;

  g_return_if_fail (! ev->clone_source);

  if (ev->uid >= ev->edb->uid)
    ev->edb->uid = ev->uid + 1;

  list = event_is_recurrence (ev)
    ? &ev->edb->recurring_events : &ev->edb->one_shot_events;
  *list = g_list_insert_sorted (*list, ev, event_sort_func); 
}

/* Remove an event from the in-memory list */
static void
event_db_remove_internal (Event *ev)
{
  GList **list;

  NO_CLONE (ev);

  list = event_is_recurrence (ev)
    ? &ev->edb->recurring_events : &ev->edb->one_shot_events;
  g_assert (g_list_find (*list, ev));
  *list = g_list_remove (*list, ev);

  g_assert (! g_list_find (ev->edb->one_shot_events, ev));
  g_assert (! g_list_find (ev->edb->recurring_events, ev));
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
load_data_callback (void *arg, int argc, char **argv, char **names)
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
	      ev->flags |= FLAG_UNTIMED;
	      ev->start += 12 * 60 * 60;
	    }
	}
      else if (!strcasecmp (argv[0], "eventid"))
	{
	  ev->eventid = g_strdup (argv[1]);
	}
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
	  r->exceptions = g_slist_append(r->exceptions,	(void *)rmtime);
	}
      else if (!strcasecmp (argv[0], "recur"))
	{
	  recur_t r = event_get_recurrence (ev);
	  r->type = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "duration"))
	{
	  ev->duration = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "alarm"))
	{
	  ev->alarm = atoi (argv[1]);
	}
    }

  return 0;
}

static int
load_callback (void *arg, int argc, char **argv, char **names)
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
	  /* Old versions of gpe-calendar dumped out a load of recurrence tags
	     even for a one-shot event.  */
	  event_db__free_recur (ev->recur);
	  ev->recur = NULL;
	}

      event_db_add_internal (ev);
      /* EDB holds a reference.  */
      g_object_ref (ev);
    }
  return 0;
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

  sqlite_exec (edb->sqliteh, schema_info, NULL, NULL, &err);
  
  if (sqlite_exec (edb->sqliteh, "select version from calendar_dbinfo",
		   dbinfo_callback, edb, &err))
    goto error;

  sqlite_exec (edb->sqliteh, schema_str, NULL, NULL, &err);
  sqlite_exec (edb->sqliteh, schema2_str, NULL, NULL, &err);
      
  if (edb->dbversion == 1) 
    {
      /* Load all the records into memory.  */
      if (sqlite_exec (edb->sqliteh, "select uid from calendar_urn",
		       load_callback, edb, &err))
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
	      char *p;
	      struct tm tm;

	      ev->edb = edb;
	      ev->flags = 0;
	      ev->uid = atoi (argv[0]);
	      event_details (ev, FALSE);

	      memset (&tm, 0, sizeof (tm));
	      p = strptime (argv[1], "%Y-%m-%d", &tm);
	      if (p == NULL)
		{
		  fprintf (stderr, "Unable to parse date: %s\n", argv[1]);
		  return 1;
		}
	      p = strptime (p, " %H:%M", &tm);
	      if (p == NULL)
		ev->flags |= FLAG_UNTIMED;
      
	      ev->start = timegm (&tm);
	      ev->duration = argv[2] ? atoi(argv[2]) : 0;
	      ev->alarm = atoi (argv[3]);

	      ev->details->summary = g_strdup (argv[5]);
	      ev->details->description = g_strdup (argv[6]);

	      event_db_add_internal (ev);
	      /* Take a reference for EDB.  */
	      g_object_ref (ev);
	    }

	  return 0;
	}

      if (sqlite_exec (edb->sqliteh,
		       "select uid, start, duration, alarmtime, recurring, summary, description from events",
		       load_callback0, edb, &err))
	goto error;

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
    
  for (iter = edb->one_shot_events; iter; iter = g_list_next (iter))
    {
      Event *ev = iter->data;
      
      if (ev->uid == uid)
	{
	  g_object_ref (ev);
	  return ev;
	}
    }

  for (iter = edb->recurring_events; iter; iter = g_list_next (iter))
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

static GSList *
event_db_list_for_period_internal (EventDB *edb, time_t start, time_t end,
				   gboolean untimed, 
				   gboolean untimed_significant,
				   gboolean alarms, 
				   guint max)
{
  GList *iter;
  GSList *iter2;
  GSList *list = NULL;
  struct tm tm_event, tm_display;
  long int month_diff;
  int events = 0;
	  
  if (end == 0)		/* ??? pb */
    {
      struct tm tm_current;

      localtime_r (&start, &tm_current);
  
      tm_current.tm_year+=1;
      end = mktime (&tm_current);
    }
  
  for (iter = edb->one_shot_events; iter; iter = g_list_next (iter))
    {
      Event *ev = iter->data;
      time_t event_start = ev->start;

      LIVE (ev);
      
      if (untimed_significant)
	{
	  /* Ignore events with wrong untimed status */
	  if (untimed != ((ev->flags & FLAG_UNTIMED) != 0))
	    continue;
	}

      /* Stop if event hasn't started yet */
      if (end && event_start > end)
	break;
      
      /* Skip events without alarms when applicable */
      if (alarms && !ev->alarm)
	continue;

      /* Skip events that have finished already */
      if ((event_start + ev->duration < start)
	  || (ev->duration && ((event_start + ev->duration == start))))
	continue;

      if (alarms)
	{
	  ev = event_clone (ev);
	  ev->start -= ev->alarm;
	}
      else
	g_object_ref (ev);
      
      list = g_slist_insert_sorted (list, ev, event_sort_func);
      if (++events == max)
	return list;
    }
  
  for (iter = edb->recurring_events; iter; iter = g_list_next (iter))
    {
      Event *ev = iter->data, *clone;
      time_t event_start = ev->start, clone_start;
      recur_t r = ev->recur;
      time_t window_start = start;

      LIVE (ev);
      
      if (untimed_significant)
	{
	  /* Ignore events with wrong untimed status */
	  if (untimed != ((ev->flags & FLAG_UNTIMED) != 0))
	    continue;
	}

      /* Stop if all remaining events are finished already */
      if (r->end && start > r->end)
	break;

      /* Skip events that haven't started yet */
      if (end && event_start > end)
	continue;
      
      /* Skip events without alarms when applicable */
      if (alarms && !ev->alarm)
	continue;

      localtime_r (&event_start, &tm_event);

      switch (r->type)
	{
	case RECUR_NONE:
	  g_critical ("Event on recurrence list marked RECUR_NONE.");
	  continue;
	  
	case RECUR_DAILY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);

	      if ((tm_event.tm_yday - tm_display.tm_yday) % r->increment ==0) 
		{
		  tm_event.tm_mon = tm_display.tm_mon;
		  tm_event.tm_year = tm_display.tm_year;
		  tm_event.tm_mday = tm_display.tm_mday;
		}

	      clone_start = mktime (&tm_event);

	      if (alarms) 
		clone_start -= ev->alarm;

	      if (clone_start <= end && (clone_start + ev->duration) > window_start)
		{
		  gboolean skip=FALSE;

		  if (alarms) 
		    clone_start += ev->alarm;

		  if (r->exceptions)
		    { 
		      for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
			{
			  if ((long)iter2->data == (long)clone_start) 
			    skip = TRUE;
			}
		    }

		  if (alarms && window_start>clone_start-ev->alarm) 
		    skip = TRUE;

		  if (!skip)
		    {
		      clone = event_clone (ev);

		      if (alarms) 
			clone->start -= ev->alarm;
		      else 
			clone->start = clone_start;
		      list = g_slist_insert_sorted (list, clone,
						    event_sort_func);
		      if (++events == max)
			return list;
		    }
		}

	      window_start += SECONDS_IN_DAY;
	    }
	  break;

	case RECUR_WEEKLY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);
	      if (((r->daymask & MON && tm_display.tm_wday==1) ||
		   (r->daymask & TUE && tm_display.tm_wday==2) ||
		   (r->daymask & WED && tm_display.tm_wday==3) ||
		   (r->daymask & THU && tm_display.tm_wday==4) ||
		   (r->daymask & FRI && tm_display.tm_wday==5) ||
		   (r->daymask & SAT && tm_display.tm_wday==6) ||
		   (r->daymask & SUN && tm_display.tm_wday==0))) 
		{
		  tm_event.tm_mon=tm_display.tm_mon;
		  tm_event.tm_year=tm_display.tm_year;
		  tm_event.tm_mday=tm_display.tm_mday;
		  clone_start=mktime(&tm_event);
		  if (alarms) clone_start-=ev->alarm;
		  
		  if (clone_start <= end && (clone_start + ev->duration) > window_start)
		    {
		      gboolean skip=FALSE;
		      if (alarms) clone_start+=ev->alarm;
		      if (r->exceptions) 
			for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
			  if ((long)iter2->data == (long)clone_start) skip=TRUE;

		      if (alarms && window_start > clone_start-ev->alarm) 
			skip = TRUE;

		      if (!skip)
			{
			  clone = event_clone(ev);
			  if (alarms) clone->start = clone_start-ev->alarm;
			  else clone->start = clone_start;
			  list = g_slist_insert_sorted (list, clone,
							event_sort_func);
			  if (++events == max)
			    return list;
			}
		    }
		}
	      
	      window_start += SECONDS_IN_DAY;
	    }
	  break;

	case RECUR_MONTHLY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);
	      month_diff=tm_display.tm_mon-tm_event.tm_mon +
		(tm_display.tm_year-tm_event.tm_year)*12;
	      if ((tm_display.tm_mday==tm_event.tm_mday) &&
		  (month_diff%r->increment)==0) 
		{
		  tm_event.tm_mon=tm_display.tm_mon;
		  tm_event.tm_year=tm_display.tm_year;
		  tm_event.tm_mday=tm_display.tm_mday;
		}; 
	      clone_start=mktime(&tm_event);
	      if (alarms) clone_start-=ev->alarm;
	      if (clone_start <= end && (clone_start + ev->duration) > window_start) 
		{
		  gboolean skip=FALSE;
		  if (alarms) clone_start+=ev->alarm;
		  if (r->exceptions) 
		    for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
		      if ((long)iter2->data == (long)clone_start) skip=TRUE;
		  if(alarms && window_start>clone_start-ev->alarm) skip=TRUE;
		  if (!skip)
		    {
		      clone = event_clone(ev);
		      if (alarms) clone->start = clone_start-ev->alarm;
		      else clone->start = clone_start;
		      list = g_slist_insert_sorted (list, clone,
						    event_sort_func);
		      if (++events == max)
			return list;
		    }
		}
	      
	      window_start += SECONDS_IN_DAY;
	    }
	  break;

	case RECUR_YEARLY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);
	      if ((tm_display.tm_mon==tm_event.tm_mon) &&
		  (tm_display.tm_mday==tm_event.tm_mday) &&
		  ((tm_display.tm_year-tm_event.tm_year)%r->increment)==0) 
		{
		  tm_event.tm_mon=tm_display.tm_mon;
		  tm_event.tm_year=tm_display.tm_year;
		  tm_event.tm_mday=tm_display.tm_mday;
		}; 
	      clone_start=mktime(&tm_event);
	      if (alarms) clone_start-=ev->alarm;
	      if (clone_start <= end && (clone_start + ev->duration) > window_start)
		{
		  gboolean skip=FALSE;
		  if (alarms) clone_start+=ev->alarm;
		  if (r->exceptions)
		    {
		      for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
			if ((long)iter2->data == (long)clone_start) skip=TRUE;
		    }
		  if(alarms && window_start > clone_start-ev->alarm) 
		    skip = TRUE;
		  if (!skip)
		    {
		      clone = event_clone(ev);
		      if (alarms)
			clone->start = clone_start - ev->alarm;
		      else
			clone->start = clone_start;
		      list = g_slist_insert_sorted (list, clone,
						    event_sort_func);
		      if (++events == max)
			return list;
		    }
		}

	      window_start += SECONDS_IN_DAY;
	    }
	  break;
  	}
    }

  return list;
}

/**
 * event_db_list_for_period:
 * @start: Start time
 * @end: End time
 * 
 * Create a list of all events for a given period.
 *
 * Returns: A list of events.
 */
GSList *
event_db_list_for_period (EventDB *edb, time_t start, time_t end)
{
  return event_db_list_for_period_internal (edb, start, end,
					    FALSE, FALSE, FALSE, 0);
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
  GSList *return_list = NULL;
  time_t shifted_end;
  struct tm tm_shift;
  
  localtime_r (&start, &tm_shift);
  tm_shift.tm_mday++;
  tm_shift.tm_hour=0;
  tm_shift.tm_min=0;
  tm_shift.tm_sec=1;
  shifted_end=mktime(&tm_shift);
  return_list = event_db_list_for_period_internal (edb, start, shifted_end,
						   FALSE, FALSE, TRUE, 0);
  start=shifted_end;
   
  while (!return_list && start<end)
    {
      return_list
	= event_db_list_for_period_internal (edb,
					     start, start+SECONDS_IN_DAY,
					     FALSE, FALSE, TRUE, 0);
      start+=SECONDS_IN_DAY;
    }
  return(return_list);
}

/**
 * event_db_untimed_list_for_period:
 * @start: Start time
 * @end: End time
 * @yes:
 * 
 * Create a list of all untimed events for a given period.
 *
 * Returns: A list of events.
 */
GSList *
event_db_untimed_list_for_period (EventDB *edb, time_t start, time_t end,
				  gboolean yes)
{
  return event_db_list_for_period_internal (edb, start, end,
					    yes, TRUE, FALSE, 0);
}

/**
 * event_db_list_for_future:
 * @start: Start time
 * @max: Maximum number of events to retrieve.
 * 
 * Get a list of all future events starting at a given time. The total amount 
 * of events may be limited by the @max parameter. Set this to 0 for an 
 * unlimited amount of events.
 *
 * Returns: A list of events.
 */
GSList *
event_db_list_for_future (EventDB *edb, time_t start, guint max)
{
  int NUMBER_OF_DAYS=14;
  int day_inc;
  GSList *return_list = NULL;
  time_t shifted_end;
  struct tm tm_shift;
  
  localtime_r (&start, &tm_shift);
  tm_shift.tm_mday++;
  tm_shift.tm_hour=0;
  tm_shift.tm_min=0;
  tm_shift.tm_sec=1;
  shifted_end=mktime(&tm_shift);
  return_list = event_db_list_for_period_internal (edb, start, shifted_end,
						   FALSE, FALSE, TRUE, 0);
  start=shifted_end;
   
  for (day_inc=0;day_inc<NUMBER_OF_DAYS;day_inc++) 
    {
      return_list = g_slist_concat (return_list,
				    event_db_list_for_period_internal
				    (edb, start, start+SECONDS_IN_DAY,
				     FALSE, FALSE, FALSE, max));
      start+=SECONDS_IN_DAY;
    }
  
  return return_list;
}

#define insert_values(db, id, key, format, value)	\
	sqlite_exec_printf (db, "insert into calendar values (%d, '%q', '" format "')", \
			    NULL, NULL, err, id, key, value)

/* Dump out an event to the SQL database */
static gboolean
event_write (Event *ev, char **err)
{
  char buf_start[64], buf_end[64];
  struct tm tm;
  gboolean rc = FALSE;
  GSList *iter;

  gmtime_r (&ev->start, &tm);
  strftime (buf_start, sizeof (buf_start), 
	    (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
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
                (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
                &tm); 
          if (insert_values (ev->edb->sqliteh, ev->uid, "rend", "%q", buf_end)) 
            goto exit;
        }
    }

  if (insert_values (ev->edb->sqliteh, ev->uid, "alarm", "%d", ev->alarm))
    goto exit;

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
    ev->eventid = event_db_make_eventid();

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

  event_db_remove_internal (ev);
  ev->start = start;
  event_db_add_internal (ev);
}

#define GET_SET(type, field) \
  type \
  event_get_##field (Event *ev) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    return ev->field; \
  } \
 \
  void \
  event_set_##field (Event *ev, type field) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    STAMP (ev); \
    ev->field = field; \
  }

GET_SET (unsigned long, duration)
GET_SET (unsigned long, alarm)

#define GET_SET_FLAG(flag, FLAG) \
  gboolean \
  event_is_##flag (Event *ev) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    return !! (ev->flags & FLAG); \
  } \
 \
  void \
  event_set_##flag (Event *ev) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    if ((ev->flags & FLAG) == FLAG) \
      return;  \
    STAMP (ev); \
    ev->flags |= FLAG; \
  } \
 \
  void \
  event_clear_##flag (Event *ev) \
  { \
    LIVE (ev); \
    ev = RESOLVE_CLONE (ev); \
    if ((ev->flags & FLAG) == 0) \
      return;  \
    STAMP (ev); \
    ev->flags &= ~FLAG; \
  }

GET_SET_FLAG (untimed, FLAG_UNTIMED)

unsigned long
event_get_uid (Event *ev)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  return ev->uid;
}

const char *
event_get_eventid (Event *ev)
{
  LIVE (ev);
  ev = RESOLVE_CLONE (ev);
  return ev->eventid;
}

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

const GSList *const
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
