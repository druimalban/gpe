/* calendar-edit-dialog.h - Calendar edit dialog interface.
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

#ifndef CALENDAR_EDIT_DIALOG_H
#define CALENDAR_EDIT_DIALOG_H

#include <gtk/gtkwindow.h>
#include <gpe/event-db.h>

G_BEGIN_DECLS

#define TYPE_CALENDAR_EDIT_DIALOG (calendar_edit_dialog_get_type ())
#define CALENDAR_EDIT_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CALENDAR_EDIT_DIALOG, \
   CalendarEditDialog))
#define CALENDAR_EDIT_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CALENDAR_EDIT_DIALOG, \
                            CalendarEditDialogClass))
#define IS_CALENDAR_EDIT_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CALENDAR_EDIT_DIALOG))
#define IS_CALENDAR_EDIT_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CALENDAR_EDIT_DIALOG))
#define CALENDAR_EDIT_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CALENDAR_EDIT_DIALOG, \
                              CalendarEditDialogClass))

struct _CalendarEditDialog;
typedef struct _CalendarEditDialog CalendarEditDialog;

struct _CalendarEditDialogClass;
typedef struct _CalendarEditDialogClass CalendarEditDialogClass;

extern GType calendar_edit_dialog_get_type (void) G_GNUC_CONST;

/* If EC is NULL, create a new event.  */
extern GtkWidget *calendar_edit_dialog_new (EventCalendar *ec);

/* Return the EventCalendar (if any).  */
extern EventCalendar *calendar_edit_dialog_get_calendar (CalendarEditDialog *);

G_END_DECLS

#endif /* CALENDAR_EDIT_DIALOG_H  */
