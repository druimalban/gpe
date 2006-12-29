/* vcal.h - Calendar import and export interface.
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

#ifndef GPE_VCAL_H
#define GPE_VCAL_H

#include <glib.h>
#include <gpe/event-db.h>

/* Import the calendar contained in CHANNEL.  Parse it and stuff it
   into EC (which may not be NULL).  Returns whether an error occured
   during import.  */
extern gboolean cal_import_from_channel (EventCalendar *ec,
					 GIOChannel *channel,
					 GError **error);

/* Import the list of VComponents, CALLIST, into calendar EC.  Returns
   whether an error occurred during import.  */
extern gboolean cal_import_from_vmimedir (EventCalendar *ec,
					  GList *callist,
					  GError **error);



/* Converts calendar EC to a string.  The caller must free the
   returned string using g_free.  */
extern char *cal_export_as_string (EventCalendar *ec);

/* Exports the calendar EC to the file named by FILENAME.  */
extern gboolean cal_export_to_file (EventCalendar *ec,
				    const gchar *filename,
				    GError **error);

/* Exports the list of Events and EventCalendars to the file named by
   FILENAME.  */
extern gboolean list_export_to_file (GSList *things, const gchar *filename,
				     GError **error);

#endif
