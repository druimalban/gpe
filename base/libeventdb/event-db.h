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

#ifndef EVENT_DB_H
#define EVENT_DB_H

#include <glib-object.h>
#include <glib.h>
#include <sqlite.h>
#include <time.h>

typedef struct
{
  GObjectClass gobject_class;

  /* Signals.  */
  guint calendar_new_signal;
  EventCalendarNew calendar_new;
  guint calendar_deleted_signal;
  EventCalendarDeleted calendar_deleted;
  guint calendar_reparented_signal;
  EventCalendarReparented calendar_reparented;
  guint calendar_changed_signal;
  EventCalendarChanged calendar_changed;
  guint calendar_modified_signal;
  EventCalendarModified calendar_modified;
  
  guint event_new_signal;
  EventNew event_new;
  guint event_removed_signal;
  EventRemoved event_removed;
  guint event_modified_signal;
  EventModified event_modified;

  guint alarm_fired_signal;
  EventDBAlarmFiredFunc alarm_fired;
} EventDBClass;

struct _EventDB
{
  GObject object;

  sqlite *sqliteh;

  GHashTable *events;
  guint default_calendar;
  GSList *calendars;

  /* A list of events that need to be flushed to backing store.  */
  GSList *laundry_list;
  /* The idle source.  */
  guint laundry_buzzer;

  /* A list of events which have no user references.  */
  GSList *cache_list;
  /* Number of seconds to leave unreferenced items in memory before
     deallocating them.  */
#ifndef CACHE_EXPIRE
#define CACHE_EXPIRE 120
#endif
  /* A timer source to periodically flush the cache.  */
  guint cache_buzzer;

  /* The list of events with upcoming alarms.  We hold a reference to
     each.  */
  GSList *upcoming_alarms;
  /* EVENTS contains alarms until this point in time.  */
  time_t period_end;

  /* The alarm source.  */
  guint alarm;

  /* The point through which alarms have been fired (when an alarm
     fires, it is entered into the alarms_unacknowledged table and
     only removed once it has been acknowledged).  */
  time_t alarms_fired_through;
};

/* E is an Event or an EventCalendar and is dirty (i.e. needs to be
   written to disk).  Add it to the laundry list and do it when we are
   idle.  */
extern void add_to_laundry_pile (GObject *e)
     __attribute__ ((visibility ("hidden")));

/* Invoked by a timeout source when either an alarm should go off or
   when we need to look for additional upcoming events.  */
extern gboolean buzzer (gpointer data)
     __attribute__ ((visibility ("hidden")));

/* All EventSources are created with a toggle reference.  When the
   last user reference is dropped, this function is invoked and we
   grab a real reference and stick the event on the EDB->CACHE_LIST
   list.  If no user reference is taken before CACHE_EXPIRE seconds,
   the event is freed.  */
extern void event_source_toggle_ref_notify (gpointer data,
					    GObject *object,
					    gboolean is_last_ref)
     __attribute__ ((visibility ("hidden")));

/* Execute the sqlite_exec (or sqlite_exec_printf) statement.  If it
   fails because the database or table is locked, try a few times
   before completely failing.  */
#define SQLITE_TRY(statement) \
  ({ \
    int _tries = 0; \
    int _ret; \
    for (;;) \
      { \
        _ret = statement; \
        if ((_ret == SQLITE_BUSY || _ret == SQLITE_LOCKED) && _tries < 3) \
  	{ \
  	  g_main_context_iteration (NULL, FALSE); \
  	  sleep (1); \
  	  _tries ++; \
  	} \
        else \
  	  break; \
      } \
    _ret; \
  })

#include <unistd.h>

/* Execute the sqlite_exec (or sqlite_exec_printf) statement.  If it
   fails because the database or table is locked, try a few times
   before completely failing.  */
#define SQLITE_TRY(statement) \
  ({ \
    int _tries = 0; \
    int _ret; \
    for (;;) \
      { \
        _ret = statement; \
        if ((_ret == SQLITE_BUSY || _ret == SQLITE_LOCKED) && _tries < 3) \
  	{ \
  	  g_main_context_iteration (NULL, FALSE); \
  	  sleep (1); \
  	  _tries ++; \
  	} \
        else \
  	  break; \
      } \
    _ret; \
  })

#endif
