/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006, 2008 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_UI_H
#define EVENT_UI_H

#include <time.h>
#include <stdbool.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <gpe/event-db.h>

extern GtkWidget *new_event (time_t t);
extern GtkWidget *edit_event (Event *ev, bool remove_event_on_cancel);

#endif
