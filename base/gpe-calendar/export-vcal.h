/* export-vcal.h - Export calendar interface.
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

#ifndef _EXPORT_VCAL_H
#define _EXPORT_VCAL_H

#include <gpe/event-db.h>
#include <mimedir/mimedir-vevent.h>

/* Converts event EV to a MIMEDirVEvent.  */
extern MIMEDirVEvent *export_event_as_vevent (Event *ev);

/* Converts event EV to a string.  The caller must free the returned
   string using g_free.  */
extern char *export_event_as_string (Event *ev);

/* Converts calendar EV to a string.  The caller must free the
   returned string using g_free.  */
extern char *export_calendar_as_string (EventCalendar *ev);

/* Prompts the user with a save as dialog.  Does not consume a
   reference to EVENT.  */
extern void export_event_save_as_dialog (Event *event);

/* Prompts the user with a save as dialog.  Does not consume a
   reference to EVENT.  */
extern void export_calendar_save_as_dialog (EventCalendar *event);

void vcal_export_init (void);
gboolean export_bluetooth_available (void);
gboolean export_irda_available (void);

gboolean export_calendar_to_file (EventCalendar *ec, const gchar *filename);
gboolean export_list_to_file (GSList *things, const gchar *filename);

void vcal_do_send_bluetooth (Event *event);
void vcal_do_send_irda (Event *event);

#endif
