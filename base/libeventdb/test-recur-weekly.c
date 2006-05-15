/* test-recur-weekly.c - Test weekly recurrences.
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
  struct tm tm;
  time_t now;

  memset (&tm, 0, sizeof (tm));
  /* April 18, 2006 (a Tuesday).  */
  tm.tm_year = 2006 - 1900;
  tm.tm_mon = 3;
  tm.tm_mday = 18;
  tm.tm_hour = 13;
  now = mktime (&tm);

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
  /* Every four weeks.  */
  event_set_recurrence_type (ev, RECUR_WEEKLY);
#define EV_INCREMENT 4
  event_set_recurrence_increment (ev, EV_INCREMENT);
#define COUNT 10
  event_set_recurrence_count (ev, COUNT);
  event_flush (ev);

  Event *ev2 = event_new (edb, NULL);
  event_set_recurrence_start (ev2, now);
  event_set_duration (ev2, D);
  /* Every 3 weeks until 10 weeks from start.  */
  event_set_recurrence_type (ev2, RECUR_WEEKLY);
#define EV2_INCREMENT 2
  event_set_recurrence_increment (ev2, EV2_INCREMENT);
#define END now + 10 * 7 * 24 * 60 * 60
  event_set_recurrence_end (ev2, END);
  /* But only sundays, mondays and fridays.  */
  event_set_recurrence_daymask (ev2, SUN | MON | FRI);
  event_flush (ev2);

  int i;
  int expected_ev2_count;
  for (i = 0; i < COUNT * EV_INCREMENT * 2 * 7; i ++)
    {
      time_t t = now + i * 24 * 60 * 60 + 12 * 60 * 60;
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

      if (i < COUNT * EV_INCREMENT * 7 && ev_count != i / EV_INCREMENT / 7 + 1)
	{
	  printf ("%d: Expected %d recurrences of ev but got %d (i: %d)\n",
		  __LINE__, i / EV_INCREMENT / 7 + 1, ev_count, i);
	  fail = 1;
	}
      if (i >= COUNT * EV_INCREMENT * 7 && ev_count != COUNT)
	{
	  printf ("%d: Expected %d recurrences of ev but got %d (i: %d)\n",
		  __LINE__, COUNT, ev_count, i);
	  fail = 1;
	}

      if (t <= END)
	expected_ev2_count = ev_count2;

      if (t <= END)
	{
	  /* 3 per increment.  */
	  int expected = 3 * (i / 7 / EV2_INCREMENT);
	  if (i % (EV2_INCREMENT * 7) >= 3)
	    /* First Friday in increment.  */
	    expected ++;
	  if (i % (EV2_INCREMENT * 7) >= 5)
	    /* First Sunday in increment.  */
	    expected ++;
	  if (i % (EV2_INCREMENT * 7) >= 6)
	    /* First Monday in increment.  */
	    expected ++;

	  if (expected != ev_count2)
	    {
	      printf ("%d: Expected %d recurrences of ev2 but got %d (i: %d)\n",
		      __LINE__, expected, ev_count2, i);
	      fail = 1;
	    }
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

