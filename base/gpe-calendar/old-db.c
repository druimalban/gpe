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

static int
load_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 7)
    {
      char *p;
      struct tm tm;
      event_t ev = g_malloc (sizeof (struct event_s));
      
      ev->flags = 0;
      ev->uid = atoi (argv[0]);
      ev->details = g_malloc (sizeof (struct event_details_s));

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
      ev->recur.type = RECUR_NONE;

      ev->details->summary = g_strdup (argv[5]);
      ev->details->description = g_strdup (argv[6]);

      if (event_db_add (ev) == FALSE)
	return 1;
    }

  return 0;
}

gboolean
convert_old_db (sqlite *sqliteh)
{
  char *err;
  int nrow, ncol;
  char **results;

  if (sqlite_get_table (sqliteh, "select uid from events where start='*MIGRATED*'", &results, 
			&nrow, &ncol, &err))
    {
      free (err);
      return FALSE;
    }

  sqlite_free_table (results);
  if (nrow)
    {
      /* Database already converted */
      return TRUE;
    }

  if (sqlite_exec (sqliteh, "select uid, start, duration, alarmtime, recurring, summary, description from events", load_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  if (sqlite_exec (sqliteh, "insert into events values (0, '*MIGRATED*', NULL, NULL, NULL, NULL, NULL, NULL)",
		   NULL, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  return TRUE;
}
