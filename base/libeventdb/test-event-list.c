/* test-event-list.c - Test the event_db_list_* family of functions.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

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

int do_test (int argc, char *argv[]);

#include "test-skeleton.h"

#include <stdio.h>
#include <error.h>
#include "gpe/event-db.h"

static int fail;

#define TIME_TO_STRING(t, buffer) \
  ({ \
    struct tm _tm; \
    time_t __t = t; \
    localtime_r (&__t, &_tm); \
    strftime (buffer, sizeof (buffer), "%F %T", &_tm); \
   })

static void
edb_error (EventDB *edb, const char *error)
{
  puts (error);
}

int
do_test (int argc, char *argv[])
{
  char *file;
  int fd;
  /* 2006-06-20 23:59:30 UTC.  */
  time_t start = 1150847970;
  setenv ("TZ", "UTC", 1);
  tzset ();

  /* Initialize the g_object system.  */
  g_type_init ();

  fd = create_temp_file (".", &file);
  /* event_db_new will open it itself.  */
  close (fd);

  GError *err = NULL;
  EventDB *edb = event_db_new (file, &err);
  if (! edb)
    error (1, 0, "evend_db_new: %s", err->message);

  g_signal_connect (G_OBJECT (edb),
		    "error", G_CALLBACK (edb_error), NULL);

  Event *ev = event_new (edb, NULL, NULL, NULL);
  event_set_recurrence_start (ev, start, NULL);
  event_set_duration (ev, 60, NULL);
  event_set_summary (ev, "timed event", NULL);
  g_object_unref (ev);

  ev = event_new (edb, NULL, NULL, NULL);
  /* Starts the next day.  */
  event_set_recurrence_start (ev, start + 30, NULL);
  event_set_untimed (ev, TRUE, NULL);
  /* One day.  */
  event_set_duration (ev, 24 * 60 * 60, NULL);
  event_set_summary (ev, "untimed event", NULL);
  g_object_unref (ev);

  /* Check the bounds checking of event_db_list_for period.  An event
     should be returned if it at all occurs between START and END
     inclusive.  EV starts at START and occurs until (and excluding)
     START + 60.  */
  time_t t;
  for (t = start - 5; t < start + 60 + 5; t ++)
    {
      GSList *list = event_db_list_for_period (edb, t, t, NULL);

      char buffer[200];
      TIME_TO_STRING (t, buffer);
      puts (buffer);

      GSList *l;
      list = g_slist_sort (list, event_compare_func);
      for (l = list; l; l = l->next)
	{
	  Event *ev = EVENT (l->data);
	  TIME_TO_STRING (event_get_start (ev), buffer);
	  printf ("%s: %s\n", buffer, event_get_summary (ev, NULL));
	}

      event_list_unref (list);
    }

  return fail;
}

