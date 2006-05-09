/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include <mimedir/mimedir-vcal.h>

#include <sqlite.h>
#include <gpe/vevent.h>
#include <gpe/vtodo.h>
#include <gpe/event-db.h>

#include "globals.h"

#define _(x)  (x)

#define TODO_DB_NAME "/.gpe/todo"

static gboolean
parse_date (const char *s, time_t *t, gboolean *date_only)
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

static void
do_import_vevent (MIMEDirVEvent *event)
{
  GSList *tags, *i;
  Event *ev;

  tags = vevent_to_tags (event);

  /* First find the eventid.  */
  const char *eventid = NULL;
  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *t = i->data;
      if (!strcasecmp (t->tag, "eventid"))
	{
	  eventid = t->value;
	  break;
	}
    }

  ev = event_new (event_db, eventid);

  /* Now load the event.  */
  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *t = i->data;

      if (!strcasecmp (t->tag, "start"))
	{
	  time_t start;
	  gboolean untimed;

	  parse_date (t->value, &start, &untimed);
	  event_set_recurrence_start (ev, start);
	  event_set_untimed (ev, untimed);
	}
      else if (!strcasecmp (t->tag, "eventid"))
	/* Ignore, we already have this.  */;
      else if (!strcasecmp (t->tag, "rend"))
	{
	  time_t end;
	  parse_date (t->value, &end, NULL);
	  event_set_recurrence_end (ev, end);
	}
      else if (!strcasecmp (t->tag, "rcount"))
	event_set_recurrence_count (ev, atoi (t->value));
      else if (!strcasecmp (t->tag, "rincrement"))
	event_set_recurrence_increment (ev, atoi (t->value));
      else if (!strcasecmp (t->tag, "rdaymask"))
	event_set_recurrence_daymask (ev, atoi (t->value));
      else if (!strcasecmp (t->tag, "rexceptions"))
	event_add_recurrence_exception (ev, (time_t) atoi (t->value));
      else if (!strcasecmp (t->tag, "recur"))
	event_set_recurrence_type (ev, atoi (t->value));
      else if (!strcasecmp (t->tag, "duration"))
	event_set_duration (ev, atoi (t->value));
      else if (!strcasecmp (t->tag, "alarm"))
	event_set_alarm (ev, atoi (t->value));
      else
	fprintf (stderr,
		 "While importing eventid %s, ignored unknown tag %s\n",
		 eventid, t->tag);
    }
  
  gpe_tag_list_free (tags);

  event_flush (ev);
}

static void
do_import_vtodo (MIMEDirVTodo *todo)
{
  sqlite *db;
  GSList *tags, *i;
  char *buf;
  const gchar *home;
  char *err = NULL;
  int id;

  home = g_get_home_dir ();
  
  buf = g_strdup_printf ("%s%s", home, TODO_DB_NAME);

  db = sqlite_open (buf, 0, &err);
  g_free (buf);

  if (db == NULL)
    {
      gpe_error_box (err);
      free (err);
      return;
    }
 
  if (sqlite_exec (db, "insert into todo_urn values (NULL)", NULL, NULL, &err) != SQLITE_OK)
    {
      gpe_error_box (err);
      free (err);
      sqlite_close (db);
      return;
    }

  id = sqlite_last_insert_rowid (db);

  tags = vtodo_to_tags (todo);

  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *t = i->data;

      sqlite_exec_printf (db, "insert into todo values ('%d', '%q', '%q')", NULL, NULL, NULL,
			  id, t->tag, t->value);
    }
  
  gpe_tag_list_free (tags);

  sqlite_close (db);
}

static void
do_import_vcal (MIMEDirVCal *vcal)
{
  GSList *list, *iter;
    
  list = mimedir_vcal_get_event_list (vcal);
    
  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVEvent *vevent;
      vevent = MIMEDIR_VEVENT (iter->data);
      do_import_vevent (vevent);
    }

  g_slist_free (list);

  list = mimedir_vcal_get_todo_list (vcal);

  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVTodo *vtodo;
      vtodo = MIMEDIR_VTODO (iter->data);
      do_import_vtodo (vtodo);
    }

  g_slist_free (list);
}

int
import_vcal (const gchar *filename)
{
  MIMEDirVCal *cal = NULL;
  GError *error = NULL;
  GList *callist, *l;
  int result = 0;

  callist = mimedir_vcal_read_file (filename, &error);

  if (error) 
    {
      fprintf (stderr, "import_vcal : %s\n",
	       error->message);
      g_error_free (error);
      return -1;
    }

  for (l = callist; l != NULL && result == 0; l = g_list_next (l))
    {
      if( l->data != NULL && MIMEDIR_IS_VCAL (l->data)) 
        {
           cal = l->data;
           do_import_vcal (cal);
        }
      else
        result = -3;
    }

  /* Cleanup */
  mimedir_vcal_free_list (callist);
  
  return result;	
}
