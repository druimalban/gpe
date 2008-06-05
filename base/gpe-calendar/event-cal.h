/* event-cal.c - Event calendar widget interface.
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

#ifndef EVENT_CAL_H
#define EVENT_CAL_H

struct _GtkEventCal;
typedef struct _GtkEventCal GtkEventCal;

#define GTK_EVENT_CAL(obj) \
  GTK_CHECK_CAST (obj, gtk_event_cal_get_type (), struct _GtkEventCal)
#define GTK_EVENT_CAL_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gtk_event_cal_get_type (), EventCalClass)
#define GTK_IS_EVENT_CAL(obj) GTK_CHECK_TYPE (obj, gtk_event_cal_get_type ())

typedef enum
{
  GTK_EVENT_CAL_SHOW_HEADING		= 1 << 0,
  GTK_EVENT_CAL_SHOW_DAY_NAMES		= 1 << 1,
  GTK_EVENT_CAL_NO_MONTH_CHANGE		= 1 << 2,
  GTK_EVENT_CAL_SHOW_WEEK_NUMBERS	= 1 << 3,
  GTK_EVENT_CAL_WEEK_START_MONDAY	= 1 << 4
} GtkEventCalDisplayOptions;

/* Return GType of a day view.  */
extern GType gtk_event_cal_get_type (void);

/* Create a new day view.  */
extern GtkWidget *gtk_event_cal_new (void);

/* Force CAL to reload the events from the event database.  */
void gtk_event_cal_reload_events (GtkEventCal *cal);

void gtk_event_cal_get_date (GtkEventCal *calendar, guint *year, guint *month, guint *day);

gboolean gtk_event_cal_select_month (GtkEventCal *calendar, guint month, guint year);

void gtk_event_cal_select_day (GtkEventCal *calendar, guint day);

void gtk_event_cal_set_display_options (GtkEventCal *calendar, GtkEventCalDisplayOptions flags);

#endif
