/* test-recur-daily.c - Test daily recurrences.
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
  event_set_start (ev, now);
#define D 100
  event_set_duration (ev, D);
  recur_t r = event_get_recurrence (ev);
  /* Every other day.  */
  r->type = RECUR_DAILY;
#define INCREMENT 2
  r->increment = INCREMENT;
#define COUNT 10
  r->count = COUNT;
  event_flush (ev);

  Event *ev2 = event_new (edb, NULL);
  event_set_start (ev2, now + D);
  event_set_duration (ev2, D);
  r = event_get_recurrence (ev2);
  /* Every day until 10 days from start.  */
  r->type = RECUR_DAILY;
#define END now + 10 * 24 * 60 * 60
  r->end = END;
  event_flush (ev2);

  int i;
  int expected_ev2_count;
  for (i = 0; i < COUNT * INCREMENT * 2; i ++)
    {
      /* XXX: Day light savings time...  */
      time_t t = now + i * 24 * 60 * 60;
      GSList *list = event_db_list_for_period (edb, now, t);

      GSList *e;
      int ev_count = 0;
      int ev_count2 = 0;
      for (e = list; e; e = g_slist_next (e))
	if (event_get_uid (e->data) == event_get_uid (ev))
	  ev_count ++;
	else if (event_get_uid (e->data) == event_get_uid (ev2))
	  ev_count2 ++;
	else
	  {
	    fail = 1;
	    printf ("Unknown and unexpected event with id %ld (i: %d)\n",
		    event_get_uid (e->data), i);
	  }

      if (i < COUNT * INCREMENT && ev_count != i / INCREMENT + 1)
	{
	  printf ("%d: Expected %d recurrences of ev but got %d (i: %d)\n",
		  __LINE__, i / INCREMENT + 1, ev_count, i);
	  fail = 1;
	}
      if (i >= COUNT * INCREMENT && ev_count != COUNT)
	{
	  printf ("%d: Expected %d recurrences of ev but got %d (i: %d)\n",
		  __LINE__, COUNT, ev_count, i);
	  fail = 1;
	}

      if (t <= END)
	expected_ev2_count = ev_count2;

      if (t <= END && ev_count2 != i)
	{
	  printf ("%d: Expected %d recurrences of ev2 but got %d (i: %d)\n",
		  __LINE__, i, ev_count2, i);
	  fail = 1;
	}
      if (t >= END && ev_count2 != expected_ev2_count)
	{
	  printf ("%d: Expected %d recurrences of ev2 but got %d (i: %d)\n",
		  __LINE__, expected_ev2_count, ev_count2, i);
	  fail = 1;
	}

      event_list_unref (list);
    }

  return fail;
}

