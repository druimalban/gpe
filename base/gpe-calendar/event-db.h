/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

typedef enum
{
  RECUR_NONE,
  RECUR_DAILY,
  RECUR_WEEKLY,
  RECUR_MONTHLY,
  RECUR_YEARLY
} recur_t;

struct gpe_calendar_event
{
  unsigned long uid;

  time_t start;
  unsigned long duration;	/* -1 == instantaneous */
  unsigned long alarm;		/* -1 == none */
  
  struct
  {
    recur_t type;

    unsigned int count;
    unsigned int daymask;	/* bit 0 = Mon, bit 1 = Tue, etc */

    gboolean perpetual;
    time_t end;
  } recur;
};
