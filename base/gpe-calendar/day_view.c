/*
 * Copyright (C) 2001, 2002, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <string.h>
#include <time.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/event-db.h>
#include <gpe/spacing.h>
#include "view.h"
#include "event-ui.h"
#include "globals.h"
#include "day_view.h"
#include "day_render.h"
#include "calendars-widgets.h"
#include "event-menu.h"

#define NUM_HOURS 24

struct _GtkDayView
{
  GtkView widget;

  /*
      Basic layout of a DayView:

       /-DayView----------------\
       | /-reminders----------\ |
       | |                    | |
       | \--------------------/ |
       | /-appointment_window-\ |
       | |/-appointments-----\| |
       | ||                  || |
       | |\------------------/| |
       | \--------------------/ |
       \------------------------/
   */

  gboolean scrolling;
  gboolean scroll_floating;

  GtkDayRender *reminders;
  GtkWidget *appointment_window;
  GtkDayRender *appointments;
};

typedef struct
{
  GtkViewClass view_class;
} DayViewClass;

static void gtk_day_view_base_class_init (gpointer klass, gpointer klass_data);
static void gtk_day_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_day_view_dispose (GObject *obj);
static void gtk_day_view_finalize (GObject *object);
static gboolean gtk_day_view_key_press_event (GtkWidget *widget,
					      GdkEventKey *event);
static void gtk_day_view_set_time (GtkView *view, time_t time);
static void gtk_day_view_reload_events (GtkView *view);

static GtkWidgetClass *parent_class;

GType
gtk_day_view_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (DayViewClass),
	NULL,
	NULL,
	gtk_day_view_base_class_init,
	NULL,
	NULL,
	sizeof (struct _GtkDayView),
	0,
	gtk_day_view_init
      };

      type = g_type_register_static (gtk_view_get_type (),
				     "GtkDayView", &info, 0);
    }

  return type;
}

static void
gtk_day_view_base_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_view_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_day_view_finalize;
  object_class->dispose = gtk_day_view_dispose;

  widget_class = (GtkWidgetClass *) klass;
  widget_class->key_press_event = gtk_day_view_key_press_event;

  view_class = (GtkViewClass *) klass;
  view_class->set_time = gtk_day_view_set_time;
  view_class->reload_events = gtk_day_view_reload_events;
}

static void
gtk_day_view_init (GTypeInstance *instance, gpointer klass)
{
}

static void
gtk_day_view_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_day_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gtk_day_view_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  switch (event->keyval)
    {
    case GDK_Left:
    case GDK_Up:
    case GDK_Page_Up:
      gtk_view_set_time (GTK_VIEW (widget),
			 gtk_view_get_time (GTK_VIEW (widget))
			 - 24 * 60 * 60);
      return TRUE;
    case GDK_Right:
    case GDK_Down:
    case GDK_Page_Down:
      gtk_view_set_time (GTK_VIEW (widget),
			 gtk_view_get_time (GTK_VIEW (widget))
			 + 24 * 60 * 60);
      return TRUE;
    default:
      return FALSE;
    }
}

static void
gtk_day_view_set_time (GtkView *view, time_t current)
{
  time_t new = gtk_view_get_time (view);
  struct tm c_tm;
  struct tm n_tm;

  localtime_r (&current, &c_tm);
  localtime_r (&new, &n_tm);

  if (c_tm.tm_year != n_tm.tm_year || c_tm.tm_yday != n_tm.tm_yday)
    /* Day changed.  */
    gtk_day_view_reload_events (view);
}

static gboolean
day_view_row_clicked (GtkWidget *widget, gint row, gpointer d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);
  time_t t = gtk_view_get_time (GTK_VIEW (day_view));
  struct tm tm;
  GtkWidget *w;

  localtime_r (&t, &tm);
  tm.tm_hour = row;
  tm.tm_min = 0;
  tm.tm_isdst = -1;

  w = new_event (mktime (&tm));
  gtk_widget_show (w);

  return FALSE;
}

static gboolean
day_view_event_clicked (GtkWidget *widget, gpointer event_p, gpointer d)
{
  GtkMenu *event_menu = event_menu_new (EVENT (event_p), TRUE);
  gtk_menu_popup (event_menu, NULL, NULL, NULL, NULL,
		  0, gtk_get_current_event_time());
  return FALSE;
}

static void
gtk_day_view_reload_events (GtkView *view)
{
  GtkDayView *day_view = GTK_DAY_VIEW (view);
  time_t t = gtk_view_get_time (view);
  struct tm vt;
  time_t end, start;
  GSList *reminders;
  GSList *events, *appointments, *iter;

  localtime_r (&t, &vt);
  vt.tm_hour = 0;
  vt.tm_min = 0;
  vt.tm_sec = 0;
  vt.tm_isdst = -1;
  start = mktime (&vt);
  end = start + 30 * 60 * 60 - 1;

  /* Get the appointments for the current period.  */
  events = event_db_list_for_period (event_db, start, end);
  reminders = NULL;
  appointments = NULL;
  for (iter = events; iter; iter = iter->next)
    {
      Event *ev = iter->data;
      if (event_get_start (ev) <= start
	  && (start + 24 * 60 * 60
	      <= event_get_start (ev) + event_get_duration (ev)))
	  /* All day event or a multi-day event.  */
	reminders = g_slist_prepend (reminders, ev);
      else if (event_get_duration (ev) == 0
	       && event_get_start (ev) < start + 24 * 60 * 60)
	/* Normal reminder.  */
	reminders = g_slist_prepend (reminders, ev);
      else if (event_get_duration (ev) == 0
	       || (event_get_start (ev) == start + 24 * 60 * 60
		   && event_get_duration (ev) == 24 * 60 * 60))
	/* An all day event or reminder which occurs tomorrow.  */
	g_object_unref (ev);
      else
	appointments = g_slist_prepend (appointments, ev);
    }

  g_slist_free (events);

  if (! reminders && day_view->reminders)
    /* There are no longer any reminders, destroy its render area.  */
    {
      gtk_container_remove (GTK_CONTAINER (day_view),
			    GTK_WIDGET (day_view->reminders));
      day_view->reminders = NULL;
    }

  if (reminders)
    {
      if (! day_view->reminders)
	/* We have reminders but no render area.  Create one.  */
	{
	  GdkColor cream;
	  cream.red = 65535;
	  cream.green = 64005;
	  cream.blue = 51100;

	  day_view->reminders
	    = GTK_DAY_RENDER (gtk_day_render_new (start, 24 * 60 * 60,
						  1, 0, 0,
						  cream, 1, FALSE,
						  reminders));

	  g_signal_connect (G_OBJECT (day_view->reminders), "event-clicked",
			    G_CALLBACK (day_view_event_clicked), NULL);
	  g_signal_connect (G_OBJECT (day_view->reminders), "row-clicked",
			    G_CALLBACK (day_view_row_clicked), day_view);

	  gtk_widget_show (GTK_WIDGET (day_view->reminders));

	  gtk_box_pack_start (GTK_BOX (day_view),
			      GTK_WIDGET (day_view->reminders),
			      FALSE, FALSE, 0);
	}
      else
	/* Set the new reminder list.  */
	gtk_day_render_set_events (day_view->reminders, reminders, start);
    }

  if (day_view->appointments)
    gtk_day_render_set_events (day_view->appointments, appointments, start);
  else
    {
      /* Don't want to infinge some patents here */
      GdkColor post_him_yellow;
      post_him_yellow.red = 63222;
      post_him_yellow.green = 59110;
      post_him_yellow.blue = 33667;

      day_view->appointments
	= GTK_DAY_RENDER (gtk_day_render_new (start, 30 * 60 * 60,
					      30, 7, 12,
					      post_him_yellow,
					      4, TRUE,
					      appointments));

      g_signal_connect (G_OBJECT (day_view->appointments), "event-clicked",
			G_CALLBACK (day_view_event_clicked), NULL);
      g_signal_connect (G_OBJECT (day_view->appointments), "row-clicked",
			G_CALLBACK (day_view_row_clicked), day_view);

      gtk_widget_show (GTK_WIDGET (day_view->appointments));

      gtk_scrolled_window_add_with_viewport
	(GTK_SCROLLED_WINDOW (day_view->appointment_window),
	 GTK_WIDGET (day_view->appointments));

      gtk_viewport_set_shadow_type
	(GTK_VIEWPORT (GTK_WIDGET (day_view->appointments)->parent),
	 GTK_SHADOW_NONE);
    }
}

/* 
 * Go to hour h
 * If h is negative it goes to the current hour
 */
static void
scroll_to (GtkDayView *day_view, gint hour)
{
  time_t t = gtk_view_get_time (GTK_VIEW (day_view));
  GtkAdjustment *adj;
  gint h = 0;
  gdouble upper, lower, value;

  /* Scroll to the current hour */
  if (hour < 0)
    {
      struct tm now_tm;
      localtime_r (&t, &now_tm);
      hour = MAX (now_tm.tm_hour - 1, 0);
    }

  gtk_widget_get_size_request (GTK_WIDGET (day_view->appointments), NULL, &h);
  value = (gdouble) hour / NUM_HOURS * h;
  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
					     (day_view->appointment_window));

  lower = 0.0;
  upper = h;

  if (value > (adj->upper - adj->page_size))
    value = adj->upper - adj->page_size;

  day_view->scrolling = TRUE;
  gtk_adjustment_set_value (adj, value);
  day_view->scrolling = FALSE;
}

void
gtk_day_view_scroll (GtkDayView *day_view, gboolean force)
{
  if (force || day_view->scroll_floating)
    {
      scroll_to (day_view, -1);
      day_view->scroll_floating = TRUE;
    }
}

static void
sink_scroller (GtkAdjustment *adjustment, gpointer data)
{
  GtkDayView *day_view = GTK_DAY_VIEW (data);
  if (!day_view->scrolling)
    day_view->scroll_floating = FALSE;
}

GtkWidget *
gtk_day_view_new (time_t time)
{
  GtkDayView *day_view;
  GtkAdjustment *adj;

  day_view = GTK_DAY_VIEW (g_object_new (gtk_day_view_get_type (), NULL));
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (day_view), GTK_CAN_FOCUS);

  day_view->appointment_window = gtk_scrolled_window_new (NULL, NULL);
  adj = gtk_scrolled_window_get_vadjustment
    (GTK_SCROLLED_WINDOW (day_view->appointment_window));
  g_signal_connect (G_OBJECT (adj), "value-changed",
		    G_CALLBACK (sink_scroller), (gpointer) day_view);
  gtk_scrolled_window_set_policy
    (GTK_SCROLLED_WINDOW (day_view->appointment_window),
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end (GTK_BOX (day_view), day_view->appointment_window,
		    TRUE, TRUE, 0);
  gtk_widget_show (day_view->appointment_window);

  gtk_view_set_time (GTK_VIEW (day_view), time);

  return GTK_WIDGET (day_view);
}
