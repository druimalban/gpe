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

int
do_test (int argc, char *argv[])
{
  char *file;
  int fd;
  time_t now = time (NULL);
  struct tm now_tm;
  localtime_r (&now, &now_tm);
  /* Wire to 7 in the morning to avoid day roll overs.  */
  now_tm.tm_hour = 7;
  now_tm.tm_min = 0;
  now_tm.tm_sec = 0;
  now = mktime (&now_tm);

  /* Initialize the g_object system.  */
  g_type_init ();

  fd = create_temp_file (".", &file);
  /* event_db_new will open it itself.  */
  close (fd);

  EventDB *edb = event_db_new (file);
  if (! edb)
    error (1, 0, "evend_db_new");

#define INTERVAL 60 * 60

  /* One time event -- no alarm.  */
  Event *ev = event_new (edb, NULL);
  event_set_recurrence_start (ev, now);
#define D 100
  event_set_duration (ev, D);
  event_flush (ev);

  /* One time event -- alarm.  */
  Event *ev2 = event_new (edb, NULL);
#define ALARM 10
  event_set_recurrence_start (ev2, now + INTERVAL * 2 + ALARM * 2);
  event_set_duration (ev2, D);
  event_set_alarm (ev2, ALARM);
  event_flush (ev2);

  /* Daily event -- alarm.  */
  Event *ev3 = event_new (edb, NULL);
  event_set_recurrence_start (ev3, now + ALARM);
  event_set_duration (ev3, 1);
  event_set_alarm (ev3, ALARM);
  event_set_recurrence_type (ev3, RECUR_DAILY);
#define COUNT 10
  event_set_recurrence_count (ev3, COUNT);
  event_flush (ev3);

  int seen[3] = { 0, 0, 0 };

  /* Check the bounds checking of event_db_list_for period.  An event
     should be returned if it at all occurs between START and END
     inclusive.  EV starts at NOW and occurs until (and including) NOW
     + 99.  */
  time_t start;
  for (start = now;
       start <= now + 2 * COUNT * 24 * 60 * 60;
       start += INTERVAL)
    {
      time_t end = start + INTERVAL - 1;
      GSList *list
	= event_db_list_alarms_for_period (edb, start, end);

      int count[3] = { 0, 0, 0 };
      GSList *e;
      for (e = list; e; e = g_slist_next (e))
	if (event_get_uid (e->data) == event_get_uid (ev))
	  count[0] ++;
	else if (event_get_uid (e->data) == event_get_uid (ev2))
	  count[1] ++;
	else if (event_get_uid (e->data) == event_get_uid (ev3))
	  count[2] ++;
	else
	  {
	    fail = 1;
	    printf ("Unknown and unexpected event with id %ld (%ld-%ld: %ld)\n",
		    event_get_uid (e->data), start, end, start - now);
	  }

      if (count[0])
	{
	  fail = 1;
	  printf ("Unexpectedly got event 1 (%ld-%ld: %ld; count: %d)\n",
		  start, end, start - now, count[0]);
	}

      if (start <= now + INTERVAL * 2 && now + INTERVAL * 2 <= end)
	{
	  if (count[1] != 1)
	    {
	      printf ("Expected 1 instance of event 2 but saw %d (%ld-%ld: %ld)\n",
		      count[1], start, end, start - now);
	      fail = 1;
	    }
	}
      else if (count[1])
	{
	  printf ("Unexpectedly got event 2 (%ld-%ld: %ld; count: %d)\n",
		  start, end, start - now, count[1]);
	  fail = 1;
	}

      struct tm s_tm;
      localtime_r (&start, &s_tm);
      struct tm e_tm;
      localtime_r (&end, &e_tm);

      int expected = (s_tm.tm_hour == now_tm.tm_hour
		      || e_tm.tm_hour == now_tm.tm_hour) ? 1 : 0;
      if (seen[2] == 10)
	expected = 0;

      if (count[2] != expected)
	{
	  printf ("Expected %d instances of event 3, saw %d (%ld-%ld: %ld)\n",
		  expected, count[2], start, end, start - now);
	  fail = 1;
	}

      event_list_unref (list);

      int i;
      for (i = 0; i < 3; i ++)
	if (count[i])
	  seen[i] ++;
    }

  return fail;
}

