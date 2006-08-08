/* event-db.h: Event Database Interface.
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_H
#define EVENT_H

#include <glib-object.h>
#include <glib.h>
#include "gpe/event-db.h"

/* An Event is actually a hollow skeleton.  All of the data is held in
   the referenced EventSource (which is derived from an Event).  */
struct _EventClass
{
  GObjectClass gobject_class;
};

struct _Event
{
  GObject object;

  struct _EventSource *clone_source;
  /* Whether the event is deleted.  */
  gboolean dead;

  time_t start;
};

typedef struct _EventSourceClass EventSourceClass;
typedef struct _EventSource EventSource;

extern GType event_source_get_type (void);

#define TYPE_EVENT_SOURCE             (event_source_get_type ())
#define EVENT_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EVENT_SOURCE, EventSource))
#define EVENT_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EVENT_SOURCE, EventSourceClass))
#define IS_EVENT_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EVENT_SOURCE))
#define IS_EVENT_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EVENT_SOURCE))
#define EVENT_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EVENT_SOURCE, EventSourceClass))

struct _EventSourceClass
{
  GObjectClass gobject_class;
};

struct _EventSource
{
  Event event;

  /* The EventDB to which event belongs.  */
  EventDB *edb;

  gboolean untimed;

  /* If this in memory version has been modified (and thus needs to be
     flushed to disk).  */
  gboolean modified;
  time_t last_modified;

  /* When the last extant user reference was removed.  */
  time_t dead_time;

  /* The event calendar to which this event belongs.  */
  guint calendar;

  unsigned long uid;

  unsigned long duration;	/* 0 == instantaneous */
  unsigned long alarm;		/* seconds before event */

  /* iCal's FREQ property.  */
  enum event_recurrence_type type;

  /* No recurrences beyond this time.  0 means forever.  */
  time_t end;

  char *eventid;

  /* Recurrence properties.  */

  /* The number of times this recurrence set is expanded.  0 means
     there is no limit.  */
  unsigned int count;
  /* iCal's interval property: the number of units to skip.  If the
     recurrence type is RECUR_YEARLY then the first recurrence occurs
     INCREMENT years after the initial start.  */
  unsigned int increment;

  /* List of strings of the form:
     [+-]([0-9]*[1-9]|)(MO|TU|WE|TH|FR|SA|SU).  */
  GSList *byday;

  /* A list of start times to exclude.  Must match the start of a
     recurrence exactly.  */
  GSList *exceptions;

  gboolean details;
  /* Fields below here are only valid if DETAILS is true.  */

  gchar *summary;
  gchar *description;
  gchar *location;  
  /* List of integers.  */
  GSList *categories;

  /* Sequence number.  */
  unsigned long sequence;
};

/* List up to MAX (0 means unlimited) instances of EV which occur
   between PERIOD_START and PERIOD_END, inclusive.  If PER_ALARM is
   true then the instances which have an alarm which goes off between
   PERIOD_START and PERIOD_END are returned.  */
extern GSList *event_list (EventSource *ev,
			   time_t period_start, time_t period_end, int max,
			   gboolean per_alarm)
     __attribute__ ((visibility ("hidden")));

#define LIVE(ev) (g_assert (! EVENT (ev)->dead))

/* Marks the event as well as the calendar in which lives as
   modified.  */
#define STAMP(ev) \
  do \
    { \
      event_details (ev, TRUE); \
      ev->last_modified = time (NULL); \
      if (! ev->modified) \
        { \
          ev->modified = TRUE; \
          add_to_laundry_pile (G_OBJECT (ev)); \
        } \
      EventCalendar *ec = event_get_calendar (EVENT (ev)); \
      MODIFIED (ec); \
      g_object_unref (ec); \
    } \
  while (0)

/* Returns the underlying EventSource.  */
#define RESOLVE_CLONE(ev) \
  ((ev)->clone_source ? EVENT_SOURCE (ev->clone_source) : EVENT_SOURCE (ev))

#endif
