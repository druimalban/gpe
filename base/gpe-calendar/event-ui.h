/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_UI_H
#define EVENT_UI_H

#include <time.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <gpe/event-db.h>

extern GtkWidget *new_event (time_t t, guint timesel);
extern GtkWidget *edit_event (Event *ev);

extern void unschedule_alarm (Event *ev, GtkWidget *d);
extern void schedule_next (guint, guint, GtkWidget *d);

#endif
