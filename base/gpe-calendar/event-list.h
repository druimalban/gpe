/* event-list.c - Event list widget interface.
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

struct _GtkEventList;
typedef struct _GtkEventList GtkEventList;

#define GTK_EVENT_LIST(obj) \
  GTK_CHECK_CAST (obj, gtk_event_list_get_type (), struct _GtkEventList)
#define GTK_EVENT_LIST_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gtk_event_list_get_type (), EventListClass)
#define GTK_IS_EVENT_LIST(obj) GTK_CHECK_TYPE (obj, gtk_event_list_get_type ())

/* Return GType of a day view.  */
extern GType gtk_event_list_get_type (void);

/* Create a new day view.  */
extern GtkWidget *gtk_event_list_new (void);

/* Force LIST to reload the events from the event database.  */
void gtk_event_list_reload_events (GtkEventList *list);

#endif
