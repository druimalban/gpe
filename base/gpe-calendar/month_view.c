/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <time.h>
#include <langinfo.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#include <gpe/event-db.h>
#include "view.h"
#include "event-ui.h"
#include "globals.h"
#include "month_view.h"
#include "day_popup.h"

#include "gtkdatesel.h"

#define TOTAL_DAYS (6 * 7)
#define TOTAL_WEEKS (TOTAL_DAYS / 7)
#define MAX_DAYS_IN_MONTH 32

#ifdef IS_HILDON
/* Hildon seems to eat up some pixels itself :-( */
#  define WIDTH_DELTA -6
#else
#  define WIDTH_DELTA 0
#endif

/* A render control.  POPUP is only contains valid data if VALID is
   true.  */
struct render_ctl
{
  struct day_popup popup;
  gboolean valid;
};

#define is_today(c) \
  ({ \
     struct tm tm; \
     localtime_r (&viewtime, &tm); \
     tm.tm_year == (c)->popup.year && tm.tm_mon == c->popup.month \
       && tm.tm_mday == c->popup.day; \
   })

/* A month view consists of a number of cells: 7 per row up to 6
   columns (as that is the maximum number of weeks any week requires
   independent of the day it starts on.

   Each cell that actually display a day from the displayed month has
   a render control associated with it.  This render control is marked
   as valid and has the date the cell represents and a pointer to the
   event list (if any).  The event list can also be found in
   DAY_EVENTS.  DAY_EVENTS is indexed by the cells day in the month,
   NOT BY THE CELL number.  */
struct _GtkMonthView
{
  GtkView widget;
  GtkWidget *draw;

  /* Events attacked to the day.  */
  GSList *day_events[MAX_DAYS_IN_MONTH];

  /* Day which has the focus.  */
  guint focused_day;

  /* Day which the popup menu is showing (if any!).  */
  struct render_ctl *has_popup;
  /* One for each cell.  */
  struct render_ctl rc[TOTAL_DAYS];

  /* Amount of canvas allocated.  */
  gint width, height;
  /* Amount of canvas the title bar requries.  */
  gint title_height;

  /* Number of weeks in the current month.  */
  gint weeks;
};

typedef struct
{
  GtkViewClass view_class;
  GObjectClass parent_class;
} GtkMonthViewClass;

static void gtk_month_view_base_class_init (gpointer klass);
static void gtk_month_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_month_view_dispose (GObject *obj);
static void gtk_month_view_finalize (GObject *object);
static void gtk_month_view_set_time (GtkView *view, time_t time);
static void gtk_month_view_reload_events (GtkView *view);

static GtkWidgetClass *parent_class;

GType
gtk_month_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (GtkMonthViewClass),
	gtk_month_view_base_class_init,
	NULL,
	NULL,
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
gtk_month_view_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_view_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_month_view_finalize;
  object_class->dispose = gtk_month_view_dispose;

  widget_class = (GtkWidgetClass *) klass;

  view_class = (GtkViewClass *) klass;
  view_class->set_time = gtk_month_view_set_time;
  view_class->reload_events = gtk_month_view_reload_events;
}

static void
gtk_month_view_init (GTypeInstance *instance, gpointer klass)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (instance);
  int day;

  month_view->focused_day = 0;
  month_view->width = 0;
  month_view->height = 0;
  month_view->weeks = TOTAL_WEEKS;
  month_view->has_popup = 0;

  for (day = 0; day < MAX_DAYS_IN_MONTH; day ++)
    month_view->day_events[day] = NULL;
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

  for (i = 0; i < MAX_DAYS_IN_MONTH; i++)
    if (month_view->day_events[i]) 
      event_db_list_destroy (month_view->day_events[i]);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
gtk_month_view_set_time (GtkView *view, time_t current)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (view);
  time_t new = gtk_view_get_time (view);
  struct tm current_tm;
  struct tm new_tm;

  localtime_r (&current, &current_tm);
  localtime_r (&new, &new_tm);

  if (current_tm.tm_year == new_tm.tm_year
      && current_tm.tm_mon == new_tm.tm_mon)
    /* Same month.  */
    {
      int offset = 0;
      while (! month_view->rc[offset].valid)
	offset ++;

      if (new_tm.tm_mday - 1 != month_view->focused_day - offset)
	/* But different day.  */
	{
	  gtk_month_view_invalidate_cell (month_view, month_view->focused_day);
	  month_view->focused_day = new_tm.tm_mday - 1 + offset;
	  gtk_month_view_invalidate_cell (month_view, month_view->focused_day);
	}
      return;
    }

  gtk_month_view_reload_events (view);
}

static gboolean
button_press (GtkWidget *widget, GdkEventButton *event, GtkWidget *mv)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (mv);
  int col, row;
  int day;
  struct render_ctl *c;

  gtk_month_view_cell_at (month_view, event->x, event->y, &col, &row);
  day = col + row * 7;
  if (day < 0 || day >= 7 * month_view->weeks)
    return FALSE;
  c = &month_view->rc[day];

  if (event->type == GDK_BUTTON_PRESS)
    {
      if (event->button == 3)
	/* Right click.  Popup a menu.  */
	{
	  if (pop_window) 
	    gtk_widget_destroy (pop_window);

	  if (c->valid && c != month_view->has_popup) 
	    {
	      pop_window = day_popup (main_window, &c->popup, TRUE);
	      month_view->has_popup = c;
	    }
	  else 
	    {
	      pop_window = NULL;
	      month_view->has_popup = NULL;
	    }
	}
      else
	/* Left click.  Select the day.  */
	{
	  if (day != month_view->focused_day)
	    {
	      if (month_view->rc[day].valid)
		gtk_view_set_time (GTK_VIEW (month_view),
				   time_from_day (c->popup.year,
						  c->popup.month,
						  c->popup.day));
	      else
		/* Change months.  */
		{
		  int d = day < 7 ? 1 : -1;
		  int i = 0;

		  /* Determine the number of days before (or after)
		     the month.  */
		  while (! c[i].valid)
		    i += d;

		  gtk_view_set_time (GTK_VIEW (month_view),
				     time_from_day (c[i].popup.year,
						    c[i].popup.month,
						    c[i].popup.day)
				    - i * 24 * 60 * 60);
		}

	      /* In case the draw doesn't have the focus.  */
	      gtk_widget_grab_focus (month_view->draw);
	    }
	  else
	    /* Click on an active box: move to the day view.  */
	    {
	      time_t t = gtk_view_get_time (GTK_VIEW (month_view));
	      struct tm tm;
	      time_t selected_time;

	      localtime_r (&t, &tm);
	      tm.tm_year = c->popup.year;
	      tm.tm_mon = c->popup.month;
	      tm.tm_mday = c->popup.day;
	      selected_time = mktime (&tm);
	      if (pop_window) 
		gtk_widget_destroy (pop_window);
	      pop_window = NULL;
	      month_view->has_popup = NULL;
	      set_time_and_day_view (selected_time);
	    }
	}

      return TRUE;
    }
  
  return FALSE;
}

/* 0, 7 are Sunday.  */
static nl_item days_of_week[] = 
  { ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, ABDAY_1 };

static void
calc_title_height (GtkMonthView *month_view)
{
  GtkWidget *widget = month_view->draw;
  PangoLayout *pl;
  PangoRectangle pr;
  int i;
  int max_height = 0;

  pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);

  for (i = 0; i < 7; i++)
    {
      gchar *s = g_locale_to_utf8 (nl_langinfo (days_of_week[i]), -1,
                                   NULL, NULL, NULL);
      pango_layout_set_text (pl, s, -1);
      pango_layout_get_pixel_extents (pl, &pr, NULL);

      if (pr.height > max_height)
        max_height = pr.height;

      g_free (s);
    }

#ifdef IS_HILDON
  month_view->title_height = max_height + 20;
#else
  month_view->title_height = max_height + 8;
#endif	
  g_object_unref (pl);
}

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEventExpose *event,
		   GtkWidget *mv)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (mv);
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  GdkGC *cream_gc;
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

  days = &days_of_week[week_starts_monday ? 1 : 0];
  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  cream_gc = pen_new (widget, 65535, 64005, 61200);
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

  white_gc = widget->style->white_gc;
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

  for (row = row_start; row <= row_last; row ++)
    {
      gint y, h;
      gtk_month_view_cell_box (month_view, 0, row, NULL, &y, NULL, &h);

      for (col = col_start; col <= col_last; col ++)
	{
	  gint day = col + (7 * row);
	  struct render_ctl *c = &month_view->rc[day];
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

	  if (c->valid)
	    {
	      GdkGC *color;
	      gchar *buffer;
	      char buf[10];

	      if (c->popup.events)
		color = cream_gc;
	      else
		{
		  if (week_starts_monday)
		    color = col >= 5 ? salmon_gc : yellow_gc;
		  else
		    color = col == 0 || col == 6 ? salmon_gc : yellow_gc;
		}

	      gdk_draw_rectangle (drawable, color, TRUE, x, y + 1, w, h);

	      /* Draw the top edge of the box.  */
	      gdk_draw_line (drawable, black_gc,
			     x, y, x + w - 1, y);
	      /* Draw the right hand side of the box.  */
	      gdk_draw_line (drawable, black_gc,
			     x + w - 1, y + 1, x + w - 1, y + h - 1);

	      snprintf (buf, sizeof (buf), "%d", c->popup.day);
	      buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	      pango_layout_set_text (pl, buffer, -1);
	      pango_layout_get_pixel_extents (pl, &pr, NULL);
	      gtk_paint_layout (widget->style,
				widget->window,
				GTK_WIDGET_STATE (widget),
				FALSE,
				&event->area,
				widget,
				"label",
				x + 2, y,
				pl);
	      g_free (buffer);

              if (c->popup.events)
                {
                  GSList *iter;
		  gint top = y;

                  for (iter = c->popup.events; iter; iter = iter->next)
                    {
                      gchar *m;
                      top += pr.height + 2;

                      event_t ev = iter->data;
                      event_details_t evd = event_db_get_details (ev);
                      m = g_strdup_printf("<small>%s</small>", evd->summary);
    
                      pango_layout_set_width (pl_evt, w * PANGO_SCALE);
                      pango_layout_set_markup (pl_evt, m, -1);
                      gtk_paint_layout (widget->style,
					widget->window,
					GTK_WIDGET_STATE (widget),
					FALSE,
					&event->area,
					widget,
					"label",
					x + 2, top,
					pl_evt);
                      g_free(m);  
                    }
                }
	    }
	  else
	    /* Not a valid date.  */
	    {
	      /* Draw the top edge of the box.  */
	      gdk_draw_line (drawable, black_gc,
			     x, y, x + w - 1, y);
	      /* Draw the right hand side of the box.  */
	      gdk_draw_line (drawable,
			     col < 6
			     && month_view->rc[(col + 1) + row * 7].valid
			     ? black_gc : light_gray_gc,
			     x + w - 1, y + 1, x + w - 1, y + h - 1);

	      /* And flood it gray.  */
	      gdk_draw_rectangle (drawable, gray_gc, TRUE,
				  x + 1, y + 1, w - 2, h - 2);
	    }

	  if (day == month_view->focused_day)
	    /* Highlight this day.  */
	    gdk_draw_rectangle (drawable, blue_gc, FALSE,
				x, y + 1, w - 2, h - 2);
	}
    }

  gdk_gc_unref (blue_gc);
  gdk_gc_unref (cream_gc);
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
  time_t t = gtk_view_get_time (GTK_VIEW (month_view));
  guint day;
  time_t start, end;
  struct tm tm_start, tm_end;
  GSList *iter;
  guint days;
  guint year, month;
  guint wday;

  /* Destroy any popup window.  */
  if (pop_window) 
    gtk_widget_destroy (pop_window);
  pop_window = NULL;
  month_view->has_popup = NULL;

  localtime_r (&t, &tm_start);
  year = tm_start.tm_year + 1900;
  month = tm_start.tm_mon;

  days = days_in_month (year, month);
  /* 0 => Monday.  */
  wday = day_of_week (year, month + 1, 1);
  if (! week_starts_monday)
    wday = (wday + 1) % 7;

  month_view->weeks = (wday + days + 6) / 7;

  for (day = 0; day < days; day++)
    {
      localtime_r (&t, &tm_start);
      tm_start.tm_hour = 0;
      tm_start.tm_min = 0;
      tm_start.tm_sec = 0;
      tm_start.tm_mday = day + 1;
      start = mktime (&tm_start);

      localtime_r (&t, &tm_end);
      tm_end.tm_hour = 23;
      tm_end.tm_min = 59;
      tm_end.tm_sec = 59;
      tm_end.tm_mday = day + 1;
      end = mktime (&tm_end);
      
      if (!tm_start.tm_isdst) 
        {
          start += 60*60;
          end += 60*60;
        }

      if (month_view->day_events[day])
        event_db_list_destroy (month_view->day_events[day]);
      month_view->day_events[day] = event_db_list_for_period (start, end);

      for (iter = month_view->day_events[day]; iter; iter = iter->next)
        ((event_t) iter->data)->mark = FALSE;
    }

  /* Destroy any remain events.  */
  for (; day < MAX_DAYS_IN_MONTH; day++)
    if (month_view->day_events[day])
      {
	event_db_list_destroy (month_view->day_events[day]);
	month_view->day_events[day] = NULL;
      }

  localtime_r (&t, &tm_start);
  for (day = 0; day < month_view->weeks * 7; day++)
    {
      gint rday = day - wday + 1;
      struct render_ctl *c = &month_view->rc[day];
      if (c->popup.events)
        c->popup.events = NULL;
      if (rday == tm_start.tm_mday) 
        month_view->focused_day = day;

      if (rday < 1 || rday > days)
	c->valid = FALSE;
      else
        {
          c->valid = TRUE;
	  c->popup.day = rday;
	  c->popup.year = year - 1900;
	  c->popup.month = month;
          c->popup.events = month_view->day_events[rday - 1];
        }
    }

  gtk_widget_queue_draw (month_view->draw);
}

static void
resize_table (GtkWidget *widget, GtkAllocation *allocation, GtkWidget *mv)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (mv);
  gint width = allocation->width + WIDTH_DELTA;
  gint height = allocation->height;

  if (width != month_view->width || height != month_view->height)
    {
      month_view->width = width;
      month_view->height = height;

      gtk_widget_queue_draw (GTK_WIDGET (month_view->draw));
    }
}

static gboolean
month_view_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *mv)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (mv);
  struct render_ctl *c = &month_view->rc[month_view->focused_day];
  int i;
 
  if (k->keyval == GDK_Escape && c->valid)
    {
      if (pop_window) 
	gtk_widget_destroy (pop_window);
      pop_window = NULL;
      month_view->has_popup = NULL;
    }    

  i = 0;
  if (k->keyval == GDK_Right)
    i = 1;
  else if (k->keyval == GDK_Left)
    i = -1;
  else if (k->keyval == GDK_Down)
    i = 7;
  else if (k->keyval == GDK_Up)
    i = -7;

  if (month_view->focused_day + i >= 0
      && month_view->focused_day + i < month_view->weeks * 7
      && c[i].valid)
    /* Same month.  */
    gtk_view_set_time (GTK_VIEW (month_view),
		       time_from_day (c->popup.year, c->popup.month,
				      c->popup.day)
		       + i * 24 * 60 * 60);
  if (i)
    return TRUE;

  if (k->keyval == GDK_space)
    {
      if (c->valid)
        {
          if (pop_window) 
            gtk_widget_destroy (pop_window);
	  if (c != month_view->has_popup) 
            {
              pop_window = day_popup (main_window, &c->popup, TRUE);
              month_view->has_popup = c;
            }
           else 
            {
              pop_window = NULL;
              month_view->has_popup = NULL;
            }
         }
      return TRUE;
    }  
     
  if (k->keyval == GDK_Return)
    {
      if (c->valid)
        {
	  time_t t = gtk_view_get_time (GTK_VIEW (month_view));
          struct tm tm;
          time_t selected_time;
          localtime_r (&t, &tm);
          tm.tm_year = c->popup.year;//- 1900;
          tm.tm_mon = c->popup.month;
          tm.tm_mday = c->popup.day;
          tm.tm_hour = 0;
          tm.tm_min = 0;
          tm.tm_sec = 0;
          selected_time = mktime (&tm);
          if (pop_window) 
	    gtk_widget_destroy (pop_window);
	  month_view->has_popup = NULL;
	  pop_window = NULL;
          set_time_and_day_view (selected_time);    
        }
      return TRUE; 
    }
  
  return FALSE;
}

GtkWidget *
gtk_month_view_new (time_t time)
{
  GtkMonthView *month_view;
  GtkWidget *scrolled_window;

  month_view = GTK_MONTH_VIEW (g_object_new (gtk_month_view_get_type (),
					     NULL));

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (month_view), scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled_window);
	  
  month_view->draw = gtk_drawing_area_new ();
  gtk_widget_show (month_view->draw);
  g_signal_connect (G_OBJECT (month_view->draw), "expose_event",
                    G_CALLBACK (draw_expose_event), month_view);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
					 month_view->draw);
  g_signal_connect (G_OBJECT (month_view->draw), "size-allocate",
                    G_CALLBACK (resize_table), month_view);
  g_signal_connect (G_OBJECT (month_view->draw), "button-press-event",
		    G_CALLBACK (button_press), month_view);
  gtk_widget_add_events (GTK_WIDGET (month_view->draw), 
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (month_view->draw), "key_press_event", 
		    G_CALLBACK (month_view_key_press_event), month_view);
  GTK_WIDGET_SET_FLAGS (month_view->draw, GTK_CAN_FOCUS);

  calc_title_height (month_view);

  gtk_widget_set_size_request (GTK_WIDGET(month_view->draw),
			       month_view->title_height * 7,
			       month_view->title_height * 7);

  gtk_view_set_time (GTK_VIEW (month_view), time);

  return GTK_WIDGET (month_view);
}
