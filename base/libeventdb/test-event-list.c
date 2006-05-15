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

int
do_test (int argc, char *argv[])
{
  char *file;
  int fd;
  time_t now = time (NULL);

  /* Initialize the g_object system.  */
  g_type_init ();

  fd = create_temp_file (".", &file);
  /* event_db_new will open it itself.  */
  close (fd);

  EventDB *edb = event_db_new (file);
  if (! edb)
    error (1, 0, "evend_db_new");

  Event *ev = event_new (edb, NULL);
  event_set_recurrence_start (ev, now);
#define D 100
  event_set_duration (ev, D);
  event_flush (ev);

  /* Check the bounds checking of event_db_list_for period.  An event
     should be returned if it at all occurs between START and END
     inclusive.  EV starts at NOW and occurs until (and including) NOW
     + 99.  */
  time_t t;
  for (t = now - 5; t < now + D + 5; t ++)
    {
      GSList *evs = event_db_list_for_period (edb, t, t);

      if (now <= t && t < now + D)
	{
	  if (! evs || evs->next
	      || event_get_uid (ev) != event_get_uid (evs->data))
	    {
	      fail = 1;
	      printf ("%d: evs: %p; next: %p; t: %ld; now: %ld (t - now:%ld)"
		      " uid: %ld =? %ld\n",
		      __LINE__, evs, evs ? evs->next : NULL, t, now, t - now,
		      event_get_uid (ev), evs ? event_get_uid (evs->data) : 0);
	    }
	}
      else
	{
	  if (evs)
	    {
	      fail = 1;
	      printf ("%d: Unexpectedly got an event "
		      "(t: %ld; now: %ld; t-now: %ld)\n",
		      __LINE__, t, now, t - now);
	    }
	}

      event_list_unref (evs);
    }

  return fail;
}

