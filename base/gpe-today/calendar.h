/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef HAVE_CALENDAR_H
#define HAVE_CALENDAR_H

#include <gpe/event-db.h>
#include "today.h"

struct {
	GtkWidget *toplevel;

	GtkWidget *vboxlogo;
	  GtkWidget *logo;

	struct myscroll *scroll;
} calendar;

int calendar_init(void);
void calendar_free(void);
void calendar_events_db_update(void);

#endif /* !HAVE_CALENDAR_H */
