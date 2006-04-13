/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef GPE_VEVENT_H
#define GPE_VEVENT_H

#include <glib.h>
#include <gpe/event-db.h>
#include <mimedir/mimedir-vevent.h>

#include <gpe/tag-db.h>

extern MIMEDirVEvent *vevent_from_tags (GSList *tags);
extern MIMEDirVEvent *vevent_from_event_t (Event *event);
extern GSList *vevent_to_tags (MIMEDirVEvent *vevent);

#endif
