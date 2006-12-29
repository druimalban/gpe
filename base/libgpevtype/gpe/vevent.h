/* vevent.h - Event import and export interface.
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

#ifndef GPE_VEVENT_H
#define GPE_VEVENT_H

#include <glib.h>
#include <gpe/event-db.h>
#include <mimedir/mimedir-vevent.h>

/* Import the event EVENT into the calendar EC.  If no error occured
   and NEW_EV is not NULL, return the event in *NEW_EV.  Returns
   whether the import was successful.  */
extern gboolean event_import_from_vevent (EventCalendar *ec,
					  MIMEDirVEvent *event,
					  Event **new_ev, GError **error);


/* Converts event EV to a MIMEDirVEvent.  */
extern MIMEDirVEvent *event_export_as_vevent (Event *ev);

/* Converts event EV to a string.  The caller must free the returned
   string using g_free.  */
extern char *event_export_as_string (Event *ev);

/* Exports the event EV to the file named by FILENAME.  */
extern gboolean event_export_to_file (Event *ev, const gchar *filename,
				      GError **error);

#endif
