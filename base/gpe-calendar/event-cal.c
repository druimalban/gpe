/* event-cal.c - Event calendar widget implementation.
   Copyright (C) 2006, 2007, 2008 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include <string.h>
#include <gtk/gtk.h>
#if defined(IS_HILDON) && HILDON_VER > 0
#include <hildon/hildon-calendar.h>
#endif
#include <gpe/event-db.h>
#include "globals.h"
#include "event-cal.h"

struct _GtkEventCal
{
#if defined(IS_HILDON) && HILDON_VER > 0
  HildonCalendar widget;
#else
  GtkCalendar widget;
#endif

  /* Current year and month.  */
  int year;
  int month;

  gboolean pending_reload;
};

typedef struct
{
#if defined(IS_HILDON) && HILDON_VER > 0
  HildonCalendarClass hildon_calendar_class;
#else
  GtkCalendarClass gtk_calendar_class;
#endif
} EventCalClass;

static void gtk_event_cal_base_class_init (gpointer klass,
					   gpointer klass_data);
static void gtk_event_cal_init (GTypeInstance *instance, gpointer klass);
static void gtk_event_cal_dispose (GObject *obj);
static void gtk_event_cal_finalize (GObject *object);
static gboolean gtk_event_cal_expose_event (GtkWidget *widget,
					    GdkEventExpose *event);
#if defined(IS_HILDON) && HILDON_VER > 0
static void gtk_event_cal_month_changed (HildonCalendar *cal);
#else
static void gtk_event_cal_month_changed (GtkCalendar *cal);
#endif
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
	NULL,
	NULL,
	gtk_event_cal_base_class_init,
	NULL,
	NULL,
	sizeof (struct _GtkEventCal),
	0,
	gtk_event_cal_init
      };

#if defined(IS_HILDON) && HILDON_VER > 0
      type = g_type_register_static (hildon_calendar_get_type (),
				     "GtkEventCal", &info, 0);
#else
      type = g_type_register_static (gtk_calendar_get_type (),
				     "GtkEventCal", &info, 0);
#endif
   }

  return type;
}

static void
gtk_event_cal_base_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
#if defined(IS_HILDON) && HILDON_VER > 0
  HildonCalendarClass *calendar_class;
#else
  GtkCalendarClass *calendar_class;
#endif

#if defined(IS_HILDON) && HILDON_VER > 0
  parent_class = g_type_class_ref (hildon_calendar_get_type ());
#else
  parent_class = g_type_class_ref (gtk_calendar_get_type ());
#endif

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_event_cal_finalize;
  object_class->dispose = gtk_event_cal_dispose;

  widget_class = (GtkWidgetClass *) klass;
  widget_class->expose_event = gtk_event_cal_expose_event;

#if defined(IS_HILDON) && HILDON_VER > 0
  calendar_class = (HildonCalendarClass *) klass;
#else
  calendar_class = (GtkCalendarClass *) klass;
#endif
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

#if defined(IS_HILDON) && HILDON_VER > 0
static void
update_cal (HildonCalendar *cal)
#else
static void
update_cal (GtkCalendar *cal)
#endif
{
  GtkEventCal *event_cal = GTK_EVENT_CAL (cal);
  unsigned int year, month, day;
  struct tm start_tm;
  struct tm end_tm;
  int days;
  time_t start, end;
  GSList *events;
  GSList *e;

#if defined(IS_HILDON) && HILDON_VER > 0
  hildon_calendar_get_date (cal, &year, &month, &day);

  hildon_calendar_freeze (cal);
  hildon_calendar_clear_marks (cal);
#else
  gtk_calendar_get_date (cal, &year, &month, &day);

  gtk_calendar_freeze (cal);
  gtk_calendar_clear_marks (cal);
#endif

  event_cal->year = year;
  event_cal->month = month;

  memset (&start_tm, 0, sizeof (start_tm));
  start_tm.tm_year = year - 1900;
  start_tm.tm_mon = month;
  start_tm.tm_mday = 1;
  start_tm.tm_hour = 0;
  start_tm.tm_min = 0;
  start_tm.tm_sec = 0;
  start = mktime (&start_tm);

  end_tm = start_tm;
  days = g_date_get_days_in_month (month + 1, year - 1900);
  end_tm.tm_mday = days;
  end_tm.tm_hour = 23;
  end_tm.tm_min = 59;
  end_tm.tm_sec = 59;
  end = mktime (&end_tm);

  events = event_db_list_for_period (event_db, start, end, NULL);

  for (e = events; e; e = e->next)
    {
      Event *ev = e->data;
      struct tm tm;
      time_t t;
      int start, end;
      int i;

      if (! event_get_visible (ev, NULL))
	continue;

      t = event_get_start (ev);
      localtime_r (&t, &tm);
      if (tm.tm_year < start_tm.tm_year)
	start = 0;
      else
	start = MAX (tm.tm_yday - start_tm.tm_yday + 1, 0);

      if (event_get_untimed (ev))
	end = start;
      else
	{
	  t = event_get_start (ev) + event_get_duration (ev) - 1;
	  localtime_r (&t, &tm);
	  if (end_tm.tm_year < tm.tm_year)
	    end = days;
	  else
	    end = MIN (tm.tm_yday - start_tm.tm_yday + 1, days);
	}

      for (i = start; i <= end; i ++)
#if defined(IS_HILDON) && HILDON_VER > 0
	hildon_calendar_mark_day (cal, i);
#else
	gtk_calendar_mark_day (cal, i);
#endif
    }
  event_list_unref (events);
#if defined(IS_HILDON) && HILDON_VER > 0
  hildon_calendar_thaw (cal);
#else
  gtk_calendar_thaw (cal);
#endif

  event_cal->pending_reload = FALSE;
}

static gboolean
gtk_event_cal_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  GtkEventCal *ec = GTK_EVENT_CAL (widget);
  if (ec->pending_reload)
#if defined(IS_HILDON) && HILDON_VER > 0
    update_cal (HILDON_CALENDAR (ec));
#else
    update_cal (GTK_CALENDAR (ec));
#endif

  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}

#if defined(IS_HILDON) && HILDON_VER > 0
static void
gtk_event_cal_month_changed (HildonCalendar *cal)
#else
static void
gtk_event_cal_month_changed (GtkCalendar *cal)
#endif
{
  GtkEventCal *ec = GTK_EVENT_CAL (cal);
  
  unsigned int year, month, day;
#if defined(IS_HILDON) && HILDON_VER > 0
  hildon_calendar_get_date (cal, &year, &month, &day);
#else
  gtk_calendar_get_date (cal, &year, &month, &day);
#endif
  if (ec->year == year && ec->month == month)
    return;

  ec->pending_reload = TRUE;
}

void
gtk_event_cal_reload_events (GtkEventCal *ecal)
{
  ecal->pending_reload = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (ecal));
}

GtkWidget *
gtk_event_cal_new (void)
{
  GtkWidget *widget = g_object_new (gtk_event_cal_get_type (), NULL);

  gtk_event_cal_reload_events (GTK_EVENT_CAL (widget));

  return widget;
}

void gtk_event_cal_get_date (GtkEventCal *calendar, guint *year, guint *month, guint *day)
{
#if defined(IS_HILDON) && HILDON_VER > 0
  return hildon_calendar_get_date(HILDON_CALENDAR(calendar), year, month, day);
#else
  return gtk_calendar_get_date(GTK_CALENDAR(calendar), year, month, day);
#endif
}

gboolean gtk_event_cal_select_month (GtkEventCal *calendar, guint month, guint year)
{
#if defined(IS_HILDON) && HILDON_VER > 0
  return hildon_calendar_select_month(HILDON_CALENDAR(calendar), month, year);
#else
  return gtk_calendar_select_month(GTK_CALENDAR(calendar), month, year);
#endif
}

void gtk_event_cal_select_day (GtkEventCal *calendar, guint day)
{
#if defined(IS_HILDON) && HILDON_VER > 0
  return hildon_calendar_select_day(HILDON_CALENDAR(calendar), day);
#else
  return gtk_calendar_select_day(GTK_CALENDAR(calendar), day);
#endif
}

void gtk_event_cal_set_display_options (GtkEventCal *calendar, GtkEventCalDisplayOptions flags)
{
#if defined(IS_HILDON) && HILDON_VER > 0
  g_assert(HILDON_CALENDAR_SHOW_HEADING == GTK_EVENT_CAL_SHOW_HEADING);
  g_assert(HILDON_CALENDAR_SHOW_DAY_NAMES == GTK_EVENT_CAL_SHOW_DAY_NAMES);
  g_assert(HILDON_CALENDAR_NO_MONTH_CHANGE == GTK_EVENT_CAL_NO_MONTH_CHANGE);
  g_assert(HILDON_CALENDAR_SHOW_WEEK_NUMBERS == GTK_EVENT_CAL_SHOW_WEEK_NUMBERS);
  g_assert(HILDON_CALENDAR_WEEK_START_MONDAY == GTK_EVENT_CAL_WEEK_START_MONDAY);
  return hildon_calendar_set_display_options(HILDON_CALENDAR(calendar), (HildonCalendarDisplayOptions)flags);
#else
  g_assert(GTK_CALENDAR_SHOW_HEADING == GTK_EVENT_CAL_SHOW_HEADING);
  g_assert(GTK_CALENDAR_SHOW_DAY_NAMES == GTK_EVENT_CAL_SHOW_DAY_NAMES);
  g_assert(GTK_CALENDAR_NO_MONTH_CHANGE == GTK_EVENT_CAL_NO_MONTH_CHANGE);
  g_assert(GTK_CALENDAR_SHOW_WEEK_NUMBERS == GTK_EVENT_CAL_SHOW_WEEK_NUMBERS);
  g_assert(GTK_CALENDAR_WEEK_START_MONDAY == GTK_EVENT_CAL_WEEK_START_MONDAY);
  return gtk_calendar_set_display_options(GTK_CALENDAR(calendar), (GtkCalendarDisplayOptions)flags);
#endif
}
