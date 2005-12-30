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

#define SECONDS_IN_DAY (24*60*60)

#include <glib.h>
#include <time.h>

#define MON  (1 << 0)
#define TUE  (1 << 1)
#define WED  (1 << 2)
#define THU  (1 << 3)
#define FRI  (1 << 4)
#define SAT  (1 << 5)
#define SUN  (1 << 6)

typedef enum
{
  RECUR_NONE,
  RECUR_DAILY,
  RECUR_WEEKLY,
  RECUR_MONTHLY,
  RECUR_YEARLY
} recur_type_t;

typedef struct event_details_s
{
  guint ref;

  unsigned long sequence;
  time_t modified;

  gchar *summary;
  gchar *description;
  gchar *location;  
  
  GSList *categories;
} *event_details_t;

#define FLAG_UNTIMED   (1 << 0)
#define FLAG_ALARM     (1 << 1)
#define FLAG_TENTATIVE (1 << 2)
#define FLAG_CLONE     (1 << 3)
#define FLAG_RECUR     (1 << 4)

struct calendar_time_s
{
  GDate date;
  GTime time;
};

typedef time_t calendar_time_t;

typedef struct recur_s
{
  recur_type_t type;
  
  unsigned int count;
  unsigned int increment;
  unsigned long daymask;	/* bit 0 = Mon, bit 1 = Tue, etc */
  GSList *exceptions;
  
  time_t end;			/* 0 == perpetual */
} *recur_t;

typedef struct event_s
{
  unsigned long uid;

  calendar_time_t start;
  unsigned long duration;	/* 0 == instantaneous */
  unsigned long alarm;		/* seconds before event */
  unsigned long flags;

  recur_t recur;
  
  event_details_t details;
  gboolean mark;
  gpointer *cloned_ev;
  char *eventid; 
} *event_t;

#define EVENT_DB_USE_MEMCHUNK

#ifdef EVENT_DB_USE_MEMCHUNK

extern GMemChunk *event_chunk, *recur_chunk;

#define event_db__alloc_event()		\
	(event_t)g_mem_chunk_alloc0 (event_chunk)

#define event_db__alloc_recur()		\
	(recur_t)g_mem_chunk_alloc0 (recur_chunk)

#define event_db__free_event(_x)	\
	g_mem_chunk_free (event_chunk, _x)

#define event_db__free_recur(_x)	\
	g_mem_chunk_free (recur_chunk, _x)

#else

#define event_db__alloc_event()		\
	(event_t)g_malloc0 (sizeof (struct event_s))

#define event_db__alloc_recur()		\
	(recur_t)g_malloc0 (sizeof (struct recur_s))

#define event_db__free_event(_x)	\
	g_free (_x)

#define event_db__free_recur(_x)	\
	g_free (_x)

#endif

extern gboolean event_db_start (void);
extern gboolean event_db_refresh (void);
extern gboolean event_db_stop (void);

extern gboolean event_db_add (event_t);
extern gboolean event_db_remove (event_t);

extern gboolean event_db_flush (event_t ev);

extern event_t event_db_clone (event_t);
extern event_t event_db_new (void);
extern event_t event_db_find_by_uid (guint uid);
extern void event_db_destroy (event_t);

extern event_details_t event_db_alloc_details (event_t);
extern event_details_t event_db_get_details (event_t);
extern void event_db_forget_details (event_t);

extern GSList *event_db_list_for_period (time_t start, time_t end);
extern GSList *event_db_list_alarms_for_period (time_t start, time_t end);
extern GSList *event_db_list_for_future (time_t start, guint max);
extern GSList *event_db_untimed_list_for_period (time_t start, time_t end, gboolean yes);
extern void event_db_list_destroy (GSList *);

extern recur_t event_db_get_recurrence (event_t ev);

#endif
