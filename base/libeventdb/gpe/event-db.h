/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GPE_EVENT_DB_H
#define GPE_EVENT_DB_H

#define SECONDS_IN_DAY (24*60*60)

#include <glib-object.h>
#include <glib.h>
#include <time.h>

/* Add a forward to avoid a dependency on libgdk.  */
struct _GdkColor;

#define MON  (1 << 0)
#define TUE  (1 << 1)
#define WED  (1 << 2)
#define THU  (1 << 3)
#define FRI  (1 << 4)
#define SAT  (1 << 5)
#define SUN  (1 << 6)

enum event_recurrence_type
{
  RECUR_NONE,
  RECUR_DAILY,
  RECUR_WEEKLY,
  RECUR_MONTHLY,
  RECUR_YEARLY,
  /* Number of items in this enum.  Only used internally and subject
     to change if new recurrence types are added.  */
  RECUR_COUNT
};

#define TYPE_EVENT (event_get_type ())
#define EVENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EVENT, Event))
#define EVENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EVENT, EventClass))
#define IS_EVENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EVENT))
#define IS_EVENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EVENT))
#define EVENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EVENT, EventClass))

struct _EventClass;
typedef struct _EventClass EventClass;

struct _Event;
typedef struct _Event Event;

extern GType event_get_type (void);

struct _EventCalendar;
typedef struct _EventCalendar EventCalendar;

#define TYPE_EVENT_CALENDAR (event_calendar_get_type ())
#define EVENT_CALENDAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EVENT_CALENDAR, EventCalendar))
#define EVENT_CALENDAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EVENT_CALENDAR, EventCalendarClass))
#define IS_EVENT_CALENDAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EVENT_CALENDAR))
#define IS_EVENT_CALENDAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EVENT_CALENDAR))
#define EVENT_CALENDAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EVENT_CALENDAR, EventCalendarClass))

extern GType event_calendar_get_type (void);

struct _EventDB;
typedef struct _EventDB EventDB;

#define TYPE_EVENT_DB             (event_db_get_type ())
#define EVENT_DB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EVENT_DB, EventDB))
#define EVENT_DB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EVENT_DB, EventDBClass))
#define IS_EVENT_DB(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EVENT_DB))
#define IS_EVENT_DB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EVENT_DB))
#define EVENT_DB_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EVENT_DB, EventDBClass))

/* Return GType of a view.  */
extern GType event_db_get_type (void);

/* An event db signals the "alarm-fired" signal when an event's alarm
   goes off (but only after a call to
   event_db_list_unacknowledged_alarms).  */
typedef void (*EventDBAlarmFiredFunc) (EventDB *, Event *);

/** Nearly all functions return a GError.  If this GError is NULL,
    i.e., ignored, then the "error" is sent on the corresponding
    EventDB *.  */
typedef void (*EventDBError) (EventDB *, const char *);

/**
  event_db_error_punt

  Propagates an error via the "error" signal.  Useful if you accepted
  an error but now don't want to propagate it via the function call
  chain.  Frees ERROR.
 */
extern void event_db_error_punt (EventDB *edb, GError *error);

/* Open a new database.  */
extern EventDB *event_db_new (const char *filename, GError **error);

/* Open database readonly.  */
extern EventDB *event_db_new (const char *filename, GError **error);

/* Return the event whose alarm will first go off at or after NOW.  */
extern Event *event_db_next_alarm (EventDB *edb, time_t now,
				   GError **error);

/* Return the events in the event database EVD which occur between
   START and END inclusive.  The list is not sorted.  */
extern GSList *event_db_list_for_period (EventDB *evd,
					 time_t start, time_t end,
					 GError **error);
/* Like event_db_list but returns the events whose alarm goes off
   between START and END inclusive.  The list is not sorted.  */
extern GSList *event_db_list_alarms_for_period (EventDB *evd,
						time_t start, time_t end,
						GError **error);
/* Like event_db_list_for_period but only for untimed events (i.e.,
   those with a 0 length duration).  The list is not sorted.  */
extern GSList *event_db_untimed_list_for_period (EventDB *evd,
						 time_t start, time_t end,
						 GError **error);

/* Returns the list of unacknowledged events since EDB was last shut
   down and turns on the "alarm-fire" event.  The list is not sorted.
   Only call this function once at start up after having connected to
   the "alarm-fire" signal.  */
extern GSList *event_db_list_unacknowledged_alarms (EventDB *edb,
						    GError **error);

/* Look up an event by its uid.  Returns the event or NULL if no event
   in the database has uid UID.  */
extern Event *event_db_find_by_uid (EventDB *edb, guint uid,
				    GError **error);
/* Look up an event by its eventid.  Returns the event or NULL if no
   event in the database has eventid EVENTID.  */
extern Event *event_db_find_by_eventid (EventDB *edb, const char *eventid,
					GError **error);

extern EventCalendar *event_db_find_calendar_by_uid (EventDB *edb, guint uid,
						     GError **error);

/**
 * event_db_find_calendar_by_name:
 * @edb: Event database
 * @name: Calendar name
 *
 * Get a desired calendar by name.
 * 
 * Returns: An #EventCalendar object or NULL if not found.
 */
extern EventCalendar *event_db_find_calendar_by_name (EventDB *edb,
						      const gchar *name,
						      GError **error);

/* Returns a list of the event calendars in EDB.  There will always be
   at least one calendar.  Allocates a reference to each calendar.  It
   is the caller's responsibility to free the list.  */
extern GSList *event_db_list_event_calendars (EventDB *edb,
					      GError **error);

/* Returns the default event calendar (calendar events are added to
   unless explicitly overridden).  Allocates a reference.  If the
   default calendar does not exist, it is created with the title
   TITLE.  */
extern EventCalendar *event_db_get_default_calendar (EventDB *edb,
						     const char *title,
						     GError **error);
/* Sets the default event calendar to event calendar EC.  */
extern void event_db_set_default_calendar (EventDB *edb, EventCalendar *ec,
					   GError **error);

/* Signal "calendar-new": emitted when a calendar in EDB is created.  */
typedef void (*EventCalendarNew) (EventDB *edb, EventCalendar *ec);
/* Signal "calendar-deleted": emitted when a calendar in EDB is
   deleted.  */
typedef void (*EventCalendarDeleted) (EventDB *edb, EventCalendar *ec);
/* Signal "calendar-reparented": emitted when a calendar in EDB is
   is reparented.  */
typedef void (*EventCalendarReparented) (EventDB *edb, EventCalendar *ec);
/* Signal "calendar-change": emitted when any attribute of calendar
   (EC) is changed but not the contents: compare with
   "calendar-modified".  The signal is emitted soon after the change
   occurs (but not immediately) thereby allowing multiple changes to
   be collapsed into a single signal.  */
typedef void (*EventCalendarChanged) (EventDB *edb, EventCalendar *ec);
/* Signal "calendar-modified": emitted on each calendar (EC) when it
   or an event or calendar it contains has a change which affects the
   calendar as publiced (i.e. not when color or last_push is changed,
   etc. but when e.g. the title, description, URL, user name or
   password changes).  Hence, EC is the calendar which contained the
   changed.  The signal may not be emitted immediately and multiple
   changes may be collapsed into a single signal.  */
typedef void (*EventCalendarModified) (EventDB *edb, EventCalendar *ec);

/* Returns the event db (without allocating a reference) in which EC
   lives.  */
extern EventDB *event_calendar_get_event_db (EventCalendar *ev)
     __attribute__ ((pure));

/* Create a new calendar in eventdb EDB.  */
extern EventCalendar *event_calendar_new (EventDB *edb, GError **error);
/* PARENT may be NULL to indicate that this is a top-level calendar.
   (A reference to PARENT is no consumed.)  TITLE, DESCRIPTION and URL
   may be NULL.  If COLOR is NULL then no color is associated with
   this calendar.  */
extern EventCalendar *event_calendar_new_full (EventDB *edb,
					       EventCalendar *parent,
					       gboolean visible,
					       const char *title,
					       const char *description,
					       const char *url,
					       struct _GdkColor *color,
					       int mode,
					       int sync_interval,
					       GError **error);

/* Delete a calendar.  If DELETE_EVENTS is true then also removes the
   events in the calendar (and any calendars EVENT_CALENDAR contains).
   Otherwise, the NEW_PARENT is the calendar which should adopt
   them.  */
extern void event_calendar_delete (EventCalendar *event_calendar,
				   gboolean delete_events,
				   EventCalendar *new_parent,
				   GError **error);

/* The event calendar's uid.  */
extern guint event_calendar_get_uid (EventCalendar *ec)
     __attribute__ ((pure));

/* If this event calendar is visible.  */
extern gboolean event_calendar_get_visible (EventCalendar *ec, GError **error);
extern void event_calendar_set_visible (EventCalendar *ec, gboolean visible,
					GError **error);

/* Returns TRUE if setting EC's parent to NEW_PARENT is okay
   (i.e. does not create a create a cycle).  */
extern gboolean event_calendar_valid_parent (EventCalendar *ec,
					     EventCalendar *new_parent,
					     GError **error);

/* The event calendar's parent.  */
/* Returns the containing calendar's parent with a reference.  */
extern EventCalendar *event_calendar_get_parent (EventCalendar *ec,
						 GError **error);
/* Sets EC's parent to P.  If P is NULL then EC is a top-level
   calendar.  Does not consume a reference to P.  */
extern void event_calendar_set_parent (EventCalendar *ec, EventCalendar *p,
				       GError **error);

/* The event calendar's title.  */
extern char *event_calendar_get_title (EventCalendar *ec, GError **error)
     __attribute__ ((malloc));
extern void event_calendar_set_title (EventCalendar *ec, const char *title,
				      GError **error);

/* The event calendar's description.  */
extern char *event_calendar_get_description (EventCalendar *ec, GError **error)
     __attribute__ ((malloc));
extern void event_calendar_set_description (EventCalendar *ec,
					    const char *description,
					    GError **error);

/* The URL associated with this calendar (if any).  */
extern char *event_calendar_get_url (EventCalendar *ev, GError **error)
     __attribute__ ((malloc));
extern void event_calendar_set_url (EventCalendar *ec, const char *url,
				    GError **error);

/* The username associated with this calendar (if any).  */
extern char *event_calendar_get_username (EventCalendar *ev, GError **error)
     __attribute__ ((malloc));
extern void event_calendar_set_username (EventCalendar *ec,
					 const char *username,
					 GError **error);

/* The password associated with this calendar (if any).  */
extern char *event_calendar_get_password (EventCalendar *ev, GError **error)
     __attribute__ ((malloc));
extern void event_calendar_set_password (EventCalendar *ec,
					 const char *password,
					 GError **error);

/* The type of synchronization: 0 means no synchronization; 1
   indicates that this is a subscription (i.e. slave); 2 that this is
   a publisher (i.e. master); 3 indicates that this is bidirectional
   feed (i.e. multiple readers and multiple writers).  */
extern int event_calendar_get_mode (EventCalendar *ec, GError **error)
     __attribute__ ((pure));
extern void event_calendar_set_mode (EventCalendar *ec, int mode,
				     GError **error);

/* The color associated with this calendar.  */
/* Returns TRUE if there is a color associated with this event and
   places the color in *COLOR, FALSE otherwise.  */
extern gboolean event_calendar_get_color (EventCalendar *ec,
					  struct _GdkColor *color,
					  GError **error);
/* If COLOR is NULL removes any color association with this
   calendar.  */
extern void event_calendar_set_color (EventCalendar *ec,
				      const struct _GdkColor *color,
				      GError **error);

/* The frequency with which this calendar should be synchronized (in
   seconds).  */
extern int event_calendar_get_sync_interval (EventCalendar *ec,
					     GError **error)
     __attribute__ ((pure));
extern void event_calendar_set_sync_interval (EventCalendar *ec,
					      int sync_interval,
					      GError **error);

/* Set by the library's user to the current time after a successful
   pull has occured.  */
extern time_t event_calendar_get_last_pull (EventCalendar *ec,
					    GError **error)
     __attribute__ ((pure));
extern void event_calendar_set_last_pull (EventCalendar *ec,
					  time_t last_pull,
					  GError **error);

/* Set by the library's user to the current time after a successful
   push (update) has occured.  */
extern time_t event_calendar_get_last_push (EventCalendar *ec,
					    GError **error)
     __attribute__ ((pure));
extern void event_calendar_set_last_push (EventCalendar *ec,
					  time_t last_push,
					  GError **error);

/* Automatically updated by libeventdb see description of
   "calendar-modified" above.  */
extern time_t event_calendar_get_last_modification (EventCalendar *ec,
						    GError **error)
     __attribute__ ((pure));

/* Returns the events in the calendar EC (but not any sub-calendars
   thereof).  A reference is allocated to each event.  The caller
   must free the returned list.  */
extern GSList *event_calendar_list_events (EventCalendar *ec,
					   GError **error);

/* Returns the events in the calendar EC (but not any sub-calendars
   thereof).  If START is non-zero, then limited to only those events
   modified on or after START.  If END is non-zero, then limited to
   only those events modified at or before END.  A reference is
   allocated to each event.  The caller must free the returned
   list.  */
extern GSList *event_calendar_list_events_modified_between (EventCalendar *ec,
							    time_t start,
							    time_t end,
							    GError **error);

/**
 * event_calendar_list_deleted:
 * @ec: An #EventCalendar
 *
 * Retrieve a list of deleted events since the last
 * event_calendar_flush_deleted() call. The events contain a very
 * basic set of information only and should be used to indicate wether
 * a particular event was deleted or not only.  The returned list of
 * events needs to be freed by the caller.
 *
 * Returns: A list of events.
 */
extern GSList *event_calendar_list_deleted (EventCalendar *ec, GError **error);

/* Clean the list of deleted events. */
extern void event_calendar_flush_deleted (EventCalendar *ec, GError **error);


/* Returns the calendars in calendar EC (but not any sub-calendars
   thereof).  A reference is allocated to each event.  The caller must
   free the returned list.  */
extern GSList *event_calendar_list_calendars (EventCalendar *ec,
					      GError **error);

/* Signal "event-new" emitted when an event in EDB is created.  */
typedef void (*EventNew) (EventDB *edb, Event *ev);
/* Signal "event-removed" emitted when an event in EDB is removed.  */
typedef void (*EventRemoved) (EventDB *edb, Event *ev);
/* Signal "event-modified" emitted when an event in EDB is modified
   (i.e. when any of the event_set_* functions are called and the
   actual value is changed.  */
typedef void (*EventModified) (EventDB *edb, Event *ev);

/* This function has a GCompareFunc type signature and can be passed
   to e.g. g_slist_sort.

   Returns -1 if A occurs before B (or, if they start at the same
   time, if A is shorter).  Returns 1 if B occurs before A (or, if
   they start at the same time, if B is shorter).  Otherwise 0.  */
extern gint event_compare_func (gconstpointer a, gconstpointer b);

/* This function has a GCompareFunc type signature and can be passed
   to e.g. g_slist_sort.

   Returns -1 if A's alarm goes off before B's.  Returns 1 if B's
   alarm goes off before A's.  Otherwise 0.  */
extern gint event_alarm_compare_func (gconstpointer a, gconstpointer b);

/* Create a new event in database EDB in calendar EC (if NULL, then
   the default calendar).  If EVENTID is NULL, one is fabricated.  If
   EVENTID is provided, it must be unique.  (Check using
   event_db_get_by_eventid first.)  */
extern Event *event_new (EventDB *edb, EventCalendar *ec,
			 const char *eventid, GError **error);

/* Create a new event in database EDB that is a duplicate of the event
   SRC.  */
extern Event *event_duplicate (EventDB *edb, Event *src, GError **error);

/* Flush event EV to the DB now.  */
extern gboolean event_flush (Event *ev, GError **error);

/* Remove event EV from the underlying DB.  Does not destroy the in
   memory version nor does it deallocate a reference to EVENT.
   Returns success.  (If EV was already removed, also returns
   TRUE.)  */
extern gboolean event_remove (Event *ev, GError **error);

/**
 * event_list_unref:
 * @l: Event list
 *
 * g_object_unref each Event * on the list and destroy the list.
 */
extern void event_list_unref (GSList *l);

/* Returns the event db (without allocating a reference) in which EV
   lives.  */
extern EventDB *event_get_event_db (Event *ev) __attribute__ ((pure));

/* The calendar associated with EV.  */
/* Returns a reference to the calendar.  */
extern EventCalendar *event_get_calendar (Event *ev, GError **error);
extern void event_set_calendar (Event *ev, EventCalendar *ec,
				GError **error);

/* Finds the first calendar which contains event EV which has a color
   and in in *COLOR and returns TRUE.  Otherwise, returns FALSE.  */
extern gboolean event_get_color (Event *ev, struct _GdkColor *color,
				 GError **error);
/* Determines if event EV is visible.  That is, if its containing
   calendar the calendar containing it, etc are all visible then EV is
   visible.  */
extern gboolean event_get_visible (Event *ev, GError **error)
     __attribute__ ((pure));

extern time_t event_get_start (Event *ev) __attribute__ ((pure));

/* The duration of an event in seconds.  Taking the start time and
   adding the duration yields the second after which the event
   ends.  */
extern unsigned long event_get_duration (Event *ev)
     __attribute__ ((pure));
extern void event_set_duration (Event *ev, unsigned long duration,
				GError **error);

extern unsigned long event_get_alarm (Event *ev) __attribute__ ((pure));
extern void event_set_alarm (Event *ev, unsigned long alarm,
			     GError **error);

/* Acknowledge that EV's alarm went off.  If EV does not have an alarm
   or it would go off in the future, does nothing.  */
extern void event_acknowledge (Event *ev, GError **error);

/* The event's sequence number, i.e. the number of significant
   revisions.  (Normally automatically updated at each important
   change to the event, e.g. change of start, end, location, etc.)  */
extern guint32 event_get_sequence (Event *ev, GError **error)
     __attribute__ ((pure));
extern void event_set_sequence (Event *ev, guint32 sequence,
				GError **error);

extern time_t event_get_last_modification (Event *event)
     __attribute__ ((pure));

/* Type of recurrence.  */
#define event_is_recurrence(ev) \
  (event_get_recurrence_type ((ev)) != RECUR_NONE)
extern enum event_recurrence_type event_get_recurrence_type (Event *ev)
     __attribute__ ((pure));
extern void event_set_recurrence_type (Event *ev,
				       enum event_recurrence_type type,
				       GError **error);

/* Start of the recurrence set.  */
extern time_t event_get_recurrence_start (Event *ev) __attribute__ ((pure));
extern void event_set_recurrence_start (Event *ev, time_t start,
					GError **error);

/* End of the recurrence set.  */
extern time_t event_get_recurrence_end (Event *ev) __attribute__ ((pure));
extern void event_set_recurrence_end (Event *ev, time_t end, GError **error);

/* Number of times the recurrence is expanded.  If 0, then this not
   does constrain the number of recurrences.  */
extern guint32 event_get_recurrence_count (Event *ev) __attribute__ ((pure));
extern void event_set_recurrence_count (Event *ev, guint32 count,
					GError **error);

/* iCal's interval property: the number of units to skip.  If the
   recurrence type is RECUR_YEARLY then the first recurrence occurs
   INCREMENT years after the initial start.  */
extern guint32 event_get_recurrence_increment (Event *ev)
     __attribute__ ((pure));
extern void event_set_recurrence_increment (Event *ev, guint32 increment,
					    GError **error);

/* Return the byday list.  Each element is a string.  The caller must
   free the strings and the list.  */
extern GSList *event_get_recurrence_byday (Event *ev, GError **error);
/* Set the byday list to the list of strings of the form:
     [+-]([0-9]*[1-9]|)(MO|TU|WE|TH|FR|SA|SU).  */
extern void event_set_recurrence_byday (Event *event, GSList *list,
					GError **error);

static inline void
event_recurrence_byday_free (GSList *byday)
{
  GSList *l;
  for (l = byday; l; l = l->next)
    g_free (l->data);
  g_slist_free (byday);
}

extern void event_add_recurrence_exception (Event *ev, time_t start,
					    GError **error);

extern gboolean event_get_untimed (Event *ev) __attribute__ ((pure));
extern void event_set_untimed (Event *ev, gboolean value, GError **error);

extern unsigned long event_get_uid (Event *ev) __attribute__ ((pure));
/* Caller must free the returned string.  */
extern char *event_get_eventid (Event *ev, GError **error)
     __attribute__ ((malloc));


/* Caller must free the returned string.  */
extern char *event_get_summary (Event *ev, GError **error)
     __attribute__ ((malloc));
extern void event_set_summary (Event *ev, const char *summary,
			       GError **error);

/* Caller must free the returned string.  */
extern char *event_get_description (Event *ev, GError **error)
     __attribute__ ((malloc));
extern void event_set_description (Event *ev, const char *description,
				   GError **error);

/* Caller must free the returned string.  */
extern char *event_get_location (Event *ev, GError **error)
     __attribute__ ((malloc));
extern void event_set_location (Event *ev, const char *location,
				GError **error);

/* Caller must free (g_slist_free) the returned list.  */
extern GSList *event_get_categories (Event *ev, GError **error);
extern void event_add_category (Event *ev, int category, GError **error);
/* After calling this function, EV owns CATEGORIES.  If you need to
   continue to use CATEGORIES, pass a copy.  */
extern void event_set_categories (Event *ev, GSList *categories,
				  GError **error);

#endif
