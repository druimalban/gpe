/* test-alarms.c - Test the event_db_list_alarms_for_period.
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

int
do_test (int argc, char *argv[])
{
  char *file;
  int fd;
  /* Tue Jun 20 07:00:00 UTC 2006.  */
  time_t start = 1150786800;
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

  void edb_error (EventDB *edb, const char *error)
    {
      puts (error);
    }
  g_signal_connect (G_OBJECT (edb),
		    "error", G_CALLBACK (edb_error), NULL);

  /* One hour.  */
#define INTERVAL 60 * 60
  /* Ten minutes.  */
#define ALARM 10 * 60

  /* One time event -- no alarm.  */
  Event *ev = event_new (edb, NULL, NULL, NULL);
  event_set_summary (ev, "no alarm", NULL);
  event_set_recurrence_start (ev, start, NULL);
  event_set_duration (ev, 60, NULL);
  g_object_unref (ev);

  /* One time event -- alarm.  */
  ev = event_new (edb, NULL, NULL, NULL);
  event_set_summary (ev, "once at 7:10", NULL);
  event_set_recurrence_start (ev, start + ALARM, NULL);
  event_set_duration (ev, 60, NULL);
  event_set_alarm (ev, ALARM, NULL);
  g_object_unref (ev);

  /* Daily event -- alarm.  */
  ev = event_new (edb, NULL, NULL, NULL);
  event_set_summary (ev, "daily at 7, twice", NULL);
  event_set_recurrence_start (ev, start, NULL);
  event_set_duration (ev, 1, NULL);
  event_set_alarm (ev, ALARM, NULL);
  event_set_recurrence_type (ev, RECUR_DAILY, NULL);
  event_set_recurrence_count (ev, 2, NULL);
  g_object_unref (ev);

  int i;
  for (i = -5; i < 3 * 24; i ++)
    {
      time_t s = start + i * 60 * 60;
      time_t e = s + 60 * 60 - 1;

      printf ("Next alarm after ");
      char buffer[200];
      TIME_TO_STRING (s, buffer);
      printf ("%s", buffer);

      Event *ev = event_db_next_alarm (edb, s, NULL);
      if (! ev)
	printf (" none\n");
      else
	{
	  printf (" %s at ", event_get_summary (ev, NULL));
	  TIME_TO_STRING (event_get_start (ev) - event_get_alarm (ev),
			  buffer);
	  printf ("%s\n", buffer);

	  g_object_unref (ev);
	}

      GSList *list
	= event_db_list_alarms_for_period (edb, s, e, NULL);

      TIME_TO_STRING (s, buffer);
      fputs (buffer, stdout);
      fputs (" until ", stdout);
      TIME_TO_STRING (e, buffer);
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

