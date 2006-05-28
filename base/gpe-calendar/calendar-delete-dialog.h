/* calendar-delete-dialog.h - Calendar delete dialog interface.
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

#ifndef CALENDAR_DELETE_DIALOG_H
#define CALENDAR_DELETE_DIALOG_H

#include <gtk/gtk.h>
#include <gpe/event-db.h>

G_BEGIN_DECLS

#define TYPE_CALENDAR_DELETE_DIALOG (calendar_delete_dialog_get_type ())
#define CALENDAR_DELETE_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CALENDAR_DELETE_DIALOG, \
   CalendarDeleteDialog))
#define CALENDAR_DELETE_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CALENDAR_DELETE_DIALOG, \
                            CalendarDeleteDialogClass))
#define IS_CALENDAR_DELETE_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CALENDAR_DELETE_DIALOG))
#define IS_CALENDAR_DELETE_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CALENDAR_DELETE_DIALOG))
#define CALENDAR_DELETE_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CALENDAR_DELETE_DIALOG, \
                              CalendarDeleteDialogClass))

struct _CalendarDeleteDialog;
typedef struct _CalendarDeleteDialog CalendarDeleteDialog;

struct _CalendarDeleteDialogClass;
typedef struct _CalendarDeleteDialogClass CalendarDeleteDialogClass;

extern GType calendar_delete_dialog_get_type (void) G_GNUC_CONST;

/* EC is the calendar to consider.  */
extern GtkWidget *calendar_delete_dialog_new (EventCalendar *ec);

G_END_DECLS

#endif /* CALENDAR_DELETE_DIALOG_H  */
