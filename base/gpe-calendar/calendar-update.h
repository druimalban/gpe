/* calendar-update.h - Calendar update interface.
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

#ifndef CALENDAR_UPDATE_H
#define CALENDAR_UPDATE_H

#include <gpe/event-db.h>

/* Gets the calendar now.  */
extern void calendar_pull (EventCalendar *ec);

/* Puts the calendar now.  */
extern void calendar_push (EventCalendar *ec);

/* Begins watching for changes on calendars.  This function should be
   called exactly once at start up.  This function always returns
   FALSE.  The data function is ignored.  This is so it can be
   directly used as an argument to g_idle_add.  */
extern gboolean calendars_sync_start (void *data);

#endif /* CALENDAR_UPDATE_H */
