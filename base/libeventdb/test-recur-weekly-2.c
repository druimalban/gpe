/* test-recur-weekly-2.c - Test weekly recurrence with a count limit.
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
    strftime (buffer, sizeof (buffer), "%a %F", &_tm); \
   })

int
do_test (int argc, char *argv[])
{
  char *file;
  int fd;
  /* August 15, 2006 0:00 UTC.  */
  time_t start = 1155600000;
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

  /* Every four weeks, ten occurrences.  */
  Event *ev = event_new (edb, NULL, NULL, NULL);
  event_set_summary (ev, "MTWF, 8 occurrences", NULL);
  /* At 14:00.  */
  event_set_recurrence_start (ev, start + 14 * 60 * 60, NULL);
  event_set_duration (ev, 60 * 60, NULL);
  event_set_recurrence_type (ev, RECUR_WEEKLY, NULL);
  event_set_recurrence_count (ev, 8, NULL);
  GSList *byday = g_slist_prepend (NULL, g_strdup ("MO"));
  byday = g_slist_prepend (byday, g_strdup ("TU"));
  byday = g_slist_prepend (byday, g_strdup ("WE"));
  byday = g_slist_prepend (byday, g_strdup ("FR"));
  event_set_recurrence_byday (ev, byday, NULL);
  g_object_unref (ev);
  
  int i;
  for (i = 0; i < 30; i ++)
    {
      time_t s = start + i * 24 * 60 * 60;
      time_t e = s + 24 * 60 * 60 - 1;

      GSList *list = event_db_list_for_period (edb, s, e, NULL);

      printf ("For day %i: ", i);
      char buffer[200];
      TIME_TO_STRING (s, buffer);
      fputs (buffer, stdout);
      fputs ("\n", stdout);

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

  time_t s = start;
  time_t e = s + 30 * 24 * 60 * 60 - 1;

  GSList *list = event_db_list_for_period (edb, s, e, NULL);

  fputs ("For next 30 days starting at ", stdout);
  char buffer[200];
  TIME_TO_STRING (s, buffer);
  fputs (buffer, stdout);
  fputs ("\n", stdout);

  GSList *l;
  list = g_slist_sort (list, event_compare_func);
  for (l = list; l; l = l->next)
    {
      Event *ev = EVENT (l->data);
      TIME_TO_STRING (event_get_start (ev), buffer);
      printf ("%s: %s\n", buffer, event_get_summary (ev, NULL));
    }

  event_list_unref (list);

  return fail;
}

