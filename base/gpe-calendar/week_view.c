/*
 * Copyright (C) 2001, 2002, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <time.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/event-db.h>
#include <gpe/stylus.h>

#include "view.h"
#include "globals.h"
#include "week_view.h"
#include "day_popup.h"

#include "gtkdatesel.h"
#include "event-cal.h"
#include "event-list.h"

#define _(x) gettext(x)

struct week_day
{
  /* Valid if HAVE_EXTENTS is true.  */
  int top;
  int height;

  /* Banner to display for this day.  */
  gchar *banner;

  /* The list of events.  */
  GSList *events;

  struct day_popup popup;
};

struct _GtkWeekView
{
  GtkView widget;

  struct week_day days[7];
  gboolean have_extents;

  GtkWidget *draw;
  GtkWidget *scroller;

  /* The week day (if any) which the popup menu is for.  */
  struct week_day *has_popup;

  /* Width required by the banners, valid if HAVE_EXTENTS is true.  */
  gint banner_width;
  /* Width required by the time fields, valid if HAVE_EXTENTS is true.  */
  gint time_width;

  /* Current canvas width and height.  */
  gint width, height;

  /* Day with the focus.  */
  int focused_day;
};

typedef struct
{
  GtkViewClass view_class;
  GObjectClass parent_class;
} GtkWeekViewClass;

static void gtk_week_view_base_class_init (gpointer klass);
static void gtk_week_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_week_view_dispose (GObject *obj);
static void gtk_week_view_finalize (GObject *object);
static void gtk_week_view_set_time (GtkView *view, time_t time);
static void gtk_week_view_reload_events (GtkView *view);

static GtkWidgetClass *parent_class;

GType
gtk_week_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (GtkWeekViewClass),
	gtk_week_view_base_class_init,
	NULL,
	NULL,
	NULL,
	NULL,
	sizeof (struct _GtkWeekView),
	0,
	gtk_week_view_init
      };

      type = g_type_register_static (gtk_view_get_type (),
				     "GtkWeekView", &info, 0);
    }

  return type;
}

static void
gtk_week_view_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_view_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_week_view_finalize;
  object_class->dispose = gtk_week_view_dispose;

  widget_class = (GtkWidgetClass *) klass;

  view_class = (GtkViewClass *) klass;
  view_class->set_time = gtk_week_view_set_time;
  view_class->reload_events = gtk_week_view_reload_events;
}

static void
gtk_week_view_init (GTypeInstance *instance, gpointer klass)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (instance);

  week_view->have_extents = FALSE;
  week_view->height = 0;
  week_view->width = 0;
  week_view->has_popup = 0;
}

static void
gtk_week_view_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_week_view_finalize (GObject *object)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (object);
  int day;

  for (day = 0; day < 7; day++)
    {
      struct week_day *d = &week_view->days[day];
      if (d->banner)
	g_free (d->banner);
      if (d->events)
        {
          event_list_unref (d->events);
          d->events = NULL;
        }
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_week_view_invalidate_day (GtkWeekView *week_view, gint i)
{
  struct week_day *day = &week_view->days[i];

  gtk_widget_queue_draw_area (week_view->draw, 0, day->top,
			      week_view->width, day->height);
}

static void
gtk_week_view_set_time (GtkView *view, time_t current)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (view);
  time_t new = gtk_view_get_time (view);
  struct tm c_tm;
  struct tm n_tm;

  localtime_r (&new, &n_tm);
  localtime_r (&current, &c_tm);

  /* Determine if we need to update the cached information.  */
  int start_of_week = n_tm.tm_yday - n_tm.tm_wday;
  if (week_starts_monday)
    {
      if (n_tm.tm_wday == 0)
	start_of_week -= 6;
      else
	start_of_week ++;
    }

  if (n_tm.tm_year == c_tm.tm_year
      && start_of_week <= c_tm.tm_yday && c_tm.tm_yday < start_of_week + 7)
    /* Same week.  */
    {
      if (n_tm.tm_yday - start_of_week != week_view->focused_day)
	/* Different day: change the focus.  */
	{
	  gtk_week_view_invalidate_day (week_view, week_view->focused_day);
	  week_view->focused_day = n_tm.tm_yday - start_of_week;
	  gtk_week_view_invalidate_day (week_view, week_view->focused_day);
	}
      return;
    }

  gtk_week_view_reload_events (view);
}

static gint
draw_expose_event (GtkWidget *widget, GdkEventExpose *event, GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);
  GdkDrawable *drawable = widget->window;
  GdkGC *black_gc;
  GdkGC *white_gc;
  GdkGC *blue_gc;
  GdkGC *yellow_gc;
  GdkGC *salmon_gc;
  GdkColor yellow;
  GdkColor salmon;
  GdkColormap *colormap;
  PangoLayout *pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
  PangoLayout *pl_evt = gtk_widget_create_pango_layout (GTK_WIDGET (widget),
							NULL);
  gboolean wide_mode = widget->allocation.width > widget->allocation.height;
  int i;

  blue_gc = pen_new (widget, 0, 0, 0xffff);

  colormap = gdk_window_get_colormap (widget->window);
  yellow_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (yellow_gc, widget->style->black_gc);
  gdk_color_parse ("palegoldenrod", &yellow);
  gdk_colormap_alloc_color (colormap, &yellow, FALSE, TRUE);
  gdk_gc_set_foreground (yellow_gc, &yellow);

  salmon_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (salmon_gc, widget->style->black_gc);
  gdk_color_parse ("light salmon", &salmon);
  gdk_colormap_alloc_color (colormap, &salmon, FALSE, TRUE);
  gdk_gc_set_foreground (salmon_gc, &salmon);

  white_gc = widget->style->white_gc;
  black_gc = widget->style->black_gc;

  int day_start, day_end;
  int top;
  if (week_view->have_extents)
    {
      /* Find the first day with damage.  */
      for (day_start = 0; day_start < 7; day_start ++)
	if (event->area.y
	    < (week_view->days[day_start].top
	       + week_view->days[day_start].height))
	  break;

      top = week_view->days[day_start].top;

      /* Find the last day with damage.  */
      for (day_end = day_start; day_end < 7; day_end ++)
	if (week_view->days[day_end].top + week_view->days[day_end].height
	    >= event->area.y + event->area.height)
	  break;
      day_end = MIN (day_end, 6);
    }
  else
    {
      day_start = 0;
      day_end = 6;
      top = 0;

      /* Calculate WEEKVIEW->BANNER_WIDTH, the minimum width required to
	 display each day's banner.  */
      int day;
      week_view->banner_width = 0;
      for (day = 0; day < 7; day++)
	{
	  PangoRectangle pr;
    
	  pango_layout_set_markup (pl, week_view->days[day].banner, -1);
	  pango_layout_get_pixel_extents (pl, NULL, &pr);
	  week_view->banner_width = MAX (pr.width, week_view->banner_width);
	}

      /* Calculate WEEK_VIEW->TIME_WIDTH, the maximum width required by
	 any event's time stamp.  */
      GSList *iter;
      week_view->time_width = 0;
      for (day = 0; day < 7; day++)
	for (iter = week_view->days[day].events; iter; iter = iter->next)
	  {
	    Event *ev = iter->data;
	    if (event_get_untimed (ev))
	      /* Reminder, i.e. no time.  */
	      continue;

	    time_t t = event_get_start (ev);
	    struct tm tm;
	    localtime_r (&t, &tm);

	    if (! (tm.tm_year == week_view->days[day].popup.year
		   && tm.tm_mon == week_view->days[day].popup.month
		   && tm.tm_mday == week_view->days[day].popup.day))
	      /* Doesn't start today.  As such, we don't show the
		 date.  */
	      continue;

	    PangoRectangle pr;

	    gchar *buffer = strftime_strdup_utf8_locale (TIMEFMT, &tm);
	    pango_layout_set_text (pl_evt, buffer, -1);
	    g_free (buffer);
	    pango_layout_get_pixel_extents (pl_evt, NULL, &pr);

	    week_view->time_width = MAX (pr.width, week_view->time_width);
	  }
      if (week_view->time_width > 0)
	/* Add a small gap.  */
	week_view->time_width += 5;
    }

  /* Calculate the amount of space for the text.  */
  int text_width = week_view->draw->allocation.width - week_view->time_width;
  if (wide_mode)
    text_width -= week_view->banner_width;

  /* Display each day.  */
  for (i = day_start; i <= day_end; i ++)
    {
      struct week_day *day = &week_view->days[i];
      GdkGC *color;
      GSList *iter;
      PangoRectangle pr;
      int left = 4;
      int banner_height;

      day->top = top;

      /* Color the day's background appropriately.  */
      if (day->events)
	color = white_gc;
      else
	{
	  if (week_starts_monday)
	    color = i >= 5 ? salmon_gc : yellow_gc;
	  else
	    color = i == 0 || i == 6 ? salmon_gc : yellow_gc;
	}
      gdk_draw_rectangle (drawable, color, TRUE,
			  0, day->top, week_view->width,
			  week_view->have_extents
			  ? day->height : week_view->height);

      /* Leave a gap at the top.  */
      top += 2;

      /* Paint the day's banner.  */
      pango_layout_set_markup (pl, day->banner, -1);
      pango_layout_get_pixel_extents (pl, NULL, &pr);
      gtk_paint_layout (widget->style,
			widget->window,
			GTK_WIDGET_STATE (widget),
			FALSE,
			&event->area,
			widget,
			"label",
			left, top,
			pl);
      banner_height = pr.height + 2 /* Gap.  */;

      /* In wide mode, the events start to the right of the banners;
	 in narrow mode, immediately under.  */
      if (wide_mode)
	left += week_view->banner_width;
      else
        top += banner_height;

      /* Display each event.  */
      for (iter = day->events; iter; iter = iter->next)
	{
	  guint height = 0;
	  Event *ev = iter->data;
	      
	  /* Paint the time for events which have a time stamp.  */
	  if (! event_get_untimed (ev))
	    {
	      time_t t;
	      struct tm tm;

	      t = event_get_start (ev);
	      localtime_r (&t, &tm);

	      /* Also, only display the time if this is the day on
		 which the event starts.  */
	      if (tm.tm_year == day->popup.year
		  && tm.tm_mon == day->popup.month
		  && tm.tm_mday == day->popup.day)
		{
		  gchar *buffer = strftime_strdup_utf8_locale (TIMEFMT, &tm);
		  pango_layout_set_text (pl_evt, buffer, -1);
		  g_free (buffer);

		  gtk_paint_layout (widget->style,
				    widget->window,
				    GTK_WIDGET_STATE (widget),
				    FALSE,
				    &event->area,
				    widget,
				    "label",
				    left, top,
				    pl_evt);
		      
		  pango_layout_get_pixel_extents (pl_evt, NULL, &pr);
		  height = pr.height;
		}
	    }

	  /* Paint the event summary.  */
	  pango_layout_set_width (pl_evt, text_width * PANGO_SCALE);
	  pango_layout_set_text (pl_evt, event_get_summary (ev), -1);
	  pango_layout_get_pixel_extents (pl_evt, NULL, &pr);
	  gtk_paint_layout (widget->style,
			    widget->window,
			    GTK_WIDGET_STATE (widget),
			    FALSE,
			    &event->area,
			    widget,
			    "label",
			    left + week_view->time_width, top,
			    pl_evt);

	  if (height < pr.height)
	    height = pr.height;

	  /* Leave a gap of two pixels.  */
	  top += height + 2;
	}

      top = MAX (top, day->top + banner_height);
      /* For the frame.  */
      top ++;

      /* Save the calculated height.  */
      if (! week_view->have_extents)
	day->height = top - day->top + 1;
      else
	g_assert (day->height == top - day->top + 1);

      if (i == week_view->focused_day)
	/* Highlight today.  */
	gdk_draw_rectangle (drawable, blue_gc, FALSE,
			    0, day->top,
			    week_view->width - 1,  day->height - 2);

      gdk_draw_line (drawable, black_gc,
		     0, day->top + day->height - 1,
		     week_view->width - 1,  day->top + day->height - 1);

      top ++;
  }

  if (! week_view->have_extents)
    {
      int bottom = week_view->days[6].top + week_view->days[6].height;

      /* When painting the original backgrounds, we overestimated.
	 Clear any unused space.  */
      gdk_window_clear_area (widget->window, 0, bottom,
			     week_view->width, week_view->height - bottom);

      gtk_widget_set_size_request (week_view->draw,
				   week_view->time_width + 100, bottom);
    }
  week_view->have_extents = TRUE;

  gdk_gc_unref (blue_gc);
  gdk_gc_unref (yellow_gc);
  gdk_gc_unref (salmon_gc);

  g_object_unref (pl);
  g_object_unref (pl_evt);

  return TRUE;
}

/* The drawing area changed size.  */
static void
resize (GtkWidget *widget, GtkAllocation *allocation, GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);
  gint width = allocation->width;
  gint height = allocation->height;

  if (width != week_view->width || height != week_view->height)
    {
      week_view->have_extents = FALSE;

      week_view->width = width;
      week_view->height = height;

      gtk_widget_queue_draw (GTK_WIDGET (week_view->draw));
    }
}

static void
gtk_week_view_reload_events (GtkView *view)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (view);
  time_t now = gtk_view_get_time (view);
  struct tm tm;
  guint day;
  struct tm start;
  PangoLayout *pl
    = gtk_widget_create_pango_layout (GTK_WIDGET (week_view->draw), NULL);
  PangoLayout *pl_evt
    = gtk_widget_create_pango_layout (GTK_WIDGET (week_view->draw), NULL);

  localtime_r (&now, &tm);

  /* Determine if we need to update the cached information.  */
  int start_of_week = tm.tm_yday - tm.tm_wday;
  if (week_starts_monday)
    {
      if (tm.tm_wday == 0)
	start_of_week -= 6;
      else
	start_of_week ++;
    }

  /* Calculate the beginning of the week being careful with respect to
     DST.  */
  start = tm;
  if ((start.tm_yday - start_of_week) > start.tm_mday)
    /* The start of the week starts last month.  */
    {
      if (start.tm_mon == 0)
	/* Which also happens to be last year.  */
	{
	  start.tm_year --;
	  start.tm_mon = 11;
	}
      else
	start.tm_mon --;

      int days = days_in_month (start.tm_year, start.tm_mon);
      start.tm_mday = days + tm.tm_mday;
    }
  start.tm_mday -= (start.tm_yday - start_of_week);

  /* Load the events for each day and figure out the focused day.  */
  week_view->focused_day = -1;
  time_t t = mktime (&start);
  for (day = 0; day < 7; day++)
    {
      struct week_day *d = &week_view->days[day];
      struct tm start, end;
      time_t s, e;

      if (d->events)
        event_list_unref (d->events);

      localtime_r (&t, &start);
      start.tm_hour = 0;
      start.tm_min = 0;
      start.tm_sec = 0;
      s = mktime (&start);

      end = start;
      end.tm_hour = 23;
      end.tm_min = 59;
      end.tm_sec = 59;
      e = mktime (&end);

      /* Tomorrow.  */
      t = e + 1;

      d->events = g_slist_sort (event_db_list_for_period (event_db, s, e),
				event_compare_func);

      if (start.tm_mday == tm.tm_mday
	  && start.tm_mon == tm.tm_mon
	  && start.tm_year == tm.tm_year)
	week_view->focused_day = day;

      if (d->banner)
        g_free (d->banner);
      d->banner = strftime_strdup_utf8_utf8 ("<b>%a %d %B</b> ", &start);

      d->popup.day = start.tm_mday;
      d->popup.year = start.tm_year;
      d->popup.month = start.tm_mon;
      d->popup.events = week_view->days[day].events;
    }
  g_assert (0 <= week_view->focused_day && week_view->focused_day < 7);

  week_view->have_extents = FALSE;

  gtk_widget_queue_draw (week_view->draw);

  g_object_unref (pl);
  g_object_unref (pl_evt);
}

static gboolean
week_view_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);
  struct week_day *d = &week_view->days[week_view->focused_day];

  int i = 0;
  switch (k->keyval)
    {
    case GDK_Escape:
      if (pop_window) 
	gtk_widget_destroy (pop_window);
      pop_window = NULL;
      week_view->has_popup = NULL;
      return TRUE;

    case GDK_Left:
      i = -7;
      break;

    case GDK_Right:
      i = 7;
      break;

    case GDK_Down:
      i = 1;
      break;

    case GDK_Up:
      i = -1;
      break;

    case GDK_space:
      if (pop_window) 
	gtk_widget_destroy (pop_window);
      if (d != week_view->has_popup) 
	{
	  pop_window = day_popup (main_window, &d->popup, FALSE);
	  week_view->has_popup = d;
	}
      else 
	{
	  pop_window = NULL;
	  week_view->has_popup = NULL;
	}
      return TRUE;

    case GDK_Return:
      /* Zoom to the day view.  */
      {
	time_t t = gtk_view_get_time (GTK_VIEW (week_view));
	struct tm tm;

	if (pop_window) 
	  gtk_widget_destroy (pop_window);
	pop_window = NULL;
	week_view->has_popup = NULL;

	localtime_r (&t, &tm);
	tm.tm_year = d->popup.year;//- 1900;
	tm.tm_mon = d->popup.month;
	tm.tm_mday = d->popup.day;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	set_time_and_day_view (mktime (&tm));    

	return TRUE; 
      }
    }

  if (i)
    {
      gtk_view_set_time (GTK_VIEW (week_view),
			 gtk_view_get_time (GTK_VIEW (week_view))
			 + i * 24 * 60 * 60);
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
week_view_button_press (GtkWidget *widget, GdkEventButton *event,
			GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);
  guint y = event->y;
  int day;
  struct week_day *d;

  /* Can't have pressed a button without having drawn the view at
     least once.  */
  g_assert (week_view->have_extents);

  /* Which day was clicked?  */
  for (day = 0; day < 7; day ++)
    if ((week_view->days[day].top <= y)
	&& (week_view->days[day].top + week_view->days[day].height >= y))
      break;
  if (day == 7)
    /* Nothing valid... */
    return FALSE;

  d = &week_view->days[day];
  if (event->type == GDK_BUTTON_PRESS)
    {
      if (event->button == 1)
	{
	  if (week_view->focused_day == day)
	    /* This day is focused, zoom to the day view.  */
	    {
	      time_t t = gtk_view_get_time (GTK_VIEW (week_view));
	      struct tm tm;

	      if (pop_window) 
		gtk_widget_destroy (pop_window);
	      pop_window = NULL;
	      week_view->has_popup = NULL;

	      localtime_r (&t, &tm);
	      tm.tm_year = d->popup.year;
	      tm.tm_mon = d->popup.month;
	      tm.tm_mday = d->popup.day;
	      set_time_and_day_view (mktime (&tm));
	    }
	  else
	    {
	      gtk_view_set_time (GTK_VIEW (week_view),
				 time_from_day (d->popup.year, d->popup.month,
						d->popup.day));

	      /* In case the draw doesn't have the focus.  */
	      gtk_widget_grab_focus (week_view->draw);
	    }
	}
      else if (event->button == 3)
	{
	  if (pop_window) 
	    gtk_widget_destroy (pop_window);
    
	  if (d != week_view->has_popup) 
	    {
	      pop_window = day_popup (gtk_widget_get_toplevel (main_window),
				      &d->popup, FALSE);
	      week_view->has_popup = d;
	    }
	  else 
	    {
	      pop_window = NULL;
	      week_view->has_popup = NULL;
	    }
	}
    }
  
  return TRUE;
}

GtkWidget *
gtk_week_view_new (time_t time)
{
  GtkWeekView *week_view;

  week_view = GTK_WEEK_VIEW (g_object_new (gtk_week_view_get_type (), NULL));

  /* Create the scroller.  */
  week_view->scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (week_view), week_view->scroller, TRUE, TRUE, 0);
  gtk_widget_show (week_view->scroller);

  /* Create the draw area.  */
  week_view->draw = gtk_drawing_area_new ();
  g_signal_connect (G_OBJECT (week_view->draw), "expose_event",
                    G_CALLBACK (draw_expose_event), week_view);
  g_signal_connect (G_OBJECT (week_view->draw), "size-allocate", 
		    G_CALLBACK (resize), week_view);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (week_view->scroller),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (week_view->scroller), week_view->draw);
  g_signal_connect (G_OBJECT (week_view->draw), "button-press-event",
                    G_CALLBACK (week_view_button_press), week_view);
  gtk_widget_add_events (GTK_WIDGET (week_view->draw),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (week_view->draw), "key_press_event", 
		    G_CALLBACK (week_view_key_press_event), week_view);
  GTK_WIDGET_SET_FLAGS (week_view->draw, GTK_CAN_FOCUS);
  gtk_widget_show (week_view->draw);

  gtk_view_set_time (GTK_VIEW (week_view), time);

  return GTK_WIDGET (week_view);
}
