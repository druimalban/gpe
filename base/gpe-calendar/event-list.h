/* event-list.h - Event list widget interface.
   Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>

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

enum event_list_columns
  {
    EVENT_LIST_START_TIME = 1 << 0,
    EVENT_LIST_SUMMARY = 1 << 1,
    EVENT_LIST_END_TIME = 1 << 2
  };

/* Create a new day view.  The period box (the text entry and combo
   which allows the user to select the the time period for which
   events should be displayed), is initially hidden.  It can be shown
   using event_list_set_period_box_visible.  */
extern GtkWidget *event_list_new (EventDB *edb /*, int column*/);

/* Force LIST to reload the events from the event database.  */
extern void event_list_reload_events (EventList *list);

/* Whether to respect the visibility of events.  */
extern void event_list_set_show_all (EventList *event_list,
				     gboolean show_all);

/* Shows or hides EVENT_LIST's period box.  */
extern void event_list_set_period_box_visible (EventList *event_list,
					       gboolean visible);

#endif
