/* sqlite.c - SQLITE backend.
   Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <glib-object.h>
#include <glib.h>
#include <gpe/errorbox.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <obstack.h>
#include <libintl.h>

#define _(x) gettext(x)

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#include "event-db.h"

#include <sqlite.h>

/* The code is written using sqlite function names but they are
   relatively easily mapped to sqlite3 function names.  */
// #define USE_SQLITE3
#ifdef USE_SQLITE3

#include <sqlite3.h>

#define sqlite sqlite3
#define sqlite_close sqlite3_close
#define sqlite_exec sqlite3_exec
#define sqlite_exec_printf(handle_, query_, cb_, cookie_, err_, args_...) \
  ({ char *q_ = sqlite3_mprintf (query_ , ## args_); \
     int ret_ = sqlite3_exec (handle_, q_, cb_, cookie_, err_); \
     sqlite3_free (q_); \
     ret_; })
#define sqlite_last_insert_rowid sqlite3_last_insert_rowid
#define sqlite_create_aggregate(handle_, name_, args_, step_, final_, \
                                cookie_) \
  sqlite3_create_function (handle_, name_, args_, SQLITE_UTF8, cookie_, \
                           NULL, step_, final_);
#define sqlite_aggregate_context sqlite3_aggregate_context
#define sqlite_func sqlite3_context
#define sqlite_set_result_string(context_, str_, count) \
  sqlite3_result_text (context_, strdup (str_), count, free)

#else /* USE_SQLITE3 */

#define sqlite3_value const char 

#endif

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
      g_warning ("Unable to parse date: %s\n", s);
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

typedef struct _SqliteDBClass SqliteDBClass;
typedef struct _SqliteDB SqliteDB;

extern GType sqlite_db_get_type (void);

#define TYPE_SQLITE_DB (sqlite_db_get_type ())
#define SQLITE_DB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SQLITE_DB, SqliteDB))
#define SQLITE_DB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SQLITE_DB, SqliteDBClass))
#define IS_SQLITE_DB(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SQLITE_DB))
#define IS_SQLITE_DB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SQLITE_DB))
#define SQLITE_DB_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SQLITE_DB, SqliteDBClass))

struct _SqliteDBClass
{
  EventDBClass event_db_class;
};

struct _SqliteDB
{
  EventDB event_db;

  sqlite *sqliteh;
};


static void sqlite_db_class_init (gpointer klass, gpointer klass_data);
static void sqlite_db_init (GTypeInstance *instance, gpointer klass);
static void sqlite_db_dispose (GObject *obj);
static void sqlite_db_finalize (GObject *object);

static GObjectClass *sqlite_db_parent_class;

GType
sqlite_db_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (SqliteDBClass),
	NULL,
	NULL,
	sqlite_db_class_init,
	NULL,
	NULL,
	sizeof (struct _SqliteDB),
	0,
	sqlite_db_init
      };

      type = g_type_register_static (event_db_get_type (),
				     "SqliteEventDB", &info, 0);
    }

  return type;
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
	/* Copy up to the comma.  */
	{
	  int len = end - p;
	  token = g_malloc (len + 1);
	  memcpy (token, p, len);
	  token[len] = 0;
	}
      else
	/* This is the last segment, just copy it.  */
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

static int
do_events_enumerate (EventDB *edb,
		     time_t period_start, time_t period_end,
		     gboolean alarms,
		     int (*cb) (EventSource *ev),
		     char **err)
{
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

  int ret = SQLITE_TRY (sqlite_exec_printf (SQLITE_DB (edb)->sqliteh, q,
					    callback, NULL, err));
  obstack_free (&query, NULL);
  return ret;
}

static void
do_set_default_calendar (EventDB *edb, EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (SQLITE_DB (edb)->sqliteh,
	"insert or replace into default_calendar values (%d);",
	NULL, NULL, &err, ec->uid)))
    {
      g_critical ("%s: %s", __func__, err);
      g_free (err);
    }
}

static gint
do_eventid_to_uid (EventDB *edb, const char *eventid)
{
  guint uid = 0;
  int callback (void *arg, int argc, char *argv[], char **names)
    {
      uid = atoi (argv[0]);
      return 1;
    }
  SQLITE_TRY (sqlite_exec_printf (SQLITE_DB (edb)->sqliteh,
				  "select uid from events"
				  " where eventid='%q';",
				  callback, NULL, NULL, eventid));

  return uid;
}

static gboolean
do_event_new (EventSource *ev, char **err)
{
  if (SQLITE_TRY (sqlite_exec (SQLITE_DB (ev->edb)->sqliteh,
			       "begin transaction;",
			       NULL, NULL, err)))
    return FALSE;

  if (sqlite_exec (SQLITE_DB (ev->edb)->sqliteh,
		   "insert into events (start) values (NULL);",
		   NULL, NULL, err))
    goto error;

  int uid = sqlite_last_insert_rowid (SQLITE_DB (ev->edb)->sqliteh);

  if (sqlite_exec (SQLITE_DB (ev->edb)->sqliteh, "commit transaction",
		   NULL, NULL, err))
    goto error;

  ev->uid = uid;
  return TRUE;

 error:
  sqlite_exec (SQLITE_DB (ev->edb)->sqliteh, "rollback transaction",
	       NULL, NULL, NULL);
  return FALSE;
}

static gboolean
do_event_load (EventSource *ev)
{
  char *err;

  char *q = sqlite_mprintf ("select * from events where uid=%d", ev->uid);

  sqlite_vm *stmt;
  const char *tail = NULL;
  if (sqlite_compile (SQLITE_DB (ev->edb)->sqliteh, q, &tail, &stmt,
		      &err))
    {
      g_warning ("%s:%d: Loading data for event %ld: %s",
		 __func__, __LINE__, ev->uid, err);
      sqlite_freemem (err);
      sqlite_freemem (q);
      return FALSE;
    }

  g_assert (tail == NULL || *tail == '\0');
  sqlite_freemem (q);

  gboolean found = FALSE;
  int argc;
  char **argv;
  char **names;
  if (SQLITE_TRY (sqlite_step (stmt, &argc, &argv, &names)) == SQLITE_ROW)
    {
      found = TRUE;
      event_load_callback (ev, argc, argv, names);
    }

  if (sqlite_finalize (stmt, &err))
    {
      g_warning ("%s:%d: Loading data for event %ld: %s",
		 __func__, __LINE__, ev->uid, err);
      sqlite_freemem (err);
      return FALSE;
    }

  return found;
}

static void
do_event_load_details (EventSource *ev)
{
  int callback (void *arg, int argc, char *argv[], char **names)
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

  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf (SQLITE_DB (ev->edb)->sqliteh,
			   "select tag,value from calendar"
			   " where uid=%d"
			   "  and tag in ('summary',"
			   "              'description', 'location',"
			   "              'category', 'sequence')",
			   callback, ev, &err, ev->uid)))
    {
      gpe_error_box (err);
      free (err);
      return;
    }
}

static gboolean
do_event_flush (EventSource *ev, char **err)
{
  struct tm tm;

  if (SQLITE_TRY
      (sqlite_exec
       (SQLITE_DB (ev->edb)->sqliteh, "begin transaction", NULL, NULL, err)))
    goto error;

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
      (SQLITE_DB (ev->edb)->sqliteh,
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
	  (SQLITE_DB (ev->edb)->sqliteh,
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
	    (SQLITE_DB (ev->edb)->sqliteh,
	     "insert into calendar values (%d, 'category', '%d');",
	     NULL, NULL, err, ev->uid, (int) i->data))
	  goto error;
    }

  if (sqlite_exec (SQLITE_DB (ev->edb)->sqliteh, "commit transaction",
		   NULL, NULL, err))
    goto error;

  ev->modified = FALSE;
  return TRUE;

error:
  sqlite_exec (SQLITE_DB (ev->edb)->sqliteh, "rollback transaction",
	       NULL, NULL, NULL);
  return FALSE;
}

static void
do_event_remove (EventSource *ev)
{
  if (SQLITE_TRY
      (sqlite_exec_printf (SQLITE_DB (ev->edb)->sqliteh,
			   "begin transaction;"
			   "insert into events_deleted (eventid, calendar)"
			   " values ('%q', '%d');"
			   "delete from calendar where uid=%d;"
			   "delete from events where uid=%d;"
			   "commit transaction;",
			   NULL, NULL, NULL, ev->eventid, ev->calendar,
			   ev->uid, ev->uid)))
    sqlite_exec (SQLITE_DB (ev->edb)->sqliteh, "rollback transaction;",
		 NULL, NULL, NULL);
}

static GSList *
do_list_unacknowledged_alarms (EventDB *edb)
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
      (sqlite_exec (SQLITE_DB (edb)->sqliteh,
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
	  (sqlite_exec_printf (SQLITE_DB (edb)->sqliteh,
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

  return list;
}

static void
do_event_mark_unacknowledged (EventSource *ev)
{
  int err;
  char *str;

  err = SQLITE_TRY (sqlite_exec_printf (SQLITE_DB (ev->edb)->sqliteh,
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

static void
do_event_mark_acknowledged (EventSource *ev)
{
  char *err;
  if (SQLITE_TRY (sqlite_exec_printf (SQLITE_DB (ev->edb)->sqliteh,
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

static void
do_acknowledge_alarms_through (EventDB *edb, time_t t)
{
  int err;
  char *str;

  err = SQLITE_TRY
    (sqlite_exec_printf
     (SQLITE_DB (edb)->sqliteh,
      "insert or replace into alarms_fired_through values (%d);",
      NULL, NULL, &str, t));
  if (err)
    {
      g_critical ("%s: %s", __func__, str);
      g_free (str);
    }
}

static void
do_event_calendar_new (EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (SQLITE_DB (ec->edb)->sqliteh,
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

  ec->uid = sqlite_last_insert_rowid (SQLITE_DB (ec->edb)->sqliteh);
}

static void
do_event_calendar_flush (EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf (SQLITE_DB (ec->edb)->sqliteh,
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
}

static void
do_event_calendar_delete (EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (SQLITE_DB (ec->edb)->sqliteh,
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
      sqlite_exec (SQLITE_DB (ec->edb)->sqliteh, "rollback transaction;",
		   NULL, NULL, NULL);
      g_critical ("%s: %s", __func__, err);
      g_free (err);
    }
}

static GSList *
do_event_calendar_list_events (EventCalendar *ec, time_t start, time_t end)
{
  GSList *list = NULL;

  int callback (void *arg, int argc, char *argv[], char **names)
    {
      Event *ev = event_db_find_by_uid (ec->edb, atoi (argv[0]));
      if (ev)
	list = g_slist_prepend (list, ev);
      return 0;
    }

  char *query = "select uid from events where calendar=%d";
  if (start || end)
    {
      char *s = NULL;
      if (start)
	s = g_strdup_printf (" and %ld <= modified", start);

      char *e = NULL;
      if (end)
	s = g_strdup_printf (" and modified <= %ld", end);

      query = g_strdup_printf ("%s%s%s", query, start ? s : "", end ? e : "");

      if (start)
	g_free (s);
      if (end)
	g_free (e);
    }

  SQLITE_TRY
    (sqlite_exec_printf (SQLITE_DB (ec->edb)->sqliteh,
			 query /* ... %d ... */,
			 callback, NULL, NULL, ec->uid));

  if (start || end)
    g_free (query);

  return list;
}

static GSList *
do_event_calendar_list_deleted (EventCalendar *ec)
{
  GSList *list = NULL;

  int callback (void *arg, int argc, char *argv[], char **names)
    {
      EventSource *ev;
      
      if (argc != 3) 
          return 0;
      ev = EVENT_SOURCE (g_object_new (event_source_get_type (), NULL));
      g_object_ref (ec->edb);
      ev->edb = ec->edb;
      ev->uid = (-1) * atoi (argv[0]);
      ev->eventid = g_strdup (argv[1]);
      ev->calendar = atoi (argv[2]);
      ev->event.dead = 1;

      list = g_slist_prepend (list, ev);
      return 0;
    }
  SQLITE_TRY
    (sqlite_exec_printf (SQLITE_DB (ec->edb)->sqliteh,
			 "select uid, eventid, calendar"
			 " from events_deleted where calendar=%d;",
			 callback, NULL, NULL, ec->uid));

  return list;
}

static void 
do_event_calendar_flush_deleted (EventCalendar *ec)
{
   SQLITE_TRY
     (sqlite_exec_printf (SQLITE_DB (ec->edb)->sqliteh,
			  "delete from events_deleted where calendar=%d;",
			  NULL, NULL, NULL, ec->uid));
}

static void
sqlite_db_class_init (gpointer klass, gpointer klass_data)
{
  sqlite_db_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = sqlite_db_finalize;
  object_class->dispose = sqlite_db_dispose;

  EventDBClass *edb = EVENT_DB_CLASS (klass);

  edb->events_enumerate = do_events_enumerate;
  edb->eventid_to_uid = do_eventid_to_uid;
  edb->set_default_calendar = do_set_default_calendar;

  edb->event_new = do_event_new;
  edb->event_load = do_event_load;
  edb->event_load_details = do_event_load_details;
  edb->event_flush = do_event_flush;
  edb->event_remove = do_event_remove;

  edb->list_unacknowledged_alarms = do_list_unacknowledged_alarms;
  edb->event_mark_unacknowledged = do_event_mark_unacknowledged;
  edb->event_mark_acknowledged = do_event_mark_acknowledged;
  edb->acknowledge_alarms_through = do_acknowledge_alarms_through;

  edb->event_calendar_new = do_event_calendar_new;
  edb->event_calendar_flush = do_event_calendar_flush;
  edb->event_calendar_delete = do_event_calendar_delete;
  edb->event_calendar_list_events = do_event_calendar_list_events;
  edb->event_calendar_list_deleted = do_event_calendar_list_deleted;
  edb->event_calendar_flush_deleted = do_event_calendar_flush_deleted;
}

static void
sqlite_db_init (GTypeInstance *instance, gpointer klass)
{
}

static void
sqlite_db_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (sqlite_db_parent_class)->dispose (obj);
}

static void
sqlite_db_finalize (GObject *object)
{
  SqliteDB *db = SQLITE_DB (object);

  G_OBJECT_CLASS (sqlite_db_parent_class)->finalize (object);

  sqlite_close (db->sqliteh);
}

EventDB *
event_db_new (const char *fname)
{
  EventDB *edb = EVENT_DB (g_object_new (TYPE_SQLITE_DB, NULL));
  char *err = NULL;

#ifdef USE_SQLITE3
  int e = sqlite3_open (fname, &SQLITE_DB (edb)->sqliteh);
  if (e)
    {
      err = g_strdup (sqlite3_errmsg (SQLITE_DB (edb)->sqliteh));
      sqlite3_close (SQLITE_DB (edb)->sqliteh);
      SQLITE_DB (edb)->sqliteh = NULL;
    }
#else
  SQLITE_DB (edb)->sqliteh = sqlite_open (fname, 0, &err);
#endif
  if (err)
    goto error_before_transaction;

  if (SQLITE_TRY
      (sqlite_exec (SQLITE_DB (edb)->sqliteh, "begin transaction;",
		    NULL, NULL, &err)))
    goto error_before_transaction;

  /* Get the calendar db version.  */
  sqlite_exec (SQLITE_DB (edb)->sqliteh,
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
  int e = sqlite_exec (SQLITE_DB (edb)->sqliteh,
		       "select version from calendar_dbinfo",
		       dbinfo_callback, NULL, &err);
#ifdef USE_SQLITE3
  /* The database may be in SQLITE2 format.  Try to convert it.  */

  if (e)
    goto error;
#else
  if (e)
    goto error;
#endif

  if (version == 2)
    /* Databases with this version come from a relatively widely
       distributed pre-release of libeventdb with a bug such that the
       calendar table accumulates lots of duplicate entries because we
       kept appending modifications instead of replacing the records.
       Clean this up.  */
    {
      if (sqlite_exec
	  (SQLITE_DB (edb)->sqliteh,
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
    sqlite_exec (SQLITE_DB (edb)->sqliteh,
		 "create table calendar"
		 " (uid INTEGER not NULL, tag STRING, value STRING);",
		 NULL, NULL, NULL);

  if (version == 0)
    /* Convert a version 0 DB to a verion 1 DB.  */
    {
      if (sqlite_exec
	  (SQLITE_DB (edb)->sqliteh,
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
  void cat_step (sqlite_func *context, int argc, sqlite3_value **argv_)
    {
      char **argv = (char **) argv_;

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

  sqlite_create_aggregate (SQLITE_DB (edb)->sqliteh, "cat", 1,
			   cat_step, cat_finalize, NULL);

#define SELECT_FIELD(field) \
  "(select uid, value from calendar where tag='" field "')"

  if (version == 1)
    /* Version 1 versions of the database stored some information in
       the calendar table.  We've since rearranged this.  */
    {
      if (sqlite_exec
	  (SQLITE_DB (edb)->sqliteh,
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
      sqlite_exec (SQLITE_DB (edb)->sqliteh,
		   "select uid, value from calendar where tag='rdaymask';"
		   "delete from calendar where tag='rdaymask'", 
		   callback, NULL, NULL);

      GSList *i;
      for (i = list; i; i = i->next)
	{
	  struct info *info = i->data;

	  sqlite_exec_printf
	    (SQLITE_DB (edb)->sqliteh,
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
	  (SQLITE_DB (edb)->sqliteh,
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
	  (SQLITE_DB (edb)->sqliteh,
	   "create table events"
	   " (uid INTEGER PRIMARY KEY, start DATE, duration INTEGER, "
	   "  recur INTEGER, rend DATE, alarm INTEGER, calendar INTEGER,"
	   "  eventid STRING, rcount INTEGER, rincrement INTEGER,"
	   "  modified DATE, byday STRING, rexceptions STRING);",
	   NULL, NULL, &err))
	goto error;

      sqlite_exec (SQLITE_DB (edb)->sqliteh,
		   "create index events_enumerate_index"
		   " on events (start, duration, rend, alarm);",
		   NULL, NULL, &err);
    }

  if (version == 1 || version == 2)
    {
      if (sqlite_exec
	  (SQLITE_DB (edb)->sqliteh,
	   "insert into events select * from foo;"
	   "drop table foo;",
	   NULL, NULL, &err))
	goto error;
    }

  /* Read EDB->ALARMS_FIRED_THROUGH.  */
  edb->alarms_fired_through = time (NULL);
  if (version < 2)
    sqlite_exec (SQLITE_DB (edb)->sqliteh,
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
  if (sqlite_exec (SQLITE_DB (edb)->sqliteh,
		   "select time from alarms_fired_through",
		   alarms_fired_through_callback, edb, &err))
    goto error;

  /* Unacknowledged alarms.  */

  /* A table of events whose alarm fired before
     EDB->ALARMS_FIRED_THROUGH but were not yet acknowledged.  */
  if (version < 2)
    sqlite_exec (SQLITE_DB (edb)->sqliteh,
		 "create table alarms_unacknowledged"
		 " (uid INTEGER, start INTEGER NOT NULL)",
		 NULL, NULL, NULL);


  /* The default calendar.  */

  /* This table definately exists in a version 2 DB and may exist in a
     version 1 DB depending on the revision.  */
  if (version < 2)
    sqlite_exec (SQLITE_DB (edb)->sqliteh,
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
  if (sqlite_exec (SQLITE_DB (edb)->sqliteh,
		   "select default_calendar from default_calendar",
		   default_calendar_callback, edb, &err))
    goto error;


  /* Calendars.  */

  sqlite_exec (SQLITE_DB (edb)->sqliteh,
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
  if (sqlite_exec (SQLITE_DB (edb)->sqliteh,
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
    sqlite_exec (SQLITE_DB (edb)->sqliteh,
		 "create table events_deleted"
		 " (uid INTEGER PRIMARY KEY, eventid STRING NOT NULL, calendar INTEGER);",
		 NULL, NULL, NULL);
    

  /* Update the version information as appropriate.  */
  if (version < 4)
    {
      if (sqlite_exec (SQLITE_DB (edb)->sqliteh,
		       "delete from calendar_dbinfo;"
		       "insert into calendar_dbinfo (version) values (4);",
		       NULL, NULL, &err))
	goto error;
    }

  /* All done.  */
  sqlite_exec (SQLITE_DB (edb)->sqliteh, "commit transaction;",
	       NULL, NULL, NULL);

  return edb;
 error:
  sqlite_exec (SQLITE_DB (edb)->sqliteh, "rollback transaction;",
	       NULL, NULL, NULL);
 error_before_transaction:
  if (err)
    {
      gpe_error_box_fmt ("event_db_new: %s", err);
      free (err);
    }

  if (SQLITE_DB (edb)->sqliteh)
    sqlite_close (SQLITE_DB (edb)->sqliteh);

  g_object_unref (edb);

  return NULL;
}
