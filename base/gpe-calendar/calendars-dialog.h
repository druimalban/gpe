/* calendars-dialog.h - Calendars dialog manipulation interface.
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

#ifndef CALENDARS_DIALOG_H
#define CALENDARS_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_CALENDARS_DIALOG (calendars_dialog_get_type ())
#define CALENDARS_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CALENDARS_DIALOG, CalendarsDialog))
#define CALENDARS_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CALENDARS_DIALOG, \
   CalendarsDialogClass))
#define IS_CALENDARS_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CALENDARS_DIALOG))
#define IS_CALENDARS_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CALENDARS_DIALOG))
#define CALENDARS_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CALENDARS_DIALOG, \
   CalendarsDialogClass))

struct _CalendarsDialog;
typedef struct _CalendarsDialog CalendarsDialog;

struct _CalendarsDialogClass;
typedef struct _CalendarsDialogClass CalendarsDialogClass;

extern GType calendars_dialog_get_type (void) G_GNUC_CONST;

extern GtkWidget *calendars_dialog_new (EventDB *edb);

G_END_DECLS

#endif /* CALENDARS_DIALOG_H  */
