/* alarms.c - Alarm Dialog Interface.
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

#ifndef ALARM_DIALOG_H
#define ALARM_DIALOG_H

#include <gpe/soundgen.h>
#include <gtk/gtkwindow.h>
#include <gpe/event-db.h>

G_BEGIN_DECLS

#define TYPE_ALARM_DIALOG             (alarm_dialog_get_type ())
#define ALARM_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_ALARM_DIALOG, AlarmDialog))
#define ALARM_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_ALARM_DIALOG, AlarmDialogClass))
#define IS_ALARM_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_ALARM_DIALOG))
#define IS_ALARM_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_ALARM_DIALOG))
#define ALARM_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_ALARM_DIALOG, AlarmDialogClass))

struct _AlarmDialog;
typedef struct _AlarmDialog AlarmDialog;

struct _AlarmDialogClass;
typedef struct _AlarmDialogClass AlarmDialogClass;

extern GType alarm_dialog_get_type (void) G_GNUC_CONST;

/* The user clicked on the "show event" button for this event.  */
typedef void (*AlarmDialogShowEventFunc) (AlarmDialog *, Event *);

extern GtkWidget *alarm_dialog_new (void);

extern void alarm_dialog_add_event (AlarmDialog *alarm_dialog, Event *event);

G_END_DECLS

#endif /* ALARM_DIALOG_H  */
