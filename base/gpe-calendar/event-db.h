/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_DB_H
#define EVENT_DB_H

#include <glib.h>
#include <time.h>

typedef enum
{
  RECUR_NONE,
  RECUR_DAILY,
  RECUR_WEEKLY,
  RECUR_MONTHLY,
  RECUR_YEARLY
} recur_t;

typedef struct event_details_s
{
  char *summary;
  char *description;
} *event_details_t;

typedef struct event_s
{
  unsigned long uid;

  time_t start;
  unsigned long duration;	/* 0 == instantaneous */
  unsigned long alarm;		/* -1 == none */
  
  struct
  {
    recur_t type;

    unsigned int count;
    unsigned int daymask;	/* bit 0 = Mon, bit 1 = Tue, etc */

    time_t end;			/* 0 == perpetual */
  } recur;

  event_details_t details;
  
  gboolean mark;
} *event_t;

extern gboolean event_db_start (void);
extern gboolean event_db_stop (void);

extern gboolean event_db_add (event_t);
extern gboolean event_db_remove (event_t);

extern event_t event_db_new (void);
extern void event_db_destroy (event_t);

extern event_details_t event_db_alloc_details (event_t);
extern event_details_t event_db_get_details (event_t);
extern void event_db_forget_details (event_t);

extern GSList *event_db_list_for_period (time_t start, time_t end);

#endif
