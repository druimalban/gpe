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
#include <math.h>
#include <sys/stat.h>

#include <sqlite.h>

#include <gpe/errorbox.h>

#include "event-db.h"
#include "globals.h"

static unsigned long dbversion;
static sqlite *sqliteh;

static GSList *recurring_events, *one_shot_events;

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

  if (ev->recur)
    recurring_events = g_slist_insert_sorted (recurring_events, ev, 
					      (GCompareFunc)event_sort_func);
  else
    one_shot_events = g_slist_insert_sorted (one_shot_events, ev, 
					     (GCompareFunc)event_sort_func);

  return TRUE;
}

/* Remove an event from the in-memory list */
static gboolean
event_db_remove_internal (event_t ev)
{
  if (ev->recur)
    recurring_events = g_slist_remove (recurring_events, ev);
  else
    one_shot_events = g_slist_remove (one_shot_events, ev);

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
      event_t ev = arg;
     
      if (!strcasecmp (argv[0], "start"))
	{
	  gboolean untimed;

	  parse_date (argv[1], &ev->start, &untimed);

	  if (untimed)
	    ev->flags |= FLAG_UNTIMED;
	}
      else if (!strcasecmp (argv[0], "rend"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  parse_date (argv[1], &r->end, NULL);
	}
      else if (!strcasecmp (argv[0], "rcount"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->count = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "rincrement"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->increment = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "rdaymask"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->daymask = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "recur"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->type = atoi (argv[1]);
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

      if (ev->recur && ev->recur->type == RECUR_NONE)
	{
	  /* Old versions of gpe-calendar dumped out a load of recurrence tags
	     even for a one-shot event.  */
	  g_free (ev->recur);
	  ev->recur = NULL;
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
      if (sqlite_exec (sqliteh, "select uid from calendar_urn", load_callback, NULL, &err))
        {
          gpe_error_box (err);
          free (err);
          return FALSE;
        }
    }
    
  else 
    {
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
      else if (!strcasecmp (argv[0], "modified"))
	parse_date (argv[1], &evd->modified, NULL);
      else if (!strcasecmp (argv[0], "sequence"))
	evd->sequence = atoi (argv[1]);
    }
  return 0;
}

event_details_t
event_db_get_details (event_t ev)
{
  char *err;
  event_details_t evd;

  if (ev->details)
    {
      evd = ev->details;
    }
  else
    {
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
    }

  evd->ref++;

  return evd;
}

void
event_db_forget_details (event_t ev)
{
  if (ev->details)
    {
      if (ev->details->ref == 0)
	fprintf (stderr, "details refcount was already zero!\n");
      else
	ev->details->ref--;

      if (ev->details->ref == 0)
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
}

gboolean
event_db_stop (void)
{
  sqlite_close (sqliteh);
  return TRUE;
}

static GSList *
event_db_list_for_period_internal (time_t start, time_t end, gboolean untimed, 
				   gboolean untimed_significant, guint max)
{
  GSList *iter;
  GSList *list = NULL;
  double test1, test2,not_today;
  struct tm tm_current, tm_event;
  guint n = 0;
  
  localtime_r (&start, &tm_current);
  
  for (iter = one_shot_events; iter; iter = g_slist_next (iter))
    {
      event_t ev = iter->data;
      
      /* Stop if we have enough events */
      if (max && n >= max)
	break;

      if (untimed_significant)
	{
	  /* Ignore events with wrong untimed status */
	  if (untimed != ((ev->flags & FLAG_UNTIMED) != 0))
	    continue;
	}

      /* Stop if event hasn't started yet */
      if (end && ev->start > end)
	break;
      
      /* Skip events that have finished already */
      if ((ev->start + ev->duration < start)
	  || (ev->duration && ((ev->start + ev->duration == start))))
	continue;

      list = g_slist_append (list, ev);
      n++;
    }

  for (iter = recurring_events; max && iter; iter = g_slist_next (iter))
    {
      event_t ev = iter->data;
      time_t fixed_start = ev->start;
      recur_t r = ev->recur;
      
      /* Stop if we have enough events */
      if (max && n >= max)
	break;

      if (untimed_significant)
	{
	  /* Ignore events with wrong untimed status */
	  if (untimed != ((ev->flags & FLAG_UNTIMED) != 0))
	    continue;
	}

      /* Skip events that haven't started yet */
      if (end && fixed_start > end)
	continue;
      
      /* Skip events that have finished already */
      if ((fixed_start + ev->duration < start)
	  || (ev->duration && ((fixed_start + ev->duration == start))))
	continue;
      list = g_slist_append (list, ev);

      switch (r->type)
	{
	case RECUR_NONE:
	  abort ();
	  
	case RECUR_DAILY:
          localtime_r (&ev->start, &tm_event);
          tm_event.tm_mday=tm_current.tm_mday;
  	  tm_event.tm_mon=tm_current.tm_mon;
  	  tm_event.tm_year=tm_current.tm_year;
  	  fixed_start = mktime (&tm_event);
	  
	  test1=(double)(fixed_start-ev->start);
	  test2=(double)SECONDS_IN_DAY*(double)r->increment;
	  not_today=fmod(test1, test2);
	  
	  /* Stop if event hasn't started yet */
          if (fixed_start > end || (r->end && fixed_start > r->end) || not_today!=0.0)
	    continue;

          /* Skip events that have finished already */
	  if ((fixed_start + ev->duration < start)
	    || (ev->duration && ((fixed_start + ev->duration == start))))
	      continue;
          list = g_slist_append (list, ev);
	  break;

	case RECUR_WEEKLY:
          localtime_r (&ev->start, &tm_event);
          tm_event.tm_mday=tm_current.tm_mday;
  	  tm_event.tm_mon=tm_current.tm_mon;
  	  tm_event.tm_year=tm_current.tm_year;
  	  fixed_start = mktime (&tm_event);
	  
	  /* Stop if event hasn't started yet */
          if (fixed_start > end || (r->end && fixed_start > r->end))
	    continue;

          /* Skip events that have finished already */
	  if ((fixed_start + ev->duration < start)
	    || (ev->duration && ((fixed_start + ev->duration == start))))
	      continue;
          
	  /* Check day of week */
	  if ((r->daymask & MON && tm_event.tm_wday==1) ||
	      (r->daymask & TUE && tm_event.tm_wday==2) ||
	      (r->daymask & WED && tm_event.tm_wday==3) ||
	      (r->daymask & THU && tm_event.tm_wday==4) ||
	      (r->daymask & FRI && tm_event.tm_wday==5) ||
	      (r->daymask & SAT && tm_event.tm_wday==6) ||
	      (r->daymask & SUN && tm_event.tm_wday==0))	  
	     list = g_slist_append (list, ev);
	  break;

	case RECUR_MONTHLY:
          localtime_r (&ev->start, &tm_event);
          test1=(double)(tm_event.tm_mon-tm_current.tm_mon);
	  tm_event.tm_mon=tm_current.tm_mon;
  	  tm_event.tm_year=tm_current.tm_year;
  	  fixed_start = mktime (&tm_event);
	  
	  test2=(double)r->increment;
	  not_today=fmod(test1, test2);
	  /* Stop if event hasn't started yet */
          if (fixed_start > end || (r->end && fixed_start > r->end) || not_today!=0.0)
	    continue;

          /* Skip events that have finished already */
	  if ((fixed_start + ev->duration < start)
	    || (ev->duration && ((fixed_start + ev->duration == start))))
	      continue;
          list = g_slist_append (list, ev);
	  break;

	case RECUR_YEARLY:
          localtime_r (&ev->start, &tm_event);
          test1=(double)(tm_event.tm_year-tm_current.tm_year);
	  tm_event.tm_year=tm_current.tm_year;
  	  fixed_start = mktime (&tm_event);
	  
	  test2=(double)r->increment;
	  not_today=fmod(test1, test2);
	  
	  /* Stop if event hasn't started yet */
          if (fixed_start > end || (r->end && fixed_start > r->end) || not_today!=0.0)
	    continue;

          /* Skip events that have finished already */
	  if ((fixed_start + ev->duration < start)
	    || (ev->duration && ((fixed_start + ev->duration == start))))
	      continue;
          list = g_slist_append (list, ev);
	  break;
	}
    }

  return list;
}

GSList *
event_db_list_for_period (time_t start, time_t end)
{
  return event_db_list_for_period_internal (start, end, FALSE, FALSE, 0);
}

GSList *
event_db_untimed_list_for_period (time_t start, time_t end, gboolean yes)
{
  return event_db_list_for_period_internal (start, end, yes, TRUE, 0);
}

GSList *
event_db_list_for_future (time_t start, guint max)
{
  return event_db_list_for_period_internal (start, 0, FALSE, FALSE, max);
}

void
event_db_list_destroy (GSList *l)
{
  g_slist_free (l);
}

#define insert_values(db, id, key, format, value)	\
	sqlite_exec_printf (db, "insert into calendar values (%d, '%q', '" ## format ## "')", \
			    NULL, NULL, err, id, key, value)

/* Dump out an event to the SQL database */
static gboolean
event_db_write (event_t ev, char **err)
{
  time_t modified;
  char buf_start[64], buf_end[64], buf_modified[64];
  struct tm tm;
  event_details_t ev_d = event_db_get_details (ev);
  gboolean rc = FALSE;

  gmtime_r (&ev->start, &tm);
  strftime (buf_start, sizeof (buf_start), 
	    (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
	    &tm);  

  modified = time (NULL);
  gmtime_r (&ev_d->modified, &tm);
  strftime (buf_modified, sizeof (buf_modified), "%Y-%m-%d %H:%M:%S", &tm); 

  if (insert_values (sqliteh, ev->uid, "summary", "%q", ev_d->summary)
      || insert_values (sqliteh, ev->uid, "description", "%q", ev_d->description)
      || insert_values (sqliteh, ev->uid, "duration", "%d", ev->duration)
      || insert_values (sqliteh, ev->uid, "modified", "%q", buf_modified)
      || insert_values (sqliteh, ev->uid, "start", "%q", buf_start)
      || insert_values (sqliteh, ev->uid, "sequence", "%d", ev_d->sequence))
    goto exit;

  if (ev->recur)
    {
      recur_t r = ev->recur;

      if (insert_values (sqliteh, ev->uid, "recur", "%d", r->type)
	  || insert_values (sqliteh, ev->uid, "rcount", "%d", r->count)
	  || insert_values (sqliteh, ev->uid, "rincrement", "%d", r->increment)
	  || insert_values (sqliteh, ev->uid, "rdaymask", "%d", r->daymask))
	goto exit;

      if (r->end != 0)
	{
	  gmtime_r (&r->end, &tm);
	  strftime (buf_end, sizeof (buf_end), 
		    (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
		    &tm); 
	  if (insert_values (sqliteh, ev->uid, "rend", "%q", buf_end)) 
	    goto exit;
	}
    }

  if (ev->flags & FLAG_ALARM)
    {
      if (insert_values (sqliteh, ev->uid, "alarm", "%d", ev->alarm))
	goto exit;
    }

  rc = TRUE;

 exit:
  event_db_forget_details (ev);
  return rc;
}

gboolean
event_db_flush (event_t ev)
{
  char *err;

  if (sqlite_exec (sqliteh, "begin transaction", NULL, NULL, &err))
    goto error;

  if (sqlite_exec_printf (sqliteh, "delete from calendar where uid=%d", NULL, NULL, &err,
			  ev->uid))
    goto error;

  if (event_db_write (ev, &err) == FALSE
      || sqlite_exec (sqliteh, "commit transaction", NULL, NULL, &err))
    goto error_and_rollback;

  return TRUE;

 error_and_rollback:
  sqlite_exec (sqliteh, "rollback transaction", NULL, NULL, NULL);
 error:
  gpe_error_box (err);
  free (err);
  return FALSE;
}

/* Add an event to both the in-memory list and the SQL database */
gboolean
event_db_add (event_t ev)
{
  char *err;
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

  if (event_db_write (ev, &err) == FALSE
      || sqlite_exec (sqliteh, "commit transaction", NULL, NULL, &err))
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

  sqlite_exec_printf (sqliteh, "delete from calendar where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  sqlite_exec_printf (sqliteh, "delete from calendar_urn where uid=%d", 
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

  if (ev->recur)
    g_free (ev->recur);

  g_free (ev);
}

event_details_t
event_db_alloc_details (event_t ev)
{
  ev->details = (event_details_t) g_malloc (sizeof (struct event_details_s));
  memset (ev->details, 0, sizeof (*ev->details));
  ev->details->ref++;
  return ev->details;
}

recur_t
event_db_get_recurrence (event_t ev)
{
  if (ev->recur == NULL)
    {
      ev->recur = g_malloc (sizeof (struct recur_s));
      memset (ev->recur, 0, sizeof (struct recur_s));

      ev->recur->type = RECUR_NONE;
    }

  return ev->recur;
}
