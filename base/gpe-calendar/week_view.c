/*
 * Copyright (C) 2001, 2002, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/event-db.h>

#include "pannedwindow.h"
#include "view.h"
#include "globals.h"
#include "week_view.h"
#include "day_popup.h"

struct week_day
{
  /* Valid if HAVE_EXTENTS is true.  */
  int top;
  int height;

  /* Banner to display for this day.  */
  gchar *banner;

  /* The list of events.  */
  GSList *events;

  GDate date;
};

struct _GtkWeekView
{
  GtkView widget;

  struct week_day days[7];
  gboolean have_extents;

  GtkWidget *draw;
  PannedWindow *panned_window;

  /* Width required by the banners, valid if HAVE_EXTENTS is true.  */
  gint banner_width;
  /* Width required by the time fields, valid if HAVE_EXTENTS is true.  */
  gint time_width;

  /* Current canvas width and height.  */
  gint width, height;

  /* Day with the focus.  */
  int focused_day;

  /* If an event reload is pending.  */
  gboolean pending_reload;

  /* Called at midnight each day.  */
  guint day_changed;
};

typedef struct
{
  GtkViewClass view_class;
} GtkWeekViewClass;

static void gtk_week_view_base_class_init (gpointer klass,
					   gpointer klass_data);
static void gtk_week_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_week_view_dispose (GObject *obj);
static void gtk_week_view_finalize (GObject *object);
static void map (GtkWidget *widget);
static void unmap (GtkWidget *widget);
static gboolean gtk_week_view_key_press_event (GtkWidget *widget,
					       GdkEventKey *k);
static void gtk_week_view_set_time (GtkView *view, time_t time);
static void gtk_week_view_reload_events (GtkView *view);

static GtkWidgetClass *parent_class;

GType
gtk_week_view_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (GtkWeekViewClass),
	NULL,
	NULL,
	gtk_week_view_base_class_init,
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
gtk_week_view_base_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_view_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_week_view_finalize;
  object_class->dispose = gtk_week_view_dispose;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->map = map;
  widget_class->unmap = unmap;
  widget_class->key_press_event = gtk_week_view_key_press_event;

  view_class = (GtkViewClass *) klass;
  view_class->set_time = gtk_week_view_set_time;
  view_class->reload_events = gtk_week_view_reload_events;
}

static void
gtk_week_view_init (GTypeInstance *instance, gpointer klass)
{
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

static gboolean day_changed (GtkWeekView *week_view);

static void
setup_day_changed (GtkWeekView *week_view)
{
  if (week_view->day_changed)
    g_source_remove (week_view->day_changed);

  time_t now = time (NULL);

  GDate tomorrow;
  g_date_set_time_t (&tomorrow, now);
  g_date_add_days (&tomorrow, 1);
  struct tm tm;
  g_date_to_struct_tm (&tomorrow, &tm);

  week_view->day_changed = g_timeout_add ((mktime (&tm) - now + 1) * 1000,
					  (GSourceFunc) day_changed,
					  week_view);
}

static gboolean
day_changed (GtkWeekView *week_view)
{
  setup_day_changed (week_view);

  gdk_window_invalidate_rect (GTK_WIDGET (week_view->draw)->window,
			      NULL, FALSE);

  return FALSE;
}

static void
map (GtkWidget *widget)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  setup_day_changed (week_view);
}

static void
unmap (GtkWidget *widget)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (widget);

  if (week_view->day_changed)
    {
      g_source_remove (week_view->day_changed);
      week_view->day_changed = 0;
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
scroll_to_focused_day (GtkWeekView *week_view)
{
  if (! GTK_WIDGET_REALIZED (week_view))
    return;

  GtkScrolledWindow *scrolled_window
    = GTK_SCROLLED_WINDOW (GTK_BIN (week_view->panned_window)->child);
  GtkAdjustment *adj
    = gtk_scrolled_window_get_vadjustment (scrolled_window);
  if (! adj)
    return;

  GtkWidget *viewport = GTK_BIN (scrolled_window)->child;

  int top = week_view->days[week_view->focused_day].top;
  int height = week_view->days[week_view->focused_day].height;

  int v_top = week_view->draw->allocation.height
    * adj->value / (adj->upper - adj->lower);
  int v_height = viewport->allocation.height;

  if (top < v_top + v_height / 8)
    v_top = top - v_height / 8;
  else if (top + height > v_top + 7 * v_height / 8)
    v_top = top + height - v_height + v_height / 8;
  else
    return;

  gtk_adjustment_set_value
    (adj, CLAMP ((gdouble) v_top / week_view->draw->allocation.height
		 * (adj->upper - adj->lower) + adj->lower,
		 adj->lower, adj->upper - adj->page_size));
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
  if (! week_starts_sunday)
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

	  if (! panned_window_is_panning (week_view->panned_window))
	    scroll_to_focused_day (week_view);
	}
      return;
    }

  gtk_week_view_reload_events (view);
}

static void reload_events_hard (GtkWeekView *week_view);

static gint
draw_expose_event (GtkWidget *widget, GdkEventExpose *event, GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);
  if (week_view->pending_reload)
    reload_events_hard (week_view);

  GdkDrawable *drawable = widget->window;
  GdkGC *black_gc;
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

  GdkGC *blue_gc = pen_new (widget, 0, 0, 0xffff);
  /* Today, "lemon chiffon".  */
  GdkGC *lemon_gc = pen_new (widget, 255 << 8, 250 << 8, 205 << 8);

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

	    GDate date;
	    g_date_set_time (&date, event_get_start (ev));

	    if (g_date_compare (&date, &week_view->days[day].date) != 0)
	      /* Doesn't start today.  As such, we don't show the
		 date.  */
	      continue;

	    PangoRectangle pr;

	    char buffer[100];
	    g_date_strftime (buffer, sizeof (buffer), TIMEFMT, &date);
	    pango_layout_set_text (pl_evt, buffer, -1);
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

  GDate today;
  g_date_set_time_t (&today, time (NULL));

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
      if (g_date_compare (&today, &week_view->days[i].date) == 0)
	color = lemon_gc;
      else if (week_starts_sunday)
	color = i == 0 || i == 6 ? salmon_gc : yellow_gc;
      else
	color = i >= 5 ? salmon_gc : yellow_gc;
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

	  if (! event_get_visible (ev, NULL))
	    continue;

	  /* Paint the time for events which have a time stamp.  */
	  if (! event_get_untimed (ev))
	    {
	      time_t t = event_get_start (ev);
	      struct tm tm;
	      localtime_r (&t, &tm);

	      GDate date;
	      g_date_set_dmy (&date, tm.tm_mday, tm.tm_mon + 1,
			      tm.tm_year + 1900);
	      
	      /* Also, only display the time if this is the day on
		 which the event starts.  */
	      if (g_date_compare (&date, &day->date) == 0)
		{
		  char *buffer = strftime_strdup_utf8_locale (TIMEFMT, &tm);
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
	  char *s = event_get_summary (ev, NULL);
	  pango_layout_set_text (pl_evt, s ?: "", -1);
	  g_free (s);
	  pango_layout_get_pixel_extents (pl_evt, NULL, &pr);

	  GdkColor color;
	  if (event_get_color (ev, &color, NULL))
	    {
	      GdkGC *color_gc;

	      color_gc = gdk_gc_new (widget->window);
	      gdk_gc_copy (color_gc, widget->style->black_gc);

	      gdk_colormap_alloc_color (colormap, &color, FALSE, TRUE);
	      gdk_gc_set_foreground (color_gc, &color);

	      gdk_draw_rectangle (drawable, color_gc, TRUE,
				  left + week_view->time_width, top,
				  pr.width + 6, pr.height);
	      gdk_draw_rectangle (drawable, black_gc, FALSE,
				  left + week_view->time_width, top,
				  pr.width + 6, pr.height);

	      g_object_unref (color_gc);
	    }

	  gtk_paint_layout (widget->style,
			    widget->window,
			    GTK_WIDGET_STATE (widget),
			    FALSE,
			    &event->area,
			    widget,
			    "label",
			    left + week_view->time_width + 3, top,
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
  GTK_WEEK_VIEW (view)->pending_reload = TRUE;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
reload_events_hard (GtkWeekView *week_view)
{
  /* Destroy any events.  */
  int i;
  for (i = 0; i < 7; i ++)
    {
      struct week_day *d = &week_view->days[i];
      if (d->events)
	{
	  event_list_unref (d->events);
	  d->events = NULL;
	}
    }

  GDate focus;
  g_date_set_time_t (&focus, gtk_view_get_time (GTK_VIEW (week_view)));

  GDate period_start = focus;
  /* Normalize to sunday or monday as appropriate.  */
  g_date_subtract_days (&period_start,
			(week_starts_sunday ?
			    7 - (G_DATE_SUNDAY - g_date_get_weekday (&focus)) :
			    (g_date_get_weekday (&focus) - G_DATE_MONDAY)) %7 );

  GDate period_end = period_start;
  g_date_add_days (&period_end, 7);

  /* The julian start day.  */
  guint period_start_day = g_date_get_julian (&period_start);

  /* The focused day.  */
  week_view->focused_day = g_date_get_julian (&focus) - period_start_day;
  g_assert (0 <= week_view->focused_day && week_view->focused_day < 7);

  for (i = 0; i < 7; i ++)
    {
      struct week_day *d = &week_view->days[i];

      if (d->banner)
        g_free (d->banner);
      g_date_set_julian (&d->date, period_start_day + i);

      struct tm tm;
      g_date_to_struct_tm (&d->date, &tm);
      d->banner = strftime_strdup_utf8_utf8 ("<b>%a %-d %b</b> ", &tm);
    }

  struct tm period_start_tm;
  g_date_to_struct_tm (&period_start, &period_start_tm);
  period_start_tm.tm_isdst = -1;

  struct tm period_end_tm;
  g_date_to_struct_tm (&period_end, &period_end_tm);
  period_end_tm.tm_isdst = -1;

  /* Load the events for each day.  */
  GSList *events
    = event_db_list_for_period (event_db, mktime (&period_start_tm),
				mktime (&period_end_tm) - 1, NULL);
  GSList *l;
  /* We need to remove one day as the code below requires an inclusive
     period.  */
  g_date_subtract_days (&period_end, 1);
  for (l = events; l; l = l->next)
    {
      Event *ev = EVENT (l->data);

      time_t s = event_get_start (ev);
      GDate start;
      g_date_set_time_t (&start, s);
      g_date_clamp (&start, &period_start, &period_end);

      time_t e = s + event_get_duration (ev) - 1;
      GDate end;
      g_date_set_time_t (&end, e);
      g_date_clamp (&end, &period_start, &period_end);

      for (i = g_date_get_julian (&start); i <= g_date_get_julian (&end); i ++)
	{
	  g_object_ref (ev);
	  week_view->days[i - period_start_day].events
	    = g_slist_prepend (week_view->days[i - period_start_day].events,
			       ev);
	}

    }
  event_list_unref (events);

  /* Sort the lists.  */
  for (i = 0; i < 7; i ++)
    if (week_view->days[i].events)
      week_view->days[i].events
	= g_slist_sort (week_view->days[i].events, event_compare_func);

  week_view->have_extents = FALSE;

  week_view->pending_reload = FALSE;

  gtk_widget_queue_draw (week_view->draw);
}

static gboolean
gtk_week_view_key_press_event (GtkWidget *widget, GdkEventKey *k)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (widget);
  struct week_day *d = &week_view->days[week_view->focused_day];

  int i = 0;
  switch (k->keyval)
    {
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
      gtk_menu_popup (day_popup (&d->date, d->events),
		      NULL, NULL, NULL, NULL,
		      0, gtk_get_current_event_time());
      return TRUE;

    case GDK_Return:
      /* Zoom to the day view.  */
      {
	struct tm tm;
	g_date_to_struct_tm (&d->date, &tm);
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
button_press_event (GtkWidget *widget, GdkEventButton *event,
		    GtkWeekView *week_view)
{
  /* Can't have pressed a button without having drawn the view at
     least once.  */
  g_assert (week_view->have_extents);

  gtk_widget_grab_focus (GTK_WIDGET (week_view));

  if (event->button == 3)
    {
      guint y = event->y;
      int day;

      /* Which day was clicked?  */
      for (day = 0; day < 7; day ++)
	if ((week_view->days[day].top <= y)
	    && (week_view->days[day].top + week_view->days[day].height >= y))
	  break;
      if (day == 7)
	/* Nothing valid... */
	return FALSE;

      struct week_day *d = &week_view->days[day];
      gtk_menu_popup (day_popup (&d->date, d->events),
		      NULL, NULL, NULL, NULL,
		      event->button, event->time);

      return TRUE;
    }
  
  return FALSE;
}

static gboolean
button_release_event (GtkWidget *widget, GdkEventButton *event,
		      GtkWeekView *week_view)
{
  gtk_widget_grab_focus (GTK_WIDGET (week_view));

  /* Can't have pressed a button without having drawn the view at
     least once.  */
  g_assert (week_view->have_extents);

  if (event->button == 1
      && ! panned_window_is_panning (week_view->panned_window))
    {
      guint y = event->y;
      int day;

      /* Which day was clicked?  */
      for (day = 0; day < 7; day ++)
	if ((week_view->days[day].top <= y)
	    && (week_view->days[day].top + week_view->days[day].height >= y))
	  break;
      if (day == 7)
	/* Nothing valid... */
	return FALSE;

      if (week_view->focused_day == day)
	/* This day is focused, zoom to the day view.  */
	{
	  struct tm tm;
	  g_date_to_struct_tm (&week_view->days[day].date, &tm);

	  set_time_and_day_view (mktime (&tm));
	}
      else
	gtk_view_set_date (GTK_VIEW (week_view),
			   &week_view->days[day].date);

      return TRUE;
    }
  
  return FALSE;
}

GtkWidget *
gtk_week_view_new (time_t time)
{
  GtkWeekView *week_view;

  week_view = GTK_WEEK_VIEW (g_object_new (gtk_week_view_get_type (), NULL));
  gtk_widget_add_events (GTK_WIDGET (week_view), GDK_KEY_PRESS_MASK);
  GTK_WIDGET_SET_FLAGS (week_view, GTK_CAN_FOCUS);

  /* Create the panned window.  */
  week_view->panned_window = PANNED_WINDOW (panned_window_new ());
  gtk_box_pack_start (GTK_BOX (week_view),
		      GTK_WIDGET (week_view->panned_window),
		      TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (week_view->panned_window));

  GtkWidget *scrolled_window = GTK_BIN (week_view->panned_window)->child;
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  /* Create the draw area.  */
  week_view->draw = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (week_view->draw),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (week_view->draw), "expose_event",
                    G_CALLBACK (draw_expose_event), week_view);
  g_signal_connect (G_OBJECT (week_view->draw), "size-allocate", 
		    G_CALLBACK (resize), week_view);
  g_signal_connect (G_OBJECT (week_view->draw), "button-press-event",
		    G_CALLBACK (button_press_event), week_view);
  g_signal_connect (G_OBJECT (week_view->draw), "button-release-event",
		    G_CALLBACK (button_release_event), week_view);
  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (scrolled_window), week_view->draw);
  gtk_widget_show (week_view->draw);

  gtk_view_set_time (GTK_VIEW (week_view), time);

  return GTK_WIDGET (week_view);
}
