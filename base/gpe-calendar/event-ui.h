/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#include "event-db.h"

extern GtkWidget *new_event(time_t t, guint timesel);
extern GtkWidget *edit_event(event_t ev);

extern void event_ui_init (void);
extern void schedule_next(void);

#endif
