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
#include <gpe/event-db.h>

static int
load_callback0 (void *arg, int argc, char **argv, char **names)
{
  if (argc == 7)
    {
      char *p;
      struct tm tm;
      event_t ev = event_db__alloc_event ();
      
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

      ev->details->summary = g_strdup (argv[5]);
      ev->details->description = g_strdup (argv[6]);

      if (event_db_add (ev) == FALSE)
	return 1;
    }

  return 0;
}

gboolean
convert_old_db (int oldversion, sqlite *sqliteh)
{
  char *err;
  
  if (oldversion==0) 
    {
      if (sqlite_exec (sqliteh, "select uid, start, duration, alarmtime, recurring, summary, description from events", load_callback0, NULL, &err))
        {
          free (err);
        }

    }

  oldversion=1; /* set equal to new version */
  
  if (sqlite_exec_printf (sqliteh, 
			  "insert into calendar_dbinfo (version) values (%d)", 
			  NULL, NULL, &err, oldversion))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return FALSE;
    }
    
  return TRUE;
}
