/*
 * Copyright (C) 2001 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_H
#define EVENT_H

typedef enum
{
  EVENT_NONE,
  EVENT_MINUTE,
  EVENT_HOUR,
  EVENT_DAY,
  EVENT_WEEK,
  EVENT_MONTH,
  EVENT_YEAR,
} event_period_t;

typedef struct event_s
{
  time_t start, end;
  guint has_time : 1;
  guint has_end : 1;

  event_period_t alarm_type;
  event_period_t repeat_type;
  guint alarm_val;
  guint repeat_val;
  time_t repeat_until;

  gchar *text;
  struct event_s *next;
  struct event_s *next_a;
  struct event_s *prev;
  struct event_s *prev_a;
} *event_t;

extern void events_init (void);
extern event_t get_events (time_t start, time_t end);
extern void free_events (event_t ev);
extern void event_add (event_t ev);
extern void event_delete (event_t ev);
extern time_t next_alarm (void);
extern void run_alarms (void);

#endif
