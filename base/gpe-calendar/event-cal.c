/* event-cal.c - Event calendar widget implementation.
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

#include <gtk/gtk.h>
#include <gpe/event-db.h>
#include "globals.h"
#include "event-cal.h"

#define _(x) gettext(x)

struct _GtkEventCal
{
  GtkCalendar widget;

  /* Current year and month.  */
  int year;
  int month;
};

typedef struct
{
  GtkVBoxClass vbox_class;
  GObjectClass parent_class;
} EventCalClass;

static void gtk_event_cal_base_class_init (gpointer klass);
static void gtk_event_cal_init (GTypeInstance *instance, gpointer klass);
static void gtk_event_cal_dispose (GObject *obj);
static void gtk_event_cal_finalize (GObject *object);
static void gtk_event_cal_month_changed (GtkCalendar *cal);

static GtkWidgetClass *parent_class;

GType
gtk_event_cal_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventCalClass),
	gtk_event_cal_base_class_init,
	NULL,
	NULL,
	NULL,
	NULL,
	sizeof (struct _GtkEventCal),
	0,
	gtk_event_cal_init
      };

      type = g_type_register_static (gtk_calendar_get_type (),
				     "GtkEventCal", &info, 0);
    }

  return type;
}

static void
gtk_event_cal_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkCalendarClass *calendar_class;

  parent_class = g_type_class_ref (gtk_vbox_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_event_cal_finalize;
  object_class->dispose = gtk_event_cal_dispose;

  widget_class = (GtkWidgetClass *) klass;

  calendar_class = (GtkCalendarClass *) klass;
  calendar_class->month_changed = gtk_event_cal_month_changed;
}

static void
gtk_event_cal_init (GTypeInstance *instance, gpointer klass)
{
  GtkEventCal *event_cal = GTK_EVENT_CAL (instance);

  event_cal->year = 0;
  event_cal->month = 0;
}

static void
gtk_event_cal_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_event_cal_finalize (GObject *object)
{
  g_return_if_fail (GTK_IS_EVENT_CAL (object));
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
update_cal (GtkCalendar *cal, gboolean force)
{
  GtkEventCal *event_cal = GTK_EVENT_CAL (cal);
  int year, month, day;
  struct tm start_tm;
  struct tm end_tm;
  int days;
  time_t start, end;
  GSList *events;
  GSList *e;

  gtk_calendar_get_date (cal, &year, &month, &day);
  if (! force && event_cal->year == year && event_cal->month == month)
    return;

  gtk_calendar_clear_marks (cal);

  event_cal->year = year;
  event_cal->month = month;

  start_tm.tm_year = year - 1900;
  start_tm.tm_mon = month;
  start_tm.tm_mday = 1;
  start_tm.tm_hour = 0;
  start_tm.tm_min = 0;
  start_tm.tm_sec = 0;
  start = mktime (&start_tm);

  end_tm = start_tm;
  days = days_in_month (year - 1900, month);
  end_tm.tm_mday = days;
  end_tm.tm_hour = 23;
  end_tm.tm_min = 59;
  end_tm.tm_sec = 59;
  end = mktime (&end_tm);

  events = event_db_list_for_period (start, end);

  for (e = events; e; e = e->next)
    {
      event_t ev = e->data;
      struct tm tm;
      time_t t;
      int start, end;
      int i;

      localtime_r (&ev->start, &tm);
      if (tm.tm_year < start_tm.tm_year)
	start = 0;
      else
	start = MAX (tm.tm_yday - start_tm.tm_yday + 1, 0);

      t = ev->start + ev->duration;
      localtime_r (&t, &tm);
      if (end_tm.tm_year < tm.tm_year)
	end = days;
      else
	end = MIN (tm.tm_yday - start_tm.tm_yday + 1, days);

      for (i = start; i <= end; i ++)
	gtk_calendar_mark_day (cal, i);
    }
  event_db_list_destroy (events);
}

static void
gtk_event_cal_month_changed (GtkCalendar *cal)
{
  update_cal (cal, FALSE);
}

void
gtk_event_cal_reload_events (GtkEventCal *ecal)
{
  update_cal (GTK_CALENDAR (ecal), TRUE);
}

GtkWidget *
gtk_event_cal_new (void)
{
  GtkWidget *widget = g_object_new (gtk_event_cal_get_type (), NULL);

  update_cal (GTK_CALENDAR (widget), TRUE);

  return widget;
}
