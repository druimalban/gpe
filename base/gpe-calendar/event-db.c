/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#include <sys/stat.h>

#include <sqlite.h>

#include <gpe/errorbox.h>

#include "event-db.h"
#include "globals.h"

static unsigned long dbversion;
static sqlite *sqliteh;

static GSList *single_events, *recurring_events;

static const char *fname = "/.gpe/calendar";

static unsigned long uid;

static const char *schema_str = 
"create table calendar (uid integer NOT NULL, tag text, value text)";
static const char *schema2_str = 
"create table calendar_urn (uid INTEGER PRIMARY KEY)";
static const char *schema_info = 
"create table calendar_dbinfo (version integer NOT NULL)";

extern gboolean convert_old_db (int oldversion, sqlite *);

static gint
event_sort_func (const event_t ev1, const event_t ev2)
{
  return (ev1->start > ev2->start) ? 1 : 0;
}

/* Add an event to the in-memory list */
static gboolean
event_db_add_internal (event_t ev)
{
  if (ev->uid >= uid)
    uid = ev->uid + 1;

  if (ev->recur.type != RECUR_NONE)
    recurring_events = g_slist_insert_sorted (recurring_events, ev, 
					      (GCompareFunc)event_sort_func);
  else
    single_events = g_slist_insert_sorted (single_events, ev, 
					   (GCompareFunc)event_sort_func);
  return TRUE;
}

/* Remove an event from the in-memory list */
static gboolean
event_db_remove_internal (event_t ev)
{
  if (ev->recur.type != RECUR_NONE)
    recurring_events = g_slist_remove (recurring_events, ev);
  else
    single_events = g_slist_remove (single_events, ev);
  return TRUE;
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
  *date_only = (p == NULL) ? TRUE : FALSE;

  *t = timegm (&tm);
  return TRUE;
}

static int
load_data_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      event_t ev = arg;
     
      if (!strcasecmp (argv[0], "start"))
	{
	  gboolean untimed;

	  parse_date (argv[1], &ev->start, &untimed);

	  if (untimed)
	    ev->flags |= FLAG_UNTIMED;
	}
      else if (!strcasecmp (argv[0], "duration"))
	{
	  ev->duration = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "alarm"))
	{
	  ev->alarm = atoi (argv[1]);
	  ev->flags |= FLAG_ALARM;
	}
    }
  return 0;
}

static int
load_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1)
    {
      char *err;
      guint uid = atoi (argv[0]);
      event_t ev = g_malloc (sizeof (struct event_s));
      memset (ev, 0, sizeof (*ev));
      ev->uid = uid;
      if (sqlite_exec_printf (sqliteh, "select tag,value from calendar where uid=%d", 
			      load_data_callback, ev, &err, uid))
	{
	  gpe_error_box (err);
	  free (err);
	  g_free (ev);
	  return 1;
	}

      if (event_db_add_internal (ev) == FALSE)
	return 1;
    }
  return 0;
}

static int
dbinfo_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1)
    {
      dbversion = atoi (argv[0]);
    }

  return 0;
}

gboolean
event_db_start (void)
{
  const char *home = g_get_home_dir ();
  char *buf;
  char *err;
  size_t len;
  len = strlen (home) + strlen (fname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, fname);
  sqliteh = sqlite_open (buf, 0, &err);
  g_free (buf);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema_info, NULL, NULL, &err);
  
  if (sqlite_exec (sqliteh, "select version from calendar_dbinfo", dbinfo_callback, NULL, &err))
    {
      dbversion=0;
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema2_str, NULL, NULL, &err);
      
  if (dbversion==1) 
    {
      printf("do not need to convert!\n");
      if (sqlite_exec (sqliteh, "select uid from calendar_urn", load_callback, NULL, &err))
        {
          gpe_error_box (err);
          free (err);
          return FALSE;
        }
    }
    
  else 
    {
      printf("converting!\n");
      convert_old_db (dbversion, sqliteh);
      dbversion=1;
    }
    
  return TRUE;
}

static int
load_details_callback (void *arg, int argc, char *argv[], char **names)
{
  if (argc == 2)
    {
      event_details_t evd = arg;
      if (!strcasecmp (argv[0], "summary") && !evd->summary)
	evd->summary = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "description") && !evd->description)
	evd->description = g_strdup (argv[1]);
    }
  return 0;
}

event_details_t
event_db_get_details (event_t ev)
{
  char *err;
  event_details_t evd;

  evd = g_malloc (sizeof (struct event_details_s));
  memset (evd, 0, sizeof (*evd));

  if (sqlite_exec_printf (sqliteh, "select tag,value from calendar where uid=%d",
			  load_details_callback, evd, &err, ev->uid))
    {
      gpe_error_box (err);
      free (err);
      g_free (evd);
      return NULL;
    }

  ev->details = evd;

  return evd;
}

void
event_db_forget_details (event_t ev)
{
  if (ev->details)
    {
      event_details_t evd = ev->details;
      if (evd->description)
	g_free (evd->description);
      if (evd->summary)
	g_free (evd->summary);
      g_free (evd);
    }

  ev->details = NULL;
}

gboolean
event_db_stop (void)
{
  sqlite_close (sqliteh);
  return TRUE;
}

static GSList *
event_db_list_for_period_internal (time_t start, time_t end, gboolean untimed)
{
  GSList *iter;
  GSList *list = NULL;

  for (iter = single_events; iter; iter = g_slist_next (iter))
    {
      event_t ev = iter->data;
      assert (ev->recur.type == RECUR_NONE);

      if (untimed != ((ev->flags & FLAG_UNTIMED) != 0))
	continue;

      /* Stop if event hasn't started yet */
      if (ev->start > end)
	break;

      /* Skip events that have finished already */
      if ((ev->start + ev->duration < start)
	  || (ev->duration && ((ev->start + ev->duration == start))))
	continue;

      list = g_slist_append (list, ev);
    }

  for (iter = recurring_events; iter; iter = g_slist_next (iter))
    {
      event_t ev = iter->data;
      assert (ev->recur.type != RECUR_NONE);

      /* Stop if event hasn't started yet */
      if (ev->start > end)
	break;

      /* Skip dead events, ie if "repeat until" time is past */
      if (ev->recur.end < start)
	continue;

      /* this will need to be fixed.... */
      list = g_slist_append (list, ev);
    }

  return list;
}

GSList *
event_db_list_for_period (time_t start, time_t end)
{
  return event_db_list_for_period_internal (start, end, FALSE);
}

GSList *
event_db_untimed_list_for_period (time_t start, time_t end)
{
  return event_db_list_for_period_internal (start, end, TRUE);
}

void
event_db_list_destroy (GSList *l)
{
  g_slist_free (l);
}

#define insert_values(db, id, key, format, value)	\
	sqlite_exec_printf (db, "insert into calendar values (%d, '%q', '" ## format ## "')", \
			    NULL, NULL, &err, id, key, value)

/* Add an event to both the in-memory list and the SQL database */
gboolean
event_db_add (event_t ev)
{
  char *err;
  char buf[256];
  struct tm tm;
  gboolean rollback = FALSE;

  if (sqlite_exec (sqliteh, "begin transaction", NULL, NULL, &err))
    goto error;

  rollback = TRUE;

  if (sqlite_exec (sqliteh, "insert into calendar_urn values (NULL)",
		   NULL, NULL, &err))
    goto error;

  ev->uid = sqlite_last_insert_rowid (sqliteh);

  if (event_db_add_internal (ev) == FALSE)
    {
      err = strdup ("Could not insert event");
      goto error;
    }

  gmtime_r (&ev->start, &tm);
  strftime (buf, 256, 
	    (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
	    &tm);  

  if (insert_values (sqliteh, ev->uid, "summary", "%q", ev->details->summary)
      || insert_values (sqliteh, ev->uid, "description", "%q", ev->details->description)
      || insert_values (sqliteh, ev->uid, "duration", "%d", ev->duration)
      || insert_values (sqliteh, ev->uid, "start", "%q", buf))
    goto error;

  if (ev->flags & FLAG_ALARM)
    {
      if (insert_values (sqliteh, ev->uid, "alarm", "%d", ev->alarm))
	goto error;
    }

  if (sqlite_exec (sqliteh, "commit transaction", NULL, NULL, &err))
    goto error;

  return TRUE;

 error:
  if (rollback)
    sqlite_exec (sqliteh, "rollback transaction", NULL, NULL, NULL);
  gpe_error_box (err);
  free (err);
  return FALSE;
}

/* Remove an event from both the in-memory list and the SQL database 
   from ev pointer */
gboolean
event_db_remove (event_t ev)
{
  if (event_db_remove_internal (ev) == FALSE)
    return FALSE;

  sqlite_exec_printf (sqliteh, "delete from events where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  return TRUE;
}

event_t
event_db_new (void)
{
  return (event_t) g_malloc (sizeof (struct event_s));
}

void
event_db_destroy (event_t ev)
{
  if (ev->details)
    {
      event_details_t ev_d = ev->details;
      if (ev_d->description)
	g_free (ev_d->description);
      g_free (ev_d);
    }

  g_free (ev);
}

event_details_t
event_db_alloc_details (event_t ev)
{
  ev->details = (event_details_t) g_malloc (sizeof (struct event_details_s));
  memset (ev->details, 0, sizeof (*ev->details));
  return ev->details;
}
