/* import-export.h - Import and export interfaces.
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

#ifndef IMPORT_EXPORT_H
#define IMPORT_EXPORT_H

/* Import the list of files (NULL terminated) into calendar EC.
   Either may be NULL in which case the user will be prompted using
   the GUI.  (If neither is NULL then a GUI is not required.)  If the
   user cancels the GUI, the import is aborted (and FALSE is returned
   but no error message).  Returns whether an error occurred during
   import.  */
extern gboolean cal_import_from_files (EventCalendar *ec, const char *files[],
				       GError **gerror);

/* This is just a wrapper for import_vcal which displays the results
   to the user.  */
extern void cal_import_dialog (EventCalendar *ec, const char *files[]);


/* Displays a dialogue allow the user to save event EV to a file.  */
extern void event_export_dialog (Event *ev);

/* Displays a dialogue allow the user to save calendar EC to a
   file.  */
extern void cal_export_dialog (EventCalendar *ec);


/* Whether bluetooth and irda are available.  */
gboolean export_bluetooth_available (void);
gboolean export_irda_available (void);

/* Send EVENT via bluetooth or irda.  Requires that the library be
   compiled with dbus support.  */
void vcal_do_send_bluetooth (Event *event);
void vcal_do_send_irda (Event *event);

#endif /* IMPORT_EXPORT_H */
