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

#include "globals.h"
#include "week_view.h"
#include "day_popup.h"

#include "gtkdatesel.h"

#define _(x) gettext(x)

#ifdef IS_HILDON
#define REALLY_MIN_CELL_HEIGHT 47
#else
#define REALLY_MIN_CELL_HEIGHT 38
#endif

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
  GtkVBox widget;

  struct week_day days[7];
  gboolean have_extents;

  GtkWidget *draw;
  GtkWidget *scroller;
  GtkWidget *datesel, *calendar;

  /* The week day (if any) which the popup menu is for.  */
  struct week_day *has_popup;

  /* With required by the banners.  */
  gint banner_width;
  /* Width required by the time fields.  */
  gint time_width;
  /* Current canvas width and height.  */
  gint width, height;

  /* Day with the focus.  */
  int focused_day;

  time_t date;
};

typedef struct
{
  GtkVBoxClass vbox_class;
  GObjectClass parent_class;
} GtkWeekViewClass;

static void gtk_week_view_base_class_init (gpointer klass);
static void gtk_week_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_week_view_dispose (GObject *obj);
static void gtk_week_view_finalize (GObject *object);
static void gtk_week_view_show (GtkWidget *object);
static void week_view_update (GtkWeekView *week_view, gboolean force);

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

      type = g_type_register_static (gtk_vbox_get_type (),
				     "GtkWeekView", &info, 0);
    }

  return type;
}

static void
gtk_week_view_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_vbox_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_week_view_finalize;
  object_class->dispose = gtk_week_view_dispose;

  widget_class = (GtkWidgetClass *) klass;
  widget_class->show = gtk_week_view_show;
}

static void
gtk_week_view_init (GTypeInstance *instance, gpointer klass)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (instance);

  week_view->have_extents = FALSE;
  week_view->height = 0;
  week_view->width = 0;
  week_view->date = (time_t) -1;
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
          event_db_list_destroy (d->events);
          d->events = NULL;
        }
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_week_view_show (GtkWidget *widget)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (widget);

  week_view_update (week_view, TRUE);

  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
gtk_week_view_invalidate_day (GtkWeekView *week_view, gint i)
{
  struct week_day *day = &week_view->days[i];

  gtk_widget_queue_draw_area (week_view->draw, 0, day->top,
			      week_view->width, day->height);
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
	  event_t ev = iter->data;
	  event_details_t evd = event_db_get_details (ev);
	      
	  /* Paint the time for events which have a time stamp.  */
	  if ((ev->flags & FLAG_UNTIMED) == 0)
	    {
	      struct tm tm;

	      localtime_r (&ev->start, &tm);

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
	  pango_layout_set_text (pl_evt, evd->summary, -1);
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
week_view_update (GtkWeekView *week_view, gboolean force)
{
  struct tm current;
  struct tm vt;
  guint day;
  struct tm start;
  PangoLayout *pl
    = gtk_widget_create_pango_layout (GTK_WIDGET (week_view->draw), NULL);
  PangoLayout *pl_evt
    = gtk_widget_create_pango_layout (GTK_WIDGET (week_view->draw), NULL);
  GSList *iter;

  localtime_r (&week_view->date, &current);
  week_view->date = viewtime;
  localtime_r (&week_view->date, &vt);

  /* Determine if the calendar needs an update.  */
  {
    unsigned int day, month, year;
    gtk_calendar_get_date (GTK_CALENDAR (week_view->calendar),
			   &year, &month, &day);

    if (year - 1900 != vt.tm_year || month != vt.tm_mon || day != vt.tm_mday)
      {
	gtk_calendar_select_month (GTK_CALENDAR (week_view->calendar),
				   vt.tm_mon, vt.tm_year + 1900);
	gtk_calendar_select_day (GTK_CALENDAR (week_view->calendar),
				 vt.tm_mday);
      }
  }

  /* Determine if the datesel needs an update.  */
  {
    time_t ds = gtk_date_sel_get_time (GTK_DATE_SEL (week_view->datesel));
    struct tm dst;
    localtime_r (&ds, &dst);

    if (dst.tm_year != vt.tm_year || dst.tm_yday != vt.tm_yday)
      gtk_date_sel_set_time (GTK_DATE_SEL (week_view->datesel), viewtime);
  }

  /* Determine if we need to update the cached information.  */
  int start_of_week = vt.tm_yday - vt.tm_wday;
  if (week_starts_monday)
    {
      if (vt.tm_wday == 0)
	start_of_week -= 6;
      else
	start_of_week ++;
    }

  if (! force
      && vt.tm_year == current.tm_year
      && start_of_week <= current.tm_yday
      && current.tm_yday < start_of_week + 7)
    /* Same week.  */
    {
      if (vt.tm_yday - start_of_week != week_view->focused_day)
	/* Different day: change the focus.  */
	{
	  gtk_week_view_invalidate_day (week_view, week_view->focused_day);
	  week_view->focused_day = vt.tm_yday - start_of_week;
	  gtk_week_view_invalidate_day (week_view, week_view->focused_day);
	}
      return;
    }

  /* Calculate the beginning of the week being careful with respect to
     DST.  */
  start = vt;
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
      start.tm_mday = days + vt.tm_mday;
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
        event_db_list_destroy (d->events);

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

      d->events = event_db_untimed_list_for_period (s, e, TRUE);
      d->events = g_slist_concat (d->events, 
				  event_db_untimed_list_for_period (s, e, 
								    FALSE));

      if (start.tm_mday == vt.tm_mday
	  && start.tm_mon == vt.tm_mon
	  && start.tm_year == vt.tm_year)
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

  /* Calculate WEEKVIEW->BANNER_WIDTH, the minimum width required to
     display each day's banner.  */
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
  week_view->time_width = 0;
  for (day = 0; day < 7; day++)
    for (iter = week_view->days[day].events; iter; iter = iter->next)
      {
	event_t ev = iter->data;
	if ((ev->flags & FLAG_UNTIMED) != 0)
	  /* Reminder, i.e. no time.  */
	  continue;

	struct tm tm;
	localtime_r (&ev->start, &tm);

	if (! (tm.tm_year == week_view->days[day].popup.year
	       && tm.tm_mon == week_view->days[day].popup.month
	       && tm.tm_mday == week_view->days[day].popup.day))
	  /* Doesn't start today.  As such, we don't show the
	     date.  */
	  continue;

	PangoRectangle pr;

	localtime_r (&ev->start, &tm);
	gchar *buffer = strftime_strdup_utf8_locale (TIMEFMT, &tm);
	pango_layout_set_text (pl_evt, buffer, -1);
	g_free (buffer);
	pango_layout_get_pixel_extents (pl_evt, NULL, &pr);

	week_view->time_width = MAX (pr.width, week_view->time_width);
      }
  if (week_view->time_width > 0)
    /* Add a small gap.  */
    week_view->time_width += 5;

  week_view->have_extents = FALSE;

  gtk_widget_queue_draw (week_view->draw);

  g_object_unref (pl);
  g_object_unref (pl_evt);
}

static void
calendar_day_changed (GtkWidget *widget, GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);
  unsigned int day, month, year;
  struct tm tm;

  gtk_calendar_get_date (GTK_CALENDAR (widget), &year, &month, &day);
  localtime_r (&viewtime, &tm);

  tm.tm_year = year - 1900;
  tm.tm_mon = month;
  tm.tm_mday = day;
  viewtime = mktime (&tm);

  week_view_update (week_view, FALSE);
}

static void
datesel_changed (GtkWidget *widget, GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);

  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  week_view_update (week_view, FALSE);
}

static void
update_hook_callback (GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);

  /* For a reload of the events.  */
  week_view_update (week_view, TRUE);
}

static gboolean
week_view_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *wv)
{
  GtkWeekView *week_view = GTK_WEEK_VIEW (wv);
  struct week_day *d = &week_view->days[week_view->focused_day];

  switch (k->keyval)
    {
    case GDK_Escape:
      if (pop_window) 
	gtk_widget_destroy (pop_window);
      pop_window = NULL;
      week_view->has_popup = NULL;
      return TRUE;

    case GDK_Left:
      gtk_date_sel_move_week (GTK_DATE_SEL (week_view->datesel), -1);
      return TRUE;

    case GDK_Right:
      gtk_date_sel_move_week (GTK_DATE_SEL (week_view->datesel), 1);
      return TRUE;

    case GDK_Down:
      gtk_date_sel_move_day (GTK_DATE_SEL (week_view->datesel), 1);
      return TRUE;

    case GDK_Up:
      gtk_date_sel_move_day (GTK_DATE_SEL (week_view->datesel), -1);
      return TRUE;

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
	struct tm tm;

	if (pop_window) 
	  gtk_widget_destroy (pop_window);
	pop_window = NULL;
	week_view->has_popup = NULL;

	localtime_r (&viewtime, &tm);
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
	      struct tm tm;

	      if (pop_window) 
		gtk_widget_destroy (pop_window);
	      pop_window = NULL;
	      week_view->has_popup = NULL;

	      localtime_r (&viewtime, &tm);
	      tm.tm_year = d->popup.year;
	      tm.tm_mon = d->popup.month;
	      tm.tm_mday = d->popup.day;
	      set_time_and_day_view (mktime (&tm));
	    }
	  else
	    {
	      /* Focus the day.  */
	      viewtime = time_from_day (d->popup.year, d->popup.month,
					d->popup.day);
	      gtk_calendar_select_month (GTK_CALENDAR (week_view->calendar),
					 d->popup.month, d->popup.year + 1900);
	      gtk_calendar_select_day (GTK_CALENDAR (week_view->calendar),
				       d->popup.day);
	      week_view_update (week_view, FALSE);

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
gtk_week_view_new (void)
{
  GtkWeekView *week_view;
  gboolean landscape;
  GtkWidget *hbox;

  week_view = GTK_WEEK_VIEW (g_object_new (gtk_week_view_get_type (), NULL));
  week_view->scroller = gtk_scrolled_window_new (NULL, NULL);
  week_view->draw = gtk_drawing_area_new ();

  g_signal_connect (G_OBJECT (week_view->draw), "size-allocate", 
		    G_CALLBACK (resize), week_view);

  landscape = (gdk_screen_width () > gdk_screen_height ()
	       && gdk_screen_width () >= 640) ? TRUE : FALSE;
  hbox = gtk_hbox_new (FALSE, 0);

  week_view->calendar = gtk_calendar_new ();
  GTK_WIDGET_UNSET_FLAGS (week_view->calendar, GTK_CAN_FOCUS);  

  gtk_calendar_set_display_options (GTK_CALENDAR (week_view->calendar), 
				    GTK_CALENDAR_SHOW_DAY_NAMES 
				    | (week_starts_monday
				       ? GTK_CALENDAR_WEEK_START_MONDAY : 0));
    
  week_view->datesel = gtk_date_sel_new (GTKDATESEL_WEEK, viewtime);

  gtk_widget_show (week_view->draw);
  gtk_widget_show (week_view->scroller);
  gtk_widget_show (week_view->datesel);
  gtk_widget_show (hbox);

  g_signal_connect (G_OBJECT (week_view->draw), "expose_event",
                    G_CALLBACK (draw_expose_event), week_view);

  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (week_view->scroller), week_view->draw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (week_view->scroller),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (week_view), week_view->datesel,
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (week_view), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), week_view->scroller, TRUE, TRUE, 0);

  if (landscape)
    {
      GtkWidget *sep;
#ifdef IS_HILDON
      gtk_widget_set_size_request(week_view->calendar, 240, -1);
#else
      sep = gtk_vseparator_new ();
      gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, TRUE, 0);
      gtk_widget_show (sep);
#endif
      gtk_box_pack_start (GTK_BOX (hbox), week_view->calendar, FALSE, TRUE, 0);
      gtk_widget_show (week_view->calendar);
    }

  g_signal_connect (G_OBJECT (week_view->calendar), 
		    "day-selected",
		    G_CALLBACK (calendar_day_changed), week_view);

  g_signal_connect (G_OBJECT (week_view->datesel), "changed",
                    G_CALLBACK (datesel_changed), week_view);

  g_object_set_data (G_OBJECT (week_view), "update_hook",
                     (gpointer) update_hook_callback);

  GTK_WIDGET_SET_FLAGS (week_view->datesel, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (week_view->datesel);
  g_signal_connect (G_OBJECT (week_view->datesel), "key_press_event", 
		    G_CALLBACK (week_view_key_press_event), week_view);
  g_signal_connect (G_OBJECT (week_view->draw), "button-press-event",
                    G_CALLBACK (week_view_button_press), week_view);

  gtk_widget_add_events (GTK_WIDGET (week_view->draw),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_add_events (GTK_WIDGET (week_view->datesel),
			 GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  
  return GTK_WIDGET (week_view);
}
