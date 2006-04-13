/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_DB_H
#define EVENT_DB_H

#define SECONDS_IN_DAY (24*60*60)

#include <glib-object.h>
#include <glib.h>
#include <time.h>

#define MON  (1 << 0)
#define TUE  (1 << 1)
#define WED  (1 << 2)
#define THU  (1 << 3)
#define FRI  (1 << 4)
#define SAT  (1 << 5)
#define SUN  (1 << 6)

struct _EventDB;
typedef struct _EventDB EventDB;

typedef enum
{
  RECUR_NONE,
  RECUR_DAILY,
  RECUR_WEEKLY,
  RECUR_MONTHLY,
  RECUR_YEARLY
} recur_type_t;

/**
 * *recur_t:
 *
 * Structure to hold recurrence information.
 */
typedef struct recur_s
{
  recur_type_t type;
  
  unsigned int count;
  unsigned int increment;
  unsigned long daymask;	/* bit 0 = Mon, bit 1 = Tue, etc */
  GSList *exceptions;
  
  time_t end;			/* 0 == perpetual */
} *recur_t;

#define EVENT_TYPE (event_get_type ())
#define EVENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVENT_TYPE, struct _Event))
#define EVENT_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, EVENT_TYPE, struct _EventCLass)

typedef struct
{
  GObjectClass gobject_class;
  GObjectClass parent_class;
} EventClass;

struct _Event;
typedef struct _Event Event;

extern GType event_get_type (void);

struct _EventDB
{
  GObject object;

  unsigned long dbversion;
  void *sqliteh;

  GList *recurring_events, *one_shot_events;

  /* Largest UID which we know of.  */
  unsigned long uid;
};

typedef struct
{
  GObjectClass gobject_class;
  GObjectClass parent_class;

  /* Signals.  */
  guint event_new_signal;
  void (*event_new) (EventDB *view, Event *event);
  guint event_removed_signal;
  void (*event_removed) (EventDB *view, Event *event);
  guint event_changed_signal;
  void (*event_changed) (EventDB *view, Event *event);
} EventDBClass;

#define EVENT_DB_TYPE (event_db_get_type ())
#define EVENT_DB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVENT_DB_TYPE, struct _EventDB))
#define EVENT_DB_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, EVENT_DB_TYPE, struct _EventDBCLass)

/* Return GType of a view.  */
extern GType event_db_get_type (void);

/* Open a new database.  */
extern EventDB *event_db_new (const char *filename);

/* Search the event database EVD for the event with the uid UID.  */
extern Event *event_db_find_by_uid (EventDB *evd, guint uid);

/* Return the events in the event database EVD which occur between
   START and END.  */
extern GSList *event_db_list_for_period (EventDB *evd,
					 time_t start, time_t end);
/* Like event_db_list but returns the events whose alarm goes off
   between START and END.  */
extern GSList *event_db_list_alarms_for_period (EventDB *evd,
						time_t start, time_t end);
/* Returns up to the MAX events which occur following START.  */
extern GSList *event_db_list_for_future (EventDB *evd,
					 time_t start, guint max);
/* Like event_db_list_for_period but only for untimed events
   (i.e. which with a 0 length duration).  */
extern GSList *event_db_untimed_list_for_period (EventDB *evd,
						 time_t start, time_t end,
						 gboolean yes);

/* Create a new event in database EDB.  */
extern Event *event_new (EventDB *edb, const char *event_id);

/* Flush event EV to the DB now.  */
extern gboolean event_flush (Event *ev);

/* Remove event EV from the underlying DB and dereference it.  */
extern gboolean event_remove (Event *ev);

/* g_object unref each Event * on the list and destroy the list.  */
extern void event_list_unref (GSList *l);

/* Return whether an event is a recurrent event.  */
extern gboolean event_is_recurrence (Event *ev) __attribute__ ((pure));
/* Return the event's recurrence data.  If this is changed, you must
   call event_flush.  */
extern recur_t event_get_recurrence (Event *ev) __attribute__ ((pure));
extern void event_clear_recurrence (Event *ev);

extern time_t event_get_start (Event *ev) __attribute__ ((pure));
extern void event_set_start (Event *ev, time_t time);

extern unsigned long event_get_duration (Event *ev) __attribute__ ((pure));
extern void event_set_duration (Event *ev, unsigned long duration);

extern unsigned long event_get_alarm (Event *ev) __attribute__ ((pure));
extern void event_set_alarm (Event *ev, unsigned long alarm);

extern gboolean event_is_untimed (Event *ev) __attribute__ ((pure));
extern void event_set_untimed (Event *ev);
extern void event_clear_untimed (Event *ev);

extern unsigned long event_get_uid (Event *ev) __attribute__ ((pure));
extern const char *event_get_eventid (Event *ev) __attribute__ ((pure));


extern const char *event_get_summary (Event *ev) __attribute__ ((pure));
extern void event_set_summary (Event *ev, const char *summary);

extern const char *event_get_description (Event *ev) __attribute__ ((pure));
extern void event_set_description (Event *ev, const char *description);

extern const char *event_get_location (Event *ev) __attribute__ ((pure));
extern void event_set_location (Event *ev, const char *location);

extern const GSList *const event_get_categories (Event *ev)
     __attribute__ ((pure));
extern void event_add_category (Event *ev, int category);
/* After calling this function, EV owns CATEGORIES.  If you need to
   continue to use CATEGORIES, pass a copy.  */
extern void event_set_categories (Event *ev, GSList *categories);

extern void event_add_exception (Event *ev, time_t start);

#endif
