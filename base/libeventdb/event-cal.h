/* event-cal.h: Event Calendar Interface.
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_CAL_H
#define EVENT_CAL_H

#include <glib-object.h>
#include "gpe/event-db.h"

typedef struct
{
  GObjectClass gobject_class;
} EventCalendarClass;

struct _EventCalendar
{
  GObject object;

  EventDB *edb;

#define EVENT_CALENDAR_NO_PARENT ((guint) -1)
  guint parent_uid;
  /* This is a cache of the resolved PARENT_UID; it holds a
     reference.  */
  EventCalendar *parent;

  guint uid;

  gboolean hidden;

  char *title;
  char *description;
  char *url;
  char *username;
  char *password;

  gboolean has_color;
  guint16 red;
  guint16 green;
  guint16 blue;

  int mode;
  int sync_interval;
  time_t last_pull;
  time_t last_push;

  gboolean modified;
  time_t last_modified;

  /* True if a calendar attribute has changed which does not affect
     how the calendar contents (e.g. color).  */
  gboolean changed;
};

/* Flush any changes to EV to backing store immediately.  */
void event_calendar_flush (EventCalendar *ec)
     __attribute__ ((visibility ("hidden")));

#define MODIFIED(_ec) \
  do \
    { \
      time_t now = time (NULL); \
      EventCalendar *_p = (_ec); \
      do \
        { \
          _p->last_modified = now; \
	  if (! _p->modified) \
            { \
              _p->modified = TRUE; \
              add_to_laundry_pile (G_OBJECT (_p)); \
            } \
          if ((_ec) != _p) \
            g_object_unref (_p); \
        } \
      while ((_p = event_calendar_get_parent (_p))); \
    } \
  while (0)
       
#endif
