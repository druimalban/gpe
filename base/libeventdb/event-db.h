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
#include <time.h>

#include "event-cal.h"
#include "event.h"

#define ERROR_DOMAIN() g_quark_from_static_string ("libeventdb")
/* If an error occurs and the passed GError is NULL, we propagate
   error by emitting a signal on EDB.  This must be used instead of
   g_set_error.  */
#define SIGNAL_ERROR(edb, gerror, args , ...) \
  do \
    { \
      char *__e = g_strdup_printf (args, ##__VA_ARGS__); \
      if (gerror) \
        g_set_error (gerror, ERROR_DOMAIN (), 0, \
                     "%s: %s", __func__, __e); \
      else \
        { \
          char *buffer = g_strdup_printf ("%s: %s", __func__, __e); \
          g_signal_emit \
            (edb, EVENT_DB_GET_CLASS (edb)->error_signal, 0, buffer); \
          g_free (buffer); \
        } \
      g_free (__e); \
    } \
  while (0)
#define SIGNAL_ERROR_GERROR(edb, dest, src) \
  do \
    { \
      if ((dest)) \
        g_propagate_error ((dest), (src)); \
      else \
        { \
          char *buffer = g_strdup_printf ("%s:%s", __func__, (src)->message); \
          g_error_free ((src)); \
          g_signal_emit \
            ((edb), EVENT_DB_GET_CLASS (edb)->error_signal, 0, buffer); \
          g_free (buffer); \
        } \
    } \
  while (0)

/* Enumerate the events in the event database EDB which MAY occur
   between (PERIOD_START and PERIOD_END].  If ALARMS is true, returns
   those events which have an alarm which MAY go off between
   PERIOD_START and PERIOD_END.  Calls CB on each event until CB
   returns a non-zero value.  A reference to EV is allocated and the
   callback function must consume it.  */
typedef void (*events_enumerate_t) (EventDB *edb,
				    time_t period_start, time_t period_end,
				    gboolean alarms,
				    int (*cb) (EventSource *ev),
				    GError **error);
/* Returns the event with the event id EVENTID or NULL if there is no
   such event.  */
typedef int (*eventid_to_uid_t) (EventDB *edb, const char *eventid,
				 GError **error);
/* Set the default calendar to EC.  */
typedef void (*set_default_calendar_t) (EventDB *edb, EventCalendar *ec,
					GError **error);

/* Create a new event on backing store.  EV is uninitialized; save the
   allocated UID in EV->UID.  */
typedef gboolean (*event_new_t) (EventSource *ev, GError **error);
/* Load event EV's basic data (i.e. not necessarily its details) from
   backing store.  EV->UID is valid.  Returns FALSE if no such event
   exists.  */
typedef gboolean (*event_load_t) (EventSource *ev, GError **error);
/* Load the event's details from backing store.  event_load has
   already been called successfully.  */
typedef void (*event_load_details_t) (EventSource *ev, GError **error);
/* Flush the event to backing store.  If an error occurs, FALSE should
   be returned and an error may be reported in *ERR which the client
   must free using g_free.  */
typedef gboolean (*event_flush_t) (EventSource *ev, GError **error);
/* Mark the event EV as removed (i.e. deleted).  */
typedef void (*event_remove_t) (EventSource *ev, GError **error);

/* List the events which have alarms which have not yet been
   acknowledged.  */
typedef GSList *(*list_unacknowledged_alarms_t) (EventDB *edb, GError **error);
/* Mark EV's alarm as having fired but yet not been acknowledged.  */
typedef void (*event_mark_unacknowledged_t) (EventSource *ev, GError **error);
/* Mark EV's alarm as having been acknowledged.  */
typedef void (*event_mark_acknowledged_t) (EventSource *ev, GError **error);
/* Mark all alarms having fired through T as having been
   acknowledged.  */
typedef void (*acknowledge_alarms_through_t) (EventDB *edb, time_t t,
					      GError **error);

/* Create a new calendar on the backing store using EC as the
   template.  Must initialize EC->UID to an identifier unique to the
   backing store.  */
typedef void (*event_calendar_new_t) (EventCalendar *ec, GError **error);
/* Flush the calendar to backing store.  */
typedef void (*event_calendar_flush_t) (EventCalendar *ec, GError **error);
/* Delete the calendar from the backing store.  The generic framework
   has already taken care that no dangling references are left
   around.  */
typedef void (*event_calendar_delete_t) (EventCalendar *ec, GError **error);
/* List the events in the calendar EC.  If MODIFIED_AFTER is non-zero,
   limit to those events which have been modified on or after
   MODIFIED_AFTER.  If MODIFIED_BEFORE is non-zero, limit to those
   events which have been modified on or before MODIFIED_BEFORE.  */
typedef GSList *(*event_calendar_list_events_t) (EventCalendar *ec,
						 time_t modified_after,
						 time_t modified_before,
						 GError **error);
/* List the deleted events in the calendar EC.  NB: Deleted events
   have EV->DEAD set to true.  */
typedef GSList *(*event_calendar_list_deleted_t) (EventCalendar *ec,
						  GError **error);
/* Flush any deleted events from EC.  */
typedef void (*event_calendar_flush_deleted_t) (EventCalendar *ec,
						GError **error);

typedef struct
{
  GObjectClass gobject_class;

  events_enumerate_t events_enumerate;
  eventid_to_uid_t eventid_to_uid;
  set_default_calendar_t set_default_calendar;

  event_new_t event_new;
  event_load_t event_load;
  event_load_details_t event_load_details;
  event_flush_t event_flush;
  event_remove_t event_remove;

  list_unacknowledged_alarms_t list_unacknowledged_alarms;
  event_mark_unacknowledged_t event_mark_unacknowledged;
  event_mark_acknowledged_t event_mark_acknowledged;
  acknowledge_alarms_through_t acknowledge_alarms_through;

  event_calendar_new_t event_calendar_new;
  event_calendar_flush_t event_calendar_flush;
  event_calendar_delete_t event_calendar_delete;
  event_calendar_list_events_t event_calendar_list_events;
  event_calendar_list_deleted_t event_calendar_list_deleted;
  event_calendar_flush_deleted_t event_calendar_flush_deleted;

  /* Signals.  */
  guint error_signal;
  EventDBError error;

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
  EventNew event_new_func;
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

#endif
