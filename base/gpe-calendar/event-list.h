/* event-list.h - Event list widget interface.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include <gtk/gtk.h>
#include <gpe/event-db.h>

struct _EventList;
typedef struct _EventList EventList;

#define TYPE_EVENT_LIST (event_list_get_type ())
#define EVENT_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EVENT_LIST, EventList))
#define EVENT_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EVENT_LIST, EventListClass))
#define IS_EVENT_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EVENT_LIST))
#define IS_EVENT_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EVENT_LIST))
#define EVENT_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EVENT_LIST, EventListClass))

/* "event-clicked" signal emitted when an event is clicked.  */
typedef void (* EventListEventClicked) (EventList *, Event *,
					GdkEventButton *);
/* "event-key-pressed" signal emitted when an event is clicked.  */
typedef void (* EventListEventKeyPressed) (EventList *, Event *,
					   GdkEventKey *);

/* Return GType of a day view.  */
extern GType event_list_get_type (void);

/* Create a new day view.  */
extern GtkWidget *event_list_new (EventDB *edb);

/* Force LIST to reload the events from the event database.  */
void event_list_reload_events (EventList *list);

#endif
