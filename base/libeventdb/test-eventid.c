/* test-eventid.c - Test eventid lookup.
   Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>

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

  EventDB *edb = event_db_new (file);
  if (! edb)
    error (1, 0, "evend_db_new");

  Event *ev = event_new (edb, NULL, NULL);
  event_set_recurrence_start (ev, start);
  event_set_duration (ev, 60);
  event_set_summary (ev, "timed event");
  g_object_unref (ev);

  char *eventid = event_get_eventid (ev);
  g_object_unref (ev);

  ev = event_db_find_by_eventid (edb, eventid);
  if (! ev)
    printf ("lookup found nothing\n");
  else
    {
      char *summary = event_get_summary (ev);
      printf ("lookup: %s\n", summary);
      g_free (summary);
    }
  g_object_unref (ev);

  ev = event_new (edb, NULL, eventid);
  if (ev)
    printf ("created event with duplicate eventid!\n");
  else
    printf ("did not create event with duplicate eventid!\n");

  /* Change the eventid to an non-existent event.  */
  if (eventid[0] == 'a')
    eventid[0] = 'b';
  else
    eventid[0] = 'a';

  ev = event_db_find_by_eventid (edb, eventid);
  if (ev)
    {
      printf ("non-existent eventid returned an event!\n");
      g_object_unref (ev);
    }
  else
    printf ("non-existent eventid returned nothing\n");

  return fail;
}

