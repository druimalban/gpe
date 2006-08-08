/*
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib-object.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "gpe/event-db.h"
#include "event-db.h"
#include "event-cal.h"
#include "event.h"

/* The only thing we use from Gdk is the GdkColor structure.  Avoid a
   dependency and include it inline here.  */
struct _GdkColor {
  guint32 pixel;
  guint16 red;
  guint16 green;
  guint16 blue;
};

static void event_calendar_class_init (gpointer klass, gpointer klass_data);
static void event_calendar_init (GTypeInstance *instance, gpointer klass);
static void event_calendar_dispose (GObject *obj);
static void event_calendar_finalize (GObject *object);

static GObjectClass *event_calendar_parent_class;

GType
event_calendar_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventCalendarClass),
	NULL,
	NULL,
	event_calendar_class_init,
	NULL,
	NULL,
	sizeof (struct _EventCalendar),
	0,
	event_calendar_init
      };

      type = g_type_register_static (G_TYPE_OBJECT, "EventCalendar", &info, 0);
    }

  return type;
}

static void
event_calendar_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;

  event_calendar_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = event_calendar_finalize;
  object_class->dispose = event_calendar_dispose;
}

static void
event_calendar_init (GTypeInstance *instance, gpointer klass)
{
}

static void
event_calendar_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (event_calendar_parent_class)->dispose (obj);
}

void
event_calendar_flush (EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf (ec->edb->sqliteh,
			   "update calendars set"
			   "  title='%q', description='%q',"
			   "  url='%q', username='%q', password='%q',"
			   "  parent=%d, hidden=%d,"
			   "  has_color=%d, red=%d, green=%d, blue=%d,"
			   "  mode=%d, sync_interval=%d,"
			   "  last_pull=%d, last_push=%d,"
			   "  last_modified=%d"
			   " where ROWID=%d;",
			   NULL, NULL, &err,
			   ec->title ?: "", ec->description ?: "",
			   ec->url ?: "", ec->username ?: "",
			   ec->password ?: "",
			   ec->parent_uid, ec->hidden,
			   ec->has_color, ec->red, ec->green, ec->blue,
			   ec->mode, ec->sync_interval,
			   ec->last_push, ec->last_pull, ec->last_modified,
			   ec->uid)))
    {
      g_critical ("%s: updating %s (%d): %s", __func__,
		  ec->description, ec->uid, err);
      g_free (err);
    }

  ec->modified = FALSE;
}

static void
event_calendar_finalize (GObject *object)
{
  EventCalendar *ec = EVENT_CALENDAR (object);

  if (ec->parent)
    g_object_unref (ec->parent);

  g_free (ec->title);
  g_free (ec->description);
  g_free (ec->url);
  g_free (ec->username);
  g_free (ec->password);

  G_OBJECT_CLASS (event_calendar_parent_class)->finalize (object);
}

static void
event_db_calendar_new (EventCalendar *ec)
{
  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (ec->edb->sqliteh,
	"insert into calendars values"
	" ('%q', '%q', '%q', '%q', '%q', %d, %d, %d, %d, %d,"
	"  %d, %d, %d, %d, %d, %d)",
	NULL, NULL, &err,
	ec->title ?: "", ec->description ?: "",
	ec->url ?: "",
	ec->username ?: "", ec->password ?: "",
	ec->parent_uid, ec->hidden,
	ec->has_color, ec->red, ec->green, ec->blue,
	ec->mode, ec->sync_interval,
	ec->last_pull, ec->last_push,
	ec->last_modified)))
    {
      g_warning ("%s: %s", __func__, err);
      g_free (err);
    }

  ec->uid = sqlite_last_insert_rowid (ec->edb->sqliteh);
}

EventCalendar *
event_calendar_new (EventDB *edb)
{
  EventCalendar *ec = EVENT_CALENDAR (g_object_new (event_calendar_get_type (),
						    NULL));

  ec->edb = edb;
  event_db_calendar_new (ec);

  g_object_ref (ec);
  edb->calendars = g_slist_prepend (edb->calendars, ec);

  g_signal_emit (edb, EVENT_DB_GET_CLASS (edb)->calendar_new_signal, 0, ec);

  return ec;
}

EventCalendar *
event_calendar_new_full (EventDB *edb,
			 EventCalendar *parent,
			 gboolean visible,
			 const char *title,
			 const char *description,
			 const char *url,
			 struct _GdkColor *color,
			 int mode,
			 int sync_interval)
{
  g_return_val_if_fail (mode >= 0, NULL);
  g_return_val_if_fail (mode <= 4, NULL);

  EventCalendar *ec = EVENT_CALENDAR (g_object_new (event_calendar_get_type (),
						    NULL));

  ec->edb = edb;
  if (parent)
    {
      ec->parent_uid = parent->uid;
      g_object_ref (parent);
      ec->parent = parent;
    }
  else
    ec->parent_uid = EVENT_CALENDAR_NO_PARENT;

  ec->hidden = ! visible;

  ec->title = title ? g_strdup (title) : NULL;
  ec->description = description ? g_strdup (description) : NULL;
  ec->url = url ? g_strdup (url) : NULL;
  if (color)
    {
      ec->has_color = TRUE;
      ec->red = color->red;
      ec->green = color->green;
      ec->blue = color->blue;
    }
  ec->mode = mode;
  ec->sync_interval = sync_interval;

  event_db_calendar_new (ec);

  g_object_ref (ec);
  edb->calendars = g_slist_prepend (edb->calendars, ec);

  g_signal_emit (edb, EVENT_DB_GET_CLASS (edb)->calendar_new_signal, 0, ec);

  return ec;
}

gboolean
event_calendar_valid_parent (EventCalendar *ec, EventCalendar *new_parent)
{
  EventCalendar *i;
  for (i = new_parent; i; i = event_calendar_get_parent (i))
    if (i == ec)
      return FALSE;
  return TRUE;
}

void
event_calendar_delete (EventCalendar *ec,
		       gboolean delete_events,
		       EventCalendar *new_parent)
{
  if (! delete_events && ! event_calendar_valid_parent (ec, new_parent))
    {
      g_critical ("%s: I refuse to create a cycle.", __func__);
      return;
    }

  GSList *link = NULL;

  /* Remove the calendars which are descendents of EC.  */
  GSList *i;
  GSList *next = ec->edb->calendars;
  while (next)
    {
      i = next;
      next = next->next;

      EventCalendar *c = EVENT_CALENDAR (i->data);

      if (c == ec)
	link = i;

      if (c->parent_uid == ec->uid)
	{
	  if (delete_events)
	    event_calendar_delete (c, TRUE, 0);
	  else
	    event_calendar_set_parent (c, new_parent);
	}
    }

  g_assert (link);
  ec->edb->calendars = g_slist_delete_link (ec->edb->calendars, link);

  GSList *events = event_calendar_list_events (ec);
  for (i = events; i; i = i->next)
    {
      Event *ev = EVENT (i->data);
      if (delete_events)
	event_remove (ev);
      else
	event_set_calendar (ev, new_parent);
    }
  event_list_unref (events);

  char *err;
  if (SQLITE_TRY
      (sqlite_exec_printf
       (ec->edb->sqliteh,
	"begin transaction;"
	/* Remove the events.  */
	"delete from calendar where uid"
	" in (select uid from events where calendar=%d);"
	"delete from events where calendar=%d;"
	/* And the calendar.  */
	"delete from calendars where ROWID=%d;"
	"commit transaction",
	NULL, NULL, &err, ec->uid, ec->uid, ec->uid)))
    {
      sqlite_exec (ec->edb->sqliteh, "rollback transaction;",
		   NULL, NULL, NULL);
      g_critical ("%s: %s", __func__, err);
      g_free (err);
    }

  g_signal_emit (ec->edb,
		 EVENT_DB_GET_CLASS (ec->edb)->calendar_deleted_signal, 0, ec);
}

#define GET(type, name) \
  type \
  event_calendar_get_##name (EventCalendar *ec) \
  { \
    return ec->name; \
  }

#define GET_SET(type, name, is_modification) \
  GET(type, name) \
  \
  void \
  event_calendar_set_##name (EventCalendar *ec, type name) \
  { \
    if (ec->name == name) \
      return; \
    ec->name = name; \
    if (is_modification) \
      { \
        ec->modified = TRUE; \
        ec->last_modified = time (NULL); \
      } \
    ec->changed = TRUE; \
    add_to_laundry_pile (G_OBJECT (ec)); \
  }

GET(guint, uid)

EventCalendar *
event_calendar_get_parent (EventCalendar *ec)
{
  if (ec->parent_uid == EVENT_CALENDAR_NO_PARENT)
    return NULL;

  if (! ec->parent)
    {
      ec->parent = event_db_find_calendar_by_uid (ec->edb, ec->parent_uid);
      g_object_ref (ec->parent);
      if (! ec->parent)
	{
	  g_warning ("Calendar (%s) %d contains a dangling parent %d!",
		     ec->description, ec->uid, ec->parent_uid);
	  return NULL;
	}
    }

  g_object_ref (ec->parent);
  return ec->parent;
}

void
event_calendar_set_parent (EventCalendar *ec, EventCalendar *p)
{
  if ((! p && ec->parent_uid == EVENT_CALENDAR_NO_PARENT)
      || (p && p->uid == ec->uid))
    return;

  g_return_if_fail (event_calendar_valid_parent (ec, p));

  if (ec->parent)
    g_object_unref (ec->parent);
  ec->parent = p;
  if (p)
    g_object_ref (p);

  ec->parent_uid = p ? p->uid : EVENT_CALENDAR_NO_PARENT;

  ec->modified = TRUE;
  ec->last_modified = time (NULL);
  ec->changed = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));

  g_signal_emit (ec->edb,
		 EVENT_DB_GET_CLASS (ec->edb)->calendar_reparented_signal,
		 0, ec);
}

gboolean
event_calendar_get_visible (EventCalendar *ec)
{
  return ! ec->hidden;
}

void
event_calendar_set_visible (EventCalendar *ec, gboolean visible)
{
  if (ec->hidden == ! visible)
    return;
  ec->hidden = ! visible;
  ec->changed = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));
}

#define GET_SET_STRING(name, is_modification) \
  char * \
  event_calendar_get_##name (EventCalendar *ec) \
  { \
    return g_strdup (ec->name); \
  } \
  \
  void \
  event_calendar_set_##name (EventCalendar *ec, const char *name) \
  { \
    if (ec->name && strcmp (ec->name, name) == 0) \
      return; \
    g_free (ec->name); \
    ec->name = g_strdup (name); \
    if (is_modification) \
      { \
        ec->modified = TRUE; \
        ec->last_modified = time (NULL); \
      } \
    ec->changed = TRUE; \
    add_to_laundry_pile (G_OBJECT (ec)); \
  }

GET_SET_STRING(title, TRUE)
GET_SET_STRING(description, TRUE)
GET_SET_STRING(url, TRUE)
GET_SET_STRING(username, TRUE)
GET_SET_STRING(password, TRUE)

GET_SET(int, mode, TRUE)

gboolean
event_calendar_get_color (EventCalendar *ec, struct _GdkColor *color)
{
  if (! ec->has_color)
    return FALSE;

  color->red = ec->red;
  color->green = ec->green;
  color->blue = ec->blue;

  return TRUE;
}

void
event_calendar_set_color (EventCalendar *ec, const struct _GdkColor *color)
{
  if (color)
    {
      ec->has_color = TRUE;
      ec->red = color->red;
      ec->green = color->green;
      ec->blue = color->blue;
    }
  else
    ec->has_color = FALSE;

  ec->changed = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));
}

GET_SET(int, sync_interval, FALSE)
GET_SET(time_t, last_pull, FALSE)
GET_SET(time_t, last_push, FALSE)

time_t
event_calendar_get_last_modification (EventCalendar *ec)
{
  return ec->last_modified;
}

GSList *
event_calendar_list_events (EventCalendar *ec)
{
  GSList *list = NULL;

  int callback (void *arg, int argc, char *argv[], char **names)
    {
      Event *ev = event_db_find_by_uid (ec->edb, atoi (argv[0]));
      if (ev)
	list = g_slist_prepend (list, ev);
      return 0;
    }
  SQLITE_TRY
    (sqlite_exec_printf (ec->edb->sqliteh,
			 "select uid from events where calendar=%d;",
			 callback, NULL, NULL, ec->uid));

  return list;
}

GSList *
event_calendar_list_calendars (EventCalendar *p)
{
  GSList *c = NULL;
  GSList *i;
  for (i = p->edb->calendars; i; i = i->next)
    {
      EventCalendar *ec = EVENT_CALENDAR (i->data);
      if (ec->parent_uid == p->uid)
	{
	  g_object_ref (ec);
	  c = g_slist_prepend (c, ec);
	}
    }

  return c;
}

GSList *
event_calendar_list_deleted (EventCalendar *ec)
{
  GSList *list = NULL;

  int callback (void *arg, int argc, char *argv[], char **names)
    {
      EventSource *ev;
      
      if (argc != 3) 
          return 0;
      ev = EVENT_SOURCE (g_object_new (event_source_get_type (), NULL));
      g_object_ref (ec->edb);
      ev->edb = ec->edb;
      ev->uid = (-1) * atoi (argv[0]);
      ev->eventid = g_strdup (argv[1]);
      ev->calendar = atoi (argv[2]);
      ev->event.dead = 1;

      list = g_slist_prepend (list, ev);
      return 0;
    }
  SQLITE_TRY
    (sqlite_exec_printf (ec->edb->sqliteh,
			 "select uid, eventid, calendar from events_deleted where calendar=%d;",
			 callback, NULL, NULL, ec->uid));

  return list;
}

void 
event_calendar_flush_deleted (EventCalendar *ec)
{
   SQLITE_TRY (sqlite_exec_printf (ec->edb->sqliteh,
			     "delete from events_deleted where calendar=%d;",
			     NULL, NULL, NULL, ec->uid));
}
