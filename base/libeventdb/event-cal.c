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
event_calendar_flush (EventCalendar *ec, GError **error)
{
  GError *e = NULL;
  EVENT_DB_GET_CLASS (ec->edb)->event_calendar_flush (ec, &e);
  if (e)
    SIGNAL_ERROR_GERROR (ec->edb, error, e);
  else
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


EventDB *
event_calendar_get_event_db (EventCalendar *ec)
{
  return ec->edb;
}

EventCalendar *
event_calendar_new (EventDB *edb, GError **error)
{
  EventCalendar *ec = EVENT_CALENDAR (g_object_new (event_calendar_get_type (),
						    NULL));

  ec->edb = edb;

  GError *e = NULL;
  EVENT_DB_GET_CLASS (ec->edb)->event_calendar_new (ec, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (edb, error, e);
      g_object_unref (ec);
      return NULL;
    }

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
			 int sync_interval,
			 GError **error)
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

  GError *e = NULL;
  EVENT_DB_GET_CLASS (ec->edb)->event_calendar_new (ec, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ec->edb, error, e);
      g_object_unref (ec);
      return NULL;
    }

  g_object_ref (ec);
  edb->calendars = g_slist_prepend (edb->calendars, ec);

  g_signal_emit (edb, EVENT_DB_GET_CLASS (edb)->calendar_new_signal, 0, ec);

  return ec;
}

gboolean
event_calendar_valid_parent (EventCalendar *ec, EventCalendar *new_parent,
			     GError **error)
{
  EventCalendar *i;
  GError *e = NULL;

  g_object_ref (new_parent);
  for (i = new_parent; i; i = event_calendar_get_parent (i, &e))
    {
      g_object_unref (i);
      /* Even though we just dropped our reference to I, the following
	 is still valid as we don't dereference I, we only use its
	 address.  */
      if (i == ec)
	return FALSE;
    }

  if (e)
    {
      SIGNAL_ERROR_GERROR (ec->edb, error, e);
      return FALSE;
    }
  return TRUE;
}

void
event_calendar_delete (EventCalendar *ec,
		       gboolean delete_events,
		       EventCalendar *new_parent,
		       GError **error)
{
  GError *e = NULL;
  if (! delete_events && ! event_calendar_valid_parent (ec, new_parent, &e))
    {
      if (e)
	SIGNAL_ERROR_GERROR (ec->edb, error, e);
      else
	SIGNAL_ERROR (ec->edb, error,
		      "Setting selected calendar as new parent would "
		      "create a cycle, refusing.");
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
	    event_calendar_delete (c, TRUE, 0, &e);
	  else
	    event_calendar_set_parent (c, new_parent, &e);

	  if (e)
	    {
	      SIGNAL_ERROR_GERROR (ec->edb, error, e);
	      return;
	    }
	}
    }

  g_assert (link);
  ec->edb->calendars = g_slist_delete_link (ec->edb->calendars, link);

  GSList *events = event_calendar_list_events (ec, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ec->edb, error, e);
      return;
    }

  for (i = events; i; i = i->next)
    {
      Event *ev = EVENT (i->data);
      if (delete_events)
	event_remove (ev, &e);
      else
	event_set_calendar (ev, new_parent, &e);

      if (e)
	{
	  SIGNAL_ERROR_GERROR (ec->edb, error, e);
	  return;
	}
    }
  event_list_unref (events);

  EVENT_DB_GET_CLASS (ec->edb)->event_calendar_delete (ec, &e);
  if (e)
    {
      SIGNAL_ERROR_GERROR (ec->edb, error, e);
      return;
    }

  g_signal_emit (ec->edb,
		 EVENT_DB_GET_CLASS (ec->edb)->calendar_deleted_signal, 0, ec);
}

#define GET(type, name) \
  type \
  event_calendar_get_##name (EventCalendar *ec, GError **error) \
  { \
    return ec->name; \
  }

#define GET_SET(type, name, is_modification) \
  GET(type, name) \
  \
  void \
  event_calendar_set_##name (EventCalendar *ec, type name, GError **error) \
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

guint
event_calendar_get_uid (EventCalendar *ec)
{
  return ec->uid;
}

EventCalendar *
event_calendar_get_parent (EventCalendar *ec, GError **error)
{
  if (ec->parent_uid == EVENT_CALENDAR_NO_PARENT)
    return NULL;

  if (! ec->parent)
    {
      GError *e = NULL;

      ec->parent = event_db_find_calendar_by_uid (ec->edb, ec->parent_uid,
						  &e);
      if (e)
	{
	  g_assert (! ec->parent);
	  SIGNAL_ERROR_GERROR (ec->edb, error, e);
	  return NULL;
	}

      if (! ec->parent)
	{
	  g_warning ("Calendar (%s) %d contains a dangling parent uid %d!",
		     ec->description, ec->uid, ec->parent_uid);
	  return NULL;
	}
      g_object_ref (ec->parent);
    }

  g_object_ref (ec->parent);
  return ec->parent;
}

void
event_calendar_set_parent (EventCalendar *ec, EventCalendar *p,
			   GError **error)
{
  if ((! p && ec->parent_uid == EVENT_CALENDAR_NO_PARENT)
      || (p && p->uid == ec->uid))
    return;

  g_return_if_fail (event_calendar_valid_parent (ec, p, error));

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
event_calendar_get_visible (EventCalendar *ec, GError **error)
{
  return ! ec->hidden;
}

void
event_calendar_set_visible (EventCalendar *ec, gboolean visible,
			    GError **error)
{
  if (ec->hidden == ! visible)
    return;
  ec->hidden = ! visible;
  ec->changed = TRUE;
  add_to_laundry_pile (G_OBJECT (ec));
}

#define GET_SET_STRING(name, is_modification) \
  char * \
  event_calendar_get_##name (EventCalendar *ec, GError **error) \
  { \
    return g_strdup (ec->name); \
  } \
  \
  void \
  event_calendar_set_##name (EventCalendar *ec, const char *name, \
                             GError **error) \
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
event_calendar_get_color (EventCalendar *ec, struct _GdkColor *color,
			  GError **error)
{
  if (! ec->has_color)
    return FALSE;

  color->red = ec->red;
  color->green = ec->green;
  color->blue = ec->blue;

  return TRUE;
}

void
event_calendar_set_color (EventCalendar *ec, const struct _GdkColor *color,
			  GError **error)
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
event_calendar_get_last_modification (EventCalendar *ec, GError **error)
{
  return ec->last_modified;
}

GSList *
event_calendar_list_events (EventCalendar *ec, GError **error)
{
  return EVENT_DB_GET_CLASS (ec->edb)->event_calendar_list_events (ec, 0, 0,
								   error);
}

GSList *
event_calendar_list_events_modified_between (EventCalendar *ec,
					     time_t start, time_t end,
					     GError **error)
{
  return
    EVENT_DB_GET_CLASS (ec->edb)->event_calendar_list_events (ec, start, end,
							      error);
}

GSList *
event_calendar_list_calendars (EventCalendar *p, GError **error)
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
event_calendar_list_deleted (EventCalendar *ec, GError **error)
{
  return EVENT_DB_GET_CLASS (ec->edb)->event_calendar_list_deleted (ec,
								    error);
}

void 
event_calendar_flush_deleted (EventCalendar *ec, GError **error)
{
  EVENT_DB_GET_CLASS (ec->edb)->event_calendar_flush_deleted (ec, error);
}
