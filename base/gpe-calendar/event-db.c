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

#include <sqlite.h>

#include "event-db.h"
#include "globals.h"

static sqlite *sqliteh;

static GSList *single_events, *recurring_events;

static char *dname = "/.gpe/calendar";

static unsigned long uid;

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

static int
load_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 5)
    {
      char *p;
      struct tm tm;
      event_t ev = g_malloc (sizeof (struct event_s));
      
      ev->flags = 0;
      ev->uid = atoi (argv[0]);

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
      
      if (event_db_add_internal (ev) == FALSE)
	return 1;
    }

  return 0;
}

gboolean
event_db_start (void)
{
  static const char *schema_str = 
    "create table events (uid integer NOT NULL, start text NOT NULL,"
    " duration integer, alarmtime integer, alarmtype text, summary text, description text, recurring text)";
  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL) 
    home = "";
  len = strlen (home) + strlen (dname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, dname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema_str, NULL, NULL, &err);
  
  if (sqlite_exec (sqliteh, "select uid, start, duration, alarmtime, recurring from events", load_callback, NULL, &err))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return FALSE;
    }

  return TRUE;
}

event_details_t
event_db_get_details (event_t ev)
{
  char *err;
  int nrow, ncol;
  char **results;
  event_details_t evd;

  if (sqlite_get_table_printf (sqliteh, "select summary,description from events where uid=%d", &results, &nrow, &ncol, &err, ev->uid))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return NULL;
    }

  evd = g_malloc (sizeof (struct event_details_s));
  
  evd->summary = g_strdup (results[ncol]);
  evd->description = g_strdup (results[ncol + 1]);

  sqlite_free_table (results);

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

/* Add an event to both the in-memory list and the SQL database */
gboolean
event_db_add (event_t ev)
{
  char *err;
  char buf[256];
  struct tm tm;

  ev->uid = uid;

  if (event_db_add_internal (ev) == FALSE)
    return FALSE;
 
  gmtime_r (&ev->start, &tm);
  strftime (buf, 256, 
	    (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
	    &tm);
 
  if (sqlite_exec_printf (sqliteh, 
			  "insert into events (uid, start, duration, alarmtime, description, summary) values (%d, '%q', %d, %d, '%q', '%q')", 
			  NULL, NULL, &err, 
			  ev->uid, buf, ev->duration, ev->alarm, 
			  ev->details->description, ev->details->summary))
    {
      event_db_remove_internal (ev);
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return FALSE;
    }

  return TRUE;
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
  memset (ev->details, 0, sizeof (ev->details));
  return ev->details;
}
