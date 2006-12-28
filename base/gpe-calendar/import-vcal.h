/* import-vcal.h - Import calendar interface.
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

#ifndef IMPORT_VCAL_H
#define IMPORT_VCAL_H

#include <glib.h>
#include <gpe/event-db.h>
#include <mimedir/mimedir-vevent.h>

/* Import the event EVENT into the calendar EC.  If no error occured
   and NEW_EV is not NULL, return the event in *NEW_EV.  Returns
   whether the import was successful.  */
extern gboolean import_vevent (EventCalendar *ec, MIMEDirVEvent *event,
			       Event **new_ev, GError **error);

/* Import the list of files (NULL terminated) into calendar EC.
   Either may be NULL in which case the user will be prompted using
   the GUI.  (If neither is NULL then a GUI is not required.)  If the
   user cancels the GUI, the import is aborted (and FALSE is returned
   but no error message).  Returns whether an error occurred during
   import.  */
extern gboolean import_vcal (EventCalendar *ec, const char *files[],
			     GError **error);

/* CHANNEL should contain an iCal object.  Parse it and stuff it into
   EC (which may not be NULL).  */
extern gboolean import_vcal_from_channel (EventCalendar *ec,
					  GIOChannel *channel,
					  GError **error);

#endif
