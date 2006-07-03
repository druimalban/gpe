/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <langinfo.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#include <gpe/event-db.h>
#include "view.h"
#include "globals.h"
#include "month_view.h"
#include "day_popup.h"

#define MAX_DAYS (6 * 7)

/* Describes each cell: the date it represents and the events occuring
   that day.  */
struct day
{
  GDate date;
  GSList *events;
};

/* A month view consists of a number of cells: 7 per row up to 6
   columns (as that is the maximum number of weeks any week requires
   independent of the day it starts on.

   Each cell has a render control associated with it.  This render
   control is marked as valid and has the date the cell represents and
   a pointer to the event list (if any).  The event list can also be
   found in DAY_EVENTS.  DAY_EVENTS is indexed by the cells day in the
   month, NOT BY THE CELL number.  */
struct _GtkMonthView
{
  GtkView widget;

  GtkScrolledWindow *scrolled_window;
  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  GtkWidget *draw;
  guint32 last_expose;

  /* Events attached to the day.  */
  struct day day[MAX_DAYS];

  /* Day which has the focus.  */
  guint focused_day;

  /* The current zoom factor.  */
  int zoom_factor;
  /* Amount of canvas allocated.  */
  gint width, height;
  /* Amount of canvas the title bar requries.  */
  gint title_height;

  /* Approximate char width and height given the current font.  */
  int char_width;
  int char_height;

  /* Number of weeks in the current month.  */
  gint weeks;

  /* If a event reload is pending.  */
  gboolean pending_reload;
  gboolean pending_update_extents;

  struct
  {
    gboolean dragging;

    /* Where the drag started (relative to the root).  */
    gdouble x_origin;
    gdouble y_origin;
    /* The location of the last "drag-motion" signal position
       (relative to the root).  */
    gdouble x;
    gdouble y;

    /* The time of the last motion.  */
    guint32 last_motion;
    gdouble page_timer;

    /* The time of the last month flip.  */
    guint32 last_flip;
  } drag;
};

static const int zoom_factors[] = { 4, 7, 12 };
#define ZOOM_FACTORS (sizeof (zoom_factors) / sizeof (zoom_factors[0]))

typedef struct
{
  GtkViewClass view_class;
} GtkMonthViewClass;

static void gtk_month_view_base_class_init (gpointer klass,
					    gpointer klass_data);
static void gtk_month_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_month_view_dispose (GObject *obj);
static void gtk_month_view_finalize (GObject *object);
static void gtk_month_view_realize (GtkWidget *);
static gboolean gtk_month_view_key_press_event (GtkWidget *widget,
						GdkEventKey *k);
static void gtk_month_view_set_time (GtkView *view, time_t time);
static void gtk_month_view_reload_events (GtkView *view);

static GtkWidgetClass *parent_class;

GType
gtk_month_view_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
	{
	  sizeof (GtkMonthViewClass),
	  NULL,
	  NULL,
	  gtk_month_view_base_class_init,
	  NULL,
	  NULL,
	  sizeof (struct _GtkMonthView),
	  0,
	  gtk_month_view_init
	};

      type = g_type_register_static (gtk_view_get_type (),
				     "GtkMonthView", &info, 0);
    }

  return type;
}

static void
gtk_month_view_base_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_view_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = gtk_month_view_dispose;
  object_class->finalize = gtk_month_view_finalize;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->realize = gtk_month_view_realize;
  widget_class->key_press_event = gtk_month_view_key_press_event;

  view_class = (GtkViewClass *) klass;
  view_class->set_time = gtk_month_view_set_time;
  view_class->reload_events = gtk_month_view_reload_events;
}

static void
gtk_month_view_init (GTypeInstance *instance, gpointer klass)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (instance);

  month_view->pending_reload = TRUE;
}

static void
gtk_month_view_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_month_view_finalize (GObject *object)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (object);
  gint i;

  for (i = 0; i < MAX_DAYS; i++)
    if (month_view->day[i].events) 
      event_list_unref (month_view->day[i].events);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_month_view_realize (GtkWidget *widget)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (widget);

  PangoLayout *pl = gtk_widget_create_pango_layout (widget, NULL);
  PangoContext *context = pango_layout_get_context (pl);
  PangoFontMetrics *metrics
    = pango_context_get_metrics (context,
				 widget->style->font_desc,
				 pango_context_get_language (context));

  month_view->char_height = (pango_font_metrics_get_ascent (metrics)
			     + pango_font_metrics_get_descent (metrics))
    / PANGO_SCALE;
  month_view->title_height = month_view->char_height + 2;

  month_view->char_width
    = pango_font_metrics_get_approximate_char_width (metrics) / PANGO_SCALE;

  g_object_unref (pl);
  pango_font_metrics_unref (metrics);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);
}

/* Get the bounding box of a cell.  Only return those dimensions for
   which a non-NULL location is provided.  */
static void
gtk_month_view_cell_box (GtkMonthView *month_view, int col, int row,
			 int *x, int *y, int *w, int *h)
{
  g_assert (col >= 0);
  g_assert (col <= 7);
  g_assert (row >= 0);
  g_assert (row < month_view->weeks);

  if (x || w)
    {
      int t;
      if (! x)
	x = &t;

      /* Left edge of cell.  We increase the width by 1 to gain an
	 extra pixel: no need to paint a black line down the
	 right.  */
      *x = col * (month_view->width + 1) / 7;

      /* Width.  */
      if (w)
	*w = (col + 1) * (month_view->width + 1) / 7 - *x;
    }

  if (y || h)
    {
      int t;
      if (! y)
	y = &t;

      /* Top of cell.  */
      *y = row * (month_view->height - month_view->title_height)
	/ month_view->weeks + month_view->title_height;

      /* Height.  */
      *h = (row + 1) * (month_view->height - month_view->title_height)
	/ month_view->weeks + month_view->title_height - *y;
    }
}

/* Get cell at X x Y.  Return -1 if the coordinates do not name a
   cell.  */
static void
gtk_month_view_cell_at (GtkMonthView *month_view, int x, int y,
			int *col, int *row)
{
  *col = x * 7 / month_view->width;
  *row = (y - month_view->title_height) * month_view->weeks
    / (month_view->height - month_view->title_height);
}

/* Cause a cell to be redrawn.  */
static void
gtk_month_view_invalidate_cell (GtkMonthView *month_view, gint cell)
{
  int col = cell % 7;
  int row = cell / 7;
  int x, y, w, h;

  g_assert (cell < month_view->weeks * 7);

  gtk_month_view_cell_box (month_view, col, row, &x, &y, &w, &h);
  gtk_widget_queue_draw_area (month_view->draw, x, y, w, h);
}

static void
scroll_to_focused_day (GtkMonthView *month_view)
{
  if (! GTK_WIDGET_REALIZED (month_view))
    return;

  if (month_view->zoom_factor == 0)
    return;

  void do_scroll (GtkAdjustment *adj,
		  int x, int width, int canvas_width, int visible_width)
    {
      int visible_x = canvas_width
	* (adj->value - adj->lower) / (adj->upper - adj->lower);

      if (x < visible_x + visible_width / 8)
	x = x - visible_width / 8;
      else if (x + width > visible_x + 7 * visible_width / 8)
	x = x - visible_width + width + visible_width / 8;
      else
	return;

      gtk_adjustment_set_value
	(adj,
	 CLAMP ((gdouble) x / canvas_width
		* (adj->upper - adj->lower) + adj->lower,
		adj->lower, adj->upper - adj->page_size));
    }

  int row = month_view->focused_day / 7;
  int col = month_view->focused_day % 7;
  int x, y, h, w;
  gtk_month_view_cell_box (month_view, col, row, &x, &y, &w, &h);

  GtkWidget *viewport = GTK_BIN (month_view->scrolled_window)->child;

  gdk_window_freeze_updates (month_view->draw->window);

  do_scroll (month_view->hadj, x, w,
	     month_view->width, viewport->allocation.width);
  do_scroll (month_view->vadj, y, h,
	     month_view->height, viewport->allocation.height);

  gdk_window_thaw_updates (month_view->draw->window);
}

static void
gtk_month_view_set_time (GtkView *view, time_t c)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (view);
  if (month_view->pending_reload)
    return;

  GDate current;
  g_date_set_time_t (&current, c);
  GDate new;
  g_date_set_time_t (&new, gtk_view_get_time (view));

  int diff = g_date_days_between (&month_view->day[0].date, &new);
  if ((! month_view->drag.dragging
       && 0 <= diff && diff < 7 * month_view->weeks)
      || (month_view->drag.dragging
	  && g_date_get_month (&current) == g_date_get_month (&new)))
    /* Same month.  */
    {
      if (diff != month_view->focused_day)
	/* But different day.  */
	{
	  gtk_month_view_invalidate_cell (month_view, month_view->focused_day);
	  month_view->focused_day = diff;
	  gtk_month_view_invalidate_cell (month_view, month_view->focused_day);
	  scroll_to_focused_day (month_view);
	}
    }
  else
    gtk_month_view_reload_events (view);
}

static gboolean
button_press_event (GtkWidget *widget, GdkEventButton *event,
		    GtkMonthView *month_view)
{
  int col, row;
  int day;
  struct day *c;

  gtk_widget_grab_focus (GTK_WIDGET (month_view));

  gtk_month_view_cell_at (month_view, event->x, event->y, &col, &row);
  day = col + row * 7;
  if (day < 0 || day >= 7 * month_view->weeks)
    return FALSE;
  c = &month_view->day[day];

  time_t t = gtk_view_get_time (GTK_VIEW (month_view));
  struct tm tm;
  localtime_r (&t, &tm);
  tm.tm_year = g_date_get_year (&c->date) - 1900;
  tm.tm_mon = g_date_get_month (&c->date) - 1;
  tm.tm_mday = g_date_get_day (&c->date);
  tm.tm_isdst = -1;

  if (event->button == 3)
    /* Right click.  Popup a menu.  */
    {
      gtk_view_set_time (GTK_VIEW (month_view), mktime (&tm));

      gtk_menu_popup (day_popup (&c->date, c->events),
		      NULL, NULL, NULL, NULL,
		      event->button, event->time);

      return TRUE;
    }

  return FALSE;
}

static void reload_events_hard (GtkMonthView *view);

static void
update_extents_hard (GtkMonthView *month_view)
{
  while (month_view->pending_update_extents)
    {
      month_view->pending_update_extents = FALSE;

      /* The size of the canvas is the size of the viewport with a
	 particular magnification.  */
      GtkWidget *viewport = GTK_BIN (month_view->scrolled_window)->child;
      int height = zoom_factors[month_view->zoom_factor]
	* viewport->allocation.height / zoom_factors[0];
      int width = zoom_factors[month_view->zoom_factor]
	* viewport->allocation.width / zoom_factors[0];

      if (month_view->width != width || month_view->height != height)
	/* The size has changed.  */
	{
	  month_view->width = width;
	  month_view->height = height;
	  gtk_widget_set_size_request (month_view->draw, width, height);

	  /* Process any size changes now.  The size renegotiation may
	     reset MONTH_VIEW->PENDING_UPDATE_EXTENTS.  Additionally,
	     we need to let the resize propagate first or the scroll
	     will not work correctly.  */
	  g_main_context_iteration (NULL, FALSE);

	  scroll_to_focused_day (month_view);
	}
    }
}

static void
update_extents (GtkMonthView *month_view)
{
  month_view->pending_update_extents = TRUE;
  gdk_window_invalidate_rect (GTK_WIDGET (month_view->draw)->window,
			      NULL, FALSE);
}

/* 0, 7 are Sunday.  */
static nl_item days_of_week[] = 
  { ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, ABDAY_1 };

static gboolean
draw_expose_event (GtkWidget *widget, GdkEventExpose *event,
		   GtkMonthView *month_view)
{
  if (event->count > 0)
    return FALSE;

  gdk_window_freeze_updates (month_view->draw->window);

  if (month_view->pending_reload)
    reload_events_hard (month_view);
  if (month_view->pending_update_extents)
    update_extents_hard (month_view);

  int last_expose = ++ month_view->last_expose;
  gdk_window_thaw_updates (month_view->draw->window);
  gdk_window_process_updates (month_view->draw->window, FALSE);
  if (month_view->last_expose != last_expose)
    return FALSE;

  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *light_gray_gc;
  GdkGC *yellow_gc;
  GdkGC *salmon_gc;
  GdkGC *blue_gc;
  GdkColor yellow;
  GdkColor salmon;
  GdkColormap *colormap;
  gint row, col;
  PangoLayout *pl;
  PangoLayout *pl_evt;
  PangoRectangle pr;
  const nl_item *days;

  days = &days_of_week[week_starts_sunday ? 0 : 1];
  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  light_gray_gc = pen_new (widget, 53040, 53040, 53040);
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

  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;

  pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
  pl_evt = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;

  gdk_window_clear_area (drawable, 
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);

  gdk_draw_rectangle (drawable, light_gray_gc, 1,
		      0, 0, month_view->width, month_view->title_height);

  /* Paint the events.  */

  /* Determine the first row with damage.  */
  int row_start, col_start;
  gtk_month_view_cell_at (month_view, event->area.x, event->area.y,
			  &col_start, &row_start);
  col_start = MIN (MAX (col_start, 0), 7);
  row_start = MIN (MAX (row_start, 0), month_view->weeks - 1);

  int row_last, col_last;
  gtk_month_view_cell_at (month_view,
			  event->area.x + event->area.width,
			  event->area.y + event->area.height,
			  &col_last, &row_last);
  col_last = MIN (MAX (col_last, 0), 6);
  row_last = MIN (MAX (row_last, 0), month_view->weeks - 1);

  int font_size = CLAMP (month_view->width / 7 / 14, 8, 14) * 1024;

  for (row = row_start; row <= row_last; row ++)
    {
      gint y, h;
      gtk_month_view_cell_box (month_view, 0, row, NULL, &y, NULL, &h);

      for (col = col_start; col <= col_last; col ++)
	{
	  gint day = col + (7 * row);
	  struct day *c = &month_view->day[day];
	  gint x, w;

	  gtk_month_view_cell_box (month_view, col, 0, &x, NULL, &w, NULL);

	  if (row == 0)
	    /* Draw the column's title.  */
	    {
	      gchar *s = g_locale_to_utf8 (nl_langinfo (days[col]), -1,
					   NULL, NULL, NULL);
	      pango_layout_set_text (pl, s, -1);
	      pango_layout_get_pixel_extents (pl, &pr, NULL);
	      g_free (s);
      
	      gtk_paint_layout (widget->style,
				widget->window,
				GTK_WIDGET_STATE (widget),
				FALSE,
				&event->area,
				widget,
				"label",
				x + (w - pr.width) / 2, 1,
				pl);
	    }

	  GdkGC *color;

	  if (week_starts_sunday)
	    color = col == 0 || col == 6 ? salmon_gc : yellow_gc;
	  else
	    color = col >= 5 ? salmon_gc : yellow_gc;

	  pango_layout_set_width (pl_evt, w * PANGO_SCALE);
	  int trying_no_wrap = FALSE;
	restart:
	  gdk_draw_rectangle (drawable, color, TRUE, x, y + 1, w, h);

	  /* Draw the top edge of the box.  */
	  gdk_draw_line (drawable, black_gc,
			 x, y, x + w - 1, y);
	  /* Draw the right hand side of the box.  */
	  gdk_draw_line (drawable, black_gc,
			 x + w - 1, y + 1, x + w - 1, y + h - 1);


	  int top = y + 2;
	  int left = x + 2;

	  char buffer[30];
	  int written = snprintf (buffer, sizeof (buffer),
				  "<span size='%d'>%d</span>", font_size,
				  g_date_get_day (&c->date));

	  pango_layout_set_markup (pl_evt, buffer, written);
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
	  left += pr.width + 3;

	  GSList *iter;
	  for (iter = c->events; iter; iter = iter->next)
	    {
	      Event *ev = iter->data;
	      if (! event_get_visible (ev))
		continue;

	      char *t = event_get_summary (ev);
	      char *s = g_strdup_printf ("<span size='%d'>%s</span>",
					 font_size, t);
	      g_free (t);
	      /* Replace '\n''s with spaces.  */
	      for (t = strchr (s, '\n'); t; t = strchr (t, '\n'))
		*t = ' ';

	      pango_layout_set_markup (pl_evt, s, -1);
	      g_free (s);

	      pango_layout_get_pixel_extents (pl_evt, NULL, &pr);
	      if (top + pr.height > y + h && ! trying_no_wrap)
		{
		  trying_no_wrap = TRUE;
		  pango_layout_set_width (pl_evt, -1);
		  goto restart;
		}

	      GdkColor color;
	      if (event_get_color (ev, &color))
		{
		  GdkGC *color_gc;

		  color_gc = gdk_gc_new (widget->window);
		  gdk_gc_copy (color_gc, widget->style->black_gc);

		  gdk_colormap_alloc_color (colormap, &color, FALSE, TRUE);
		  gdk_gc_set_foreground (color_gc, &color);

		  gdk_draw_rectangle (drawable, color_gc, TRUE,
				      left, top,
				      MIN (pr.width + 2, w - 2), pr.height);
		  gdk_draw_rectangle (drawable, black_gc, FALSE,
				      left, top,
				      MIN (pr.width + 2, w - 2), pr.height);

		  g_object_unref (color_gc);
		}

	      gtk_paint_layout (widget->style,
				widget->window,
				GTK_WIDGET_STATE (widget),
				FALSE,
				&event->area,
				widget,
				"label",
				left + 1, top,
				pl_evt);

	      top += pr.height + 2;
	      left = x + 1;
	    }

	  if (day == month_view->focused_day)
	    /* Highlight this day.  */
	    gdk_draw_rectangle (drawable, blue_gc, FALSE,
				x, y + 1, w - 2, h - 2);
	}
    }

  gdk_gc_unref (blue_gc);
  gdk_gc_unref (light_gray_gc);
  gdk_gc_unref (yellow_gc);
  gdk_gc_unref (salmon_gc);

  g_object_unref (pl);
  g_object_unref (pl_evt);

  return TRUE;
}

static void
gtk_month_view_reload_events (GtkView *view)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (view);
  month_view->pending_reload = TRUE;

  gtk_widget_queue_draw (GTK_WIDGET (month_view->draw));
}

/* Changes DATE to name the first day to display for the month it
   names and sets *WEEKS to the the number of weeks what need to be
   display.  For instance, if date names July, 2006:

             July        
     Su Mo Tu We Th Fr Sa
                        1
      2  3  4  5  6  7  8
      9 10 11 12 13 14 15
     16 17 18 19 20 21 22
     23 24 25 26 27 28 29
     30 31                                       

   and week_starts_sunday is true, then the date is set to June 25 and
   *WEEKS to 6.  If week_starts_sunday is false, then the date is set
   to June 26 and *WEEKS to 5.
*/
static void
first_day (GDate *date, int *weeks)
{
  g_date_set_day (date, 1);
  int wday = g_date_weekday (date) - 1;
  if (week_starts_sunday)
    wday = (wday + 1) % 7;

  int days = g_date_get_days_in_month (g_date_get_month (date),
				       g_date_get_year (date));
  *weeks = (wday + days + 6) / 7;

  if (wday)
    /* We don't start on a monday.  Find the first day.  */
    g_date_subtract_days (date, wday);
}

static void
reload_events_hard (GtkMonthView *month_view)
{
  time_t t = gtk_view_get_time (GTK_VIEW (month_view));
  struct tm tm_start;
  guint year, month, day;

  /* Destroy outstanding events.  */
  int i;
  for (i = 0; i < MAX_DAYS; i ++)
    if (month_view->day[i].events)
      {
	/* Destroy outstanding events.  */
	event_list_unref (month_view->day[i].events);
	month_view->day[i].events = NULL;
      }

  localtime_r (&t, &tm_start);
  year = tm_start.tm_year + 1900;
  month = tm_start.tm_mon;
  day = tm_start.tm_mday;

  /* 0 => Monday.  */
  GDate period_start;
  g_date_set_dmy (&period_start, 1, month + 1, year);
  first_day (&period_start, &month_view->weeks);

  GDate period_end = period_start;
  /* The day following the last day.  */
  g_date_add_days (&period_end, month_view->weeks * 7);

  struct tm start_tm;
  memset (&start_tm, 0, sizeof (start_tm));
  start_tm.tm_year = g_date_get_year (&period_start) - 1900;
  start_tm.tm_mon = g_date_get_month (&period_start) - 1;
  start_tm.tm_mday = g_date_get_day (&period_start);
  start_tm.tm_isdst = -1;

  struct tm end_tm;
  memset (&end_tm, 0, sizeof (end_tm));
  end_tm.tm_year = g_date_get_year (&period_end) - 1900;
  end_tm.tm_mon = g_date_get_month (&period_end) - 1;
  end_tm.tm_mday = g_date_get_day (&period_end);
  end_tm.tm_isdst = -1;

  /* Initialize the days.  */
  GDate d;
  for (i = 0, d = period_start;
       i < month_view->weeks * 7;
       i ++, g_date_add_days (&d, 1))
    {
      month_view->day[i].date = d;

      if (tm_start.tm_mday == g_date_get_day (&d)
	  && tm_start.tm_mon == g_date_get_month (&d) - 1)
        month_view->focused_day = i;
    }

  /* Get the events for the period.  */
  GSList *events
    = event_db_list_for_period (event_db,
				mktime (&start_tm), mktime (&end_tm) - 1);

  /* PERIOD_END is the day after the end.  Before we want the day of
     the end.  */
  g_date_subtract_days (&period_end, 1);

  guint period_start_day = g_date_get_julian (&period_start);

  GSList *l;
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
	  month_view->day[i - period_start_day].events
	    = g_slist_prepend (month_view->day[i - period_start_day].events,
			       ev);
	}
    }
  event_list_unref (events);

  /* Sort the lists.  */
  for (i = 0; i < month_view->weeks * 7; i ++)
    if (month_view->day[i].events)
      month_view->day[i].events
	= g_slist_sort (month_view->day[i].events, event_compare_func);

  month_view->pending_reload = FALSE;
  scroll_to_focused_day (month_view);

  gdk_window_invalidate_rect (GTK_WIDGET (month_view->draw)->window,
			      NULL, FALSE);
}

/* The viewport's size allocation.  */
static void
size_allocate (GtkWidget *widget, GtkAllocation *allocation,
	       GtkMonthView *month_view)
{
  month_view->pending_update_extents = TRUE;
  /* We don't call update_extents as it will invalidate the draw area
     and we don't actually know if the size has even changed.  */
  update_extents_hard (month_view);
}

/* Called periodically when the mouse is held down to continue
   scrolling.  */
static gboolean
page_flip (GtkMonthView *month_view)
{
  guint32 t = gdk_x11_get_server_time (month_view->draw->window);

  if (month_view->drag.last_flip == 0)
    month_view->drag.last_flip = t;

  if (t - month_view->drag.last_motion >= 100)
    /* Greater than a 100 ms since the user last moved the mouse.  If
       the user has moved the mouse beyond the start point then do a
       bit of scrolling.  */
    {
      int dx = month_view->drag.x_origin - month_view->drag.x;
      int dy = month_view->drag.y_origin - month_view->drag.y;

      gdouble dx_share = (gdouble) (dx * dx) / (dx * dx + dy * dy);
      gdouble dy_share = (gdouble) (dy * dy) / (dx * dx + dy * dy);

      GtkAdjustment *hadj = month_view->hadj;
      GtkAdjustment *vadj = month_view->vadj;

      if (hadj->value <= hadj->lower && dx_share > 0.3
	  && t - month_view->drag.last_flip > 1000)
	/* Horizontal edge: flip to the left (past).  */
	{
	  if (vadj->value >= vadj->upper - vadj->page_size
	      && dy_share > 0.3)
	    /* But vertical edge says down!  Do nothing.  */
	    ;
	  else
	    /* Flip backwards one month.  */
	    {
	      month_view->drag.last_flip = t;

	      GDate date;
	      gtk_view_get_date (GTK_VIEW (month_view), &date);
	      g_date_set_day (&date, 1);
	      g_date_subtract_months (&date, 1);
	      int weeks;
	      first_day (&date, &weeks);
	      g_date_add_days (&date,
			       MIN (month_view->focused_day / 7 * 7 + 6,
				    weeks * 7 - 1));
	      gtk_view_set_date (GTK_VIEW (month_view), &date);
	    }
	}
      else if (vadj->value <= vadj->lower && dy_share > 0.3
	       && t - month_view->drag.last_flip > 500)
	/* Vertical edge: flip up (past).  */
	{
	  if (hadj->value >= hadj->upper - hadj->page_size
	      && dx_share > 0.3)
	    /* But horizontal edge says right!  Do nothing.  */
	    ;
	  else
	    {
	      month_view->drag.last_flip = t;

	      GDate date;
	      gtk_view_get_date (GTK_VIEW (month_view), &date);

	      int wday = g_date_get_weekday (&date);

	      /* Go to the last day of the previous month.  */
	      g_date_set_day (&date, 1);
	      g_date_subtract_days (&date, 1);
	      int month = g_date_get_month (&date);
	      int weeks;
	      first_day (&date, &weeks);
	      g_date_add_days (&date, (weeks - 1) * 7 + wday - 1);
	      if (g_date_get_month (&date) != month)
		g_date_subtract_days (&date, 7);
	      gtk_view_set_date (GTK_VIEW (month_view), &date);
	    }
	}
      else if (hadj->value >= hadj->upper - hadj->page_size
	       && dx_share > 0.3
	       && t - month_view->drag.last_flip > 1000)
	/* Flip to the right (future).  */
	{
	  month_view->drag.last_flip = t;

	  GDate date;
	  gtk_view_get_date (GTK_VIEW (month_view), &date);
	  g_date_set_day (&date, 1);
	  g_date_add_months (&date, 1);
	  int weeks;
	  first_day (&date, &weeks);
	  g_date_add_days (&date,
			   MIN (month_view->focused_day / 7 * 7,
				weeks * 7 - 1));
	  gtk_view_set_date (GTK_VIEW (month_view), &date);
	}
      else if (vadj->value >= vadj->upper - vadj->page_size
	       && dy_share > 0.3
	       && t - month_view->drag.last_flip > 1000)
	/* Flip down (future).  */
	{
	  month_view->drag.last_flip = t;

	      GDate date;
	      gtk_view_get_date (GTK_VIEW (month_view), &date);

	      int wday = g_date_get_weekday (&date);

	      /* Go to the last day of the previous month.  */
	      g_date_set_day (&date, 1);
	      g_date_add_months (&date, 1);
	      int month = g_date_get_month (&date);
	      int weeks;
	      first_day (&date, &weeks);
	      g_date_add_days (&date, wday - 1);
	      if (g_date_get_month (&date) != month)
		g_date_add_days (&date, 7);
	      gtk_view_set_date (GTK_VIEW (month_view), &date);
	}
      else
	/* Scroll a bit.  */
	{
	  void scroll (GtkAdjustment *adj, int diff, int dimension,
		       gdouble share)
	    {
	      if (! (diff < -10 || 10 <= diff))
		return;

	      /* Scroll by 10 * share pixels.  */
	      gdouble inc
		= (10 * share / dimension) * (adj->upper - adj->lower);
	      if (diff < 0)
		inc = -inc;

	      gtk_adjustment_set_value
		(adj, CLAMP (adj->value + inc,
			     adj->lower, adj->upper - adj->page_size));
	    }

	  scroll (hadj, dx, month_view->width, dx_share);
	  scroll (vadj, dy, month_view->height, dy_share);
	}
    }
  return TRUE;
}

static gboolean
motion_notify_event (GtkWidget *widget, GdkEventMotion *event,
		     GtkMonthView *month_view)
{
  if (! month_view->drag.dragging)
    /* This is the start of a drag event.  */
    {
      month_view->drag.dragging = TRUE;
      month_view->drag.last_flip = 0;
      month_view->drag.x_origin = event->x_root;
      month_view->drag.y_origin = event->y_root;
    }
  else
    {
      GtkWidget *viewport = GTK_BIN (month_view->scrolled_window)->child;

      int x, y;
      gtk_widget_translate_coordinates (month_view->draw, viewport,
					event->x, event->y, &x, &y);
      if (x < 0 || y < 0
	  || x >= viewport->allocation.width
	  || y >= viewport->allocation.height)
	/* The mouse has gone outside of the viewport.  Do automatic
	   scrolling.  */
	{
	  if (! month_view->drag.page_timer)
	    month_view->drag.page_timer
	      = g_timeout_add (100, (GSourceFunc) page_flip, month_view);
	}
      else if (month_view->drag.page_timer)
	/* The mouse has returned inside the viewport.  Remove
	   automatic scrolling.  */
	{
	  g_source_remove (month_view->drag.page_timer);
	  month_view->drag.page_timer = 0;
	  month_view->drag.last_flip = 0;
	}

      void scroll_by (GtkAdjustment *adj, gdouble delta, int dimension)
	{
	  /* Current position of scroll bar as a percentage.  */
	  gdouble pos = (adj->value - adj->lower)
	    / (adj->upper - adj->lower);
	  /* New position of scroll bar as a percentage.  */
	  gdouble new_pos = (pos * dimension + delta) / dimension;

	  gtk_adjustment_set_value
	    (adj, CLAMP (new_pos * (adj->upper - adj->lower) + adj->lower,
			 adj->lower, adj->upper - adj->page_size));
	}

      scroll_by (month_view->vadj, - (event->y_root - month_view->drag.y),
		 month_view->height);
      scroll_by (month_view->hadj, - (event->x_root - month_view->drag.x),
		 month_view->width);
    }

  month_view->drag.x = event->x_root;
  month_view->drag.y = event->y_root;
  month_view->drag.last_motion = event->time;

  return TRUE;
}

static gboolean
button_release_event (GtkWidget *widget, GdkEventButton *event,
		      GtkMonthView *month_view)
{
  gboolean was_dragging = month_view->drag.dragging;
  month_view->drag.dragging = FALSE;

  if (month_view->drag.page_timer)
    {
      g_source_remove (month_view->drag.page_timer);
      month_view->drag.page_timer = 0;
    }

  int col, row;
  gtk_month_view_cell_at (month_view, event->x, event->y, &col, &row);

  int day = col + row * 7;
  if (day < 0 || day >= 7 * month_view->weeks)
    return FALSE;

  if (event->button == 1)
    {
      struct day *c = &month_view->day[day];

      time_t t = gtk_view_get_time (GTK_VIEW (month_view));
      struct tm tm;
      localtime_r (&t, &tm);
      tm.tm_year = g_date_get_year (&c->date) - 1900;
      tm.tm_mon = g_date_get_month (&c->date) - 1;
      tm.tm_mday = g_date_get_day (&c->date);
      tm.tm_isdst = -1;

      if (day == month_view->focused_day)
	{
	  if (! was_dragging)
	    /* The user clicked on the focused day and there was no drag
	       motion.  Zoom to that day.  */
	    set_time_and_day_view (mktime (&tm));
	}
      else
	gtk_view_set_time (GTK_VIEW (month_view), mktime (&tm));

      return FALSE;
    }

  return TRUE;
}

void
month_view_mod_zoom_factor (GtkMonthView *month_view, int delta)
{
  month_view->zoom_factor += delta;

  if (month_view->zoom_factor < 0)
    month_view->zoom_factor = 0;
  if (month_view->zoom_factor >= ZOOM_FACTORS)
    {
      set_time_and_day_view (gtk_view_get_time (GTK_VIEW (month_view)));
      return;
    }

  update_extents (month_view);
}

void
month_view_set_zoom (GtkMonthView *month_view, int value)
{
  if (value < 0)
    value = ZOOM_FACTORS + value;
  month_view->zoom_factor = CLAMP (value, 0, ZOOM_FACTORS - 1);
  month_view_mod_zoom_factor (month_view, 0);
}

static gboolean
gtk_month_view_key_press_event (GtkWidget *widget, GdkEventKey *k)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (widget);
  struct day *c = &month_view->day[month_view->focused_day];
  int i;
 
  i = 0;
  if (k->keyval == GDK_Right)
    i = 1;
  else if (k->keyval == GDK_Left)
    i = -1;
  else if (k->keyval == GDK_Down)
    i = 7;
  else if (k->keyval == GDK_Up)
    i = -7;
  else if (k->keyval == GDK_Page_Up)
    i = - (7 * month_view->weeks);
  else if (k->keyval == GDK_Page_Down)
    i = 7 * month_view->weeks;

  if (i)
    {
      GDate date;
      gtk_view_get_date (GTK_VIEW (month_view), &date);
      if (i < 0)
	g_date_subtract_days (&date, - i);
      else
	g_date_add_days (&date, i);
      gtk_view_set_date (GTK_VIEW (month_view), &date);
      return TRUE;
    }

  if (k->keyval == GDK_space)
    {
      gtk_menu_popup (day_popup (&c->date, c->events),
		      NULL, NULL, NULL, NULL,
		      0, gtk_get_current_event_time());
      return TRUE;
    }  
     
  if (k->keyval == GDK_Return)
    {
      struct tm tm;
      g_date_to_struct_tm (&c->date, &tm);
      set_time_and_day_view (mktime (&tm));
      return TRUE; 
    }

  if (k->keyval == '+' || k->keyval == '='
#if IS_HILDON
      || k->keyval == GDK_F7
#endif
      )
    /* Zoom in.  */
    {
      month_view_mod_zoom_factor (month_view, 1);
      return TRUE;
    }
  if (k->keyval == '-'
#if IS_HILDON
      || k->keyval == GDK_F8
#endif
      )
    /* Zoom out.  */
    {
      month_view_mod_zoom_factor (month_view, -1);
      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, k);
}

GtkWidget *
gtk_month_view_new (time_t time)
{
  GtkMonthView *month_view;
  GtkWidget *scrolled_window;

  month_view = GTK_MONTH_VIEW (g_object_new (gtk_month_view_get_type (),
					     NULL));
  GTK_WIDGET_SET_FLAGS (month_view, GTK_CAN_FOCUS);
  gtk_widget_add_events (GTK_WIDGET (month_view), GDK_KEY_PRESS_MASK);

  month_view->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0));
  month_view->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0));
  scrolled_window = gtk_scrolled_window_new (month_view->hadj,
					     month_view->vadj);
  month_view->scrolled_window = GTK_SCROLLED_WINDOW (scrolled_window);
  gtk_box_pack_start (GTK_BOX (month_view), scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled_window);
	  
  month_view->draw = gtk_drawing_area_new ();
  gtk_widget_set_app_paintable (month_view->draw, TRUE);
  gtk_widget_add_events (GTK_WIDGET (month_view->draw),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK
			 | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (month_view->draw), "button-press-event",
		    G_CALLBACK (button_press_event), month_view);
  g_signal_connect (G_OBJECT (month_view->draw), "expose_event",
		    G_CALLBACK (draw_expose_event), month_view);
  g_signal_connect (G_OBJECT (month_view->draw), "motion_notify_event",
		    G_CALLBACK (motion_notify_event), month_view);
  g_signal_connect (G_OBJECT (month_view->draw), "button-release-event",
		    G_CALLBACK (button_release_event), month_view);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
					 month_view->draw);
  gtk_widget_show (month_view->draw);

  GtkWidget *viewport = GTK_BIN (scrolled_window)->child;
  g_signal_connect (G_OBJECT (viewport), "size-allocate",
		    G_CALLBACK (size_allocate), month_view);

  gtk_view_set_time (GTK_VIEW (month_view), time);

  return GTK_WIDGET (month_view);
}
