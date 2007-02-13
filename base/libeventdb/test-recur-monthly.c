/* test-recur-monthly.c - Test monthly recurrences.
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
    strftime (buffer, sizeof (buffer), "%F", &_tm); \
   })

int
do_test (int argc, char *argv[])
{
  char *file;
  int fd;
  struct tm tm;

  setenv ("TZ", "UTC", 1);
  tzset ();

  memset (&tm, 0, sizeof (tm));
  /* January 1, 2006 at 13:00.  */
  tm.tm_year = 2006 - 1900;
  tm.tm_mon = 0;
  tm.tm_mday = 1;
  tm.tm_hour = 13;
  time_t start = mktime (&tm);

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

  Event *ev = event_new (edb, NULL, NULL, NULL);
  event_set_summary (ev, "Every four months, two instances", NULL);
  event_set_recurrence_start (ev, start, NULL);
  event_set_duration (ev, 60, NULL);
  event_set_recurrence_type (ev, RECUR_MONTHLY, NULL);
  event_set_recurrence_increment (ev, 4, NULL);
  event_set_recurrence_count (ev, 2, NULL);
  g_object_unref (ev);

  /* Every month until November.  */
  ev = event_new (edb, NULL, NULL, NULL);
  struct tm end = tm;
  end.tm_mon = 10;
  time_t e = mktime (&end) + 1;
  char buffer[200];
  TIME_TO_STRING (e, buffer);
  char summary[200];
  sprintf (summary, "Monthly until %s", buffer);
  event_set_summary (ev, summary, NULL);
  event_set_recurrence_start (ev, start, NULL);
  event_set_duration (ev, 60, NULL);
  event_set_recurrence_type (ev, RECUR_MONTHLY, NULL);
  event_set_recurrence_end (ev, e, NULL);
  g_object_unref (ev);

  int i;
  for (i = 0; i < 12; i ++)
    {
      struct tm start = tm;
      start.tm_mon = i;
      time_t s = mktime (&start);
      struct tm end = start;
      end.tm_mday = 28;
      time_t e = mktime (&end);
      GSList *list = event_db_list_for_period (edb, s, e, NULL);

      char buffer[200];
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

