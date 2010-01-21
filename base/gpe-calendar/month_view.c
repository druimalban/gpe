/*
   Copyright (C) 2001, 2002, 2003, 2004, 2005 Philip Blundell <philb@gnu.org>
   Copyright (C) 2006, 2007, 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2009, 2010 Graham R. Cobb <g+gpe@cobb.uk.net>
   
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

#include <time.h>
#include <langinfo.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include "pannedwindow.h"
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
   rows (as that is the maximum number of weeks any month requires
   independent of the day it starts on.

   Each cell has a render control associated with it.  This render
   control is marked as valid and has the date the cell represents and
   a pointer to the event list (if any).  */
struct _GtkMonthView
{
  GtkView widget;

  PannedWindow *panned_window;

  GdkWindow *title_area;
  GtkWidget *draw;
  GdkPixmap *draw_cache;
  guint draw_cache_expire;
  time_t draw_cache_last_access;

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

  /* If an event reload is pending.  */
  gboolean pending_reload;
  gboolean pending_update_extents;

  /* Called at midnight each day.  */
  guint day_changed;
};

static const int zoom_factors[] = { 4, 7, 12 };
#define ZOOM_FACTORS (sizeof (zoom_factors) / sizeof (zoom_factors[0]))

static void
draw_cache_destroy (GtkMonthView *month_view)
{
  if (month_view->draw_cache_expire)
    /* Remove the timer.  */
    {
      g_source_remove (month_view->draw_cache_expire);
      month_view->draw_cache_expire = 0;
    }

  if (month_view->draw_cache)
    /* Destroy the cache.  */
    {
      gdk_drawable_unref (month_view->draw_cache);
      month_view->draw_cache = NULL;
    }
}

static gboolean
draw_cache_expire (GtkMonthView *month_view)
{
  if (time (NULL) - month_view->draw_cache_last_access >= 60)
    /* The cache has not been accessed for at least a minute.  */
    draw_cache_destroy (month_view);

  return TRUE;
}

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
static void gtk_month_view_unrealize (GtkWidget *);
static void gtk_month_view_map (GtkWidget *);
static void gtk_month_view_unmap (GtkWidget *);
static gboolean gtk_month_view_expose (GtkWidget *widget,
				       GdkEventExpose *event);
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
  widget_class->unrealize = gtk_month_view_unrealize;
  widget_class->map = gtk_month_view_map;
  widget_class->unmap = gtk_month_view_unmap;
  widget_class->expose_event = gtk_month_view_expose;
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

  draw_cache_destroy (month_view);

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

  GdkWindowAttr attributes;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget)
    | GDK_BUTTON_MOTION_MASK
    | GDK_BUTTON_PRESS_MASK
    | GDK_BUTTON_RELEASE_MASK
    | GDK_EXPOSURE_MASK
    | GDK_ENTER_NOTIFY_MASK
    | GDK_LEAVE_NOTIFY_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.wclass = GDK_INPUT_OUTPUT;
      
  gint attributes_mask
    = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  month_view->title_area
    = gdk_window_new (gtk_widget_get_parent_window (widget),
		      &attributes, attributes_mask);
  gdk_window_set_user_data (month_view->title_area, month_view);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);
}

static void
gtk_month_view_unrealize (GtkWidget *widget)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (widget);

  gdk_window_set_user_data (month_view->title_area, NULL);
  gdk_window_destroy (month_view->title_area);
  month_view->title_area = NULL;

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static gboolean day_changed (GtkMonthView *month_view);

static void
setup_day_changed (GtkMonthView *month_view)
{
  if (month_view->day_changed)
    g_source_remove (month_view->day_changed);

  time_t now = time (NULL);

  GDate tomorrow;
  g_date_set_time_t (&tomorrow, now);
  g_date_add_days (&tomorrow, 1);
  struct tm tm;
  g_date_to_struct_tm (&tomorrow, &tm);

  month_view->day_changed = g_timeout_add ((mktime (&tm) - now + 1) * 1000,
					   (GSourceFunc) day_changed,
					   month_view);
}

static gboolean
day_changed (GtkMonthView *month_view)
{
  setup_day_changed (month_view);

  /* This is gratuitous if the current day was not shown and the
     current day will not be shown but it's relatively cheap so it
     shouldn't matter.  */
  draw_cache_destroy (month_view);
  gdk_window_invalidate_rect (GTK_WIDGET (month_view->draw)->window,
			      NULL, FALSE);

  return FALSE;
}

static void
gtk_month_view_map (GtkWidget *widget)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);
  /* Show the title area after the main window to make sure it is on
     top.  */
  gdk_window_show (month_view->title_area);

  setup_day_changed (month_view);
}

static void
gtk_month_view_unmap (GtkWidget *widget)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (widget);

  gdk_window_hide (month_view->title_area);

  if (month_view->day_changed)
    {
      g_source_remove (month_view->day_changed);
      month_view->day_changed = 0;
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

/* Get the bounding box of a cell.  Only return those dimensions for
   which a non-NULL location is provided.  */
static void
gtk_month_view_cell_box (GtkMonthView *month_view, int col, int row,
			 int *x, int *y, int *w, int *h,
			 GdkGC **color)
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

  if (color)
    {
      *color = gdk_gc_new (month_view->draw->window);
      gdk_gc_copy (*color, month_view->draw->style->black_gc);

      GDate today;
      g_date_set_time_t (&today, time (NULL));
      GdkColor c;
      if (g_date_compare (&today,
			  &month_view->day[row * 7 + col].date) == 0)
	/* Today, "lemon chiffon".  */
	{
	  c.red = 255 << 8;
	  c.green = 250 << 8;
	  c.blue = 205 << 8;
	}
      else if ((week_starts_sunday && (col == 0 || col == 6))
	       || (! week_starts_sunday && col >= 5))
	/* Weekend, "light salmon".  */
	{
	  c.red = 255 << 8;
	  c.green = 160 << 8;
	  c.blue = 122 << 8;
	}
      else
	/* Weekday, "palegoldenrod".  */
	{
	  c.red = 238 << 8;
	  c.green = 232 << 8;
	  c.blue = 170 << 8;
	}

      GdkColormap *colormap
	= gdk_window_get_colormap (month_view->draw->window);
      gdk_colormap_alloc_color (colormap, &c, FALSE, TRUE);
      gdk_gc_set_foreground (*color, &c);
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

/* Draw the focused cell.  If OLD is not -1, then erases the
   focus. around the cell OLD.  */
static void
gtk_month_view_draw_focus (GtkMonthView *month_view, int old)
{
  g_assert (old < month_view->weeks * 7);

  if (month_view->draw_cache)
    {
      int x, y, w, h;

      if (old != -1)
	/* Erase the old focus area.  */
	{
	  GdkGC *bg;
	  gtk_month_view_cell_box (month_view, old % 7, old / 7,
				   &x, &y, &w, &h, &bg);
	  gdk_draw_rectangle (month_view->draw_cache, bg, FALSE,
			      x, y + 1, w - 2, h - 2);
	}

      /* Draw the new focus area.  */
      gtk_month_view_cell_box (month_view,
			       month_view->focused_day % 7,
			       month_view->focused_day / 7,
			       &x, &y, &w, &h, NULL);
      GdkGC *blue_gc = pen_new (month_view->draw, 0, 0, 0xffff);
      gdk_draw_rectangle (month_view->draw_cache, blue_gc, FALSE,
			  x, y + 1, w - 2, h - 2);
      gdk_gc_unref (blue_gc);
    }

  gdk_window_invalidate_rect (GTK_WIDGET (month_view->draw)->window,
			      NULL, FALSE);
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
      if (! adj)
	return;

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
  gtk_month_view_cell_box (month_view, col, row, &x, &y, &w, &h, NULL);

  GtkWidget *viewport
    = GTK_BIN (GTK_BIN (month_view->panned_window)->child)->child;

  gdk_window_freeze_updates (month_view->draw->window);

  GtkScrolledWindow *scrolled_window
    = GTK_SCROLLED_WINDOW (GTK_BIN (month_view->panned_window)->child);
  do_scroll (gtk_scrolled_window_get_hadjustment (scrolled_window), x, w,
	     month_view->width, viewport->allocation.width);
  do_scroll (gtk_scrolled_window_get_vadjustment (scrolled_window), y, h,
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
  if ((! panned_window_is_panning (month_view->panned_window)
       && 0 <= diff && diff < 7 * month_view->weeks)
      || (panned_window_is_panning (month_view->panned_window)
	  && g_date_get_month (&current) == g_date_get_month (&new)))
    /* Same month.  */
    {
      if (diff != month_view->focused_day)
	/* But different day.  */
	{
	  int old = month_view->focused_day;
	  month_view->focused_day = diff;
	  gtk_month_view_draw_focus (month_view, old);
	  if (! panned_window_is_panning (month_view->panned_window))
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

  if (event->button == 3)
    /* Right click.  Popup a menu.  */
    {
      gtk_view_set_date (GTK_VIEW (month_view), &c->date); 

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
  g_assert (! month_view->pending_reload);

  while (month_view->pending_update_extents)
    {
      month_view->pending_update_extents = FALSE;

      /* The size of the canvas is the size of the viewport with a
	 particular magnification.  */
      GtkWidget *viewport
	= GTK_BIN (GTK_BIN (month_view->panned_window)->child)->child;
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
	  draw_cache_destroy (month_view);

	  /* Process any size changes now.  The size renegotiation may
	     reset MONTH_VIEW->PENDING_UPDATE_EXTENTS.  Additionally,
	     we need to let the resize propagate first or the scroll
	     will not work correctly.  */
	  g_main_context_iteration (NULL, FALSE);

	  if (! panned_window_is_panning (month_view->panned_window))
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
  gdk_window_invalidate_rect (month_view->title_area,
			      NULL, FALSE);
}

static void
render (GtkMonthView *month_view)
{
  /* Suppress any expose events.  */
  gdk_window_freeze_updates (month_view->draw->window);

  if (month_view->pending_reload)
    reload_events_hard (month_view);
  if (month_view->pending_update_extents)
    update_extents_hard (month_view);

  /* Trigger any pending expose events.  If there were any, then just
     return.  */
  gdk_window_thaw_updates (month_view->draw->window);

  GtkWidget *widget = GTK_WIDGET (month_view);

  month_view->draw_cache_last_access = time (NULL);

  GdkDrawable *drawable = month_view->draw_cache;
  if (drawable)
    /* We already have a cached copy of the area.  */
    return;

  if (! month_view->draw_cache_expire)
    /* Start checking if the cache needs to be expired.  */
    month_view->draw_cache_expire
      = g_timeout_add (60 * 1000, (GSourceFunc) draw_cache_expire,
		       month_view);

  drawable = gdk_pixmap_new (widget->window,
			     month_view->width, month_view->height, -1);
  month_view->draw_cache = drawable;

  GdkRectangle area;
  area.x = 0;
  area.y = 0;
  area.width = month_view->width;
  area.height = month_view->height;

  GdkGC *light_gray_gc = pen_new (widget, 53040, 53040, 53040);
  GdkGC *black_gc = widget->style->black_gc;

  PangoLayout *pl_evt
    = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);

  /* Scale the font appropriately.  */
  const PangoFontDescription *orig_font
    = pango_layout_get_font_description (pl_evt);
  if (! orig_font)
    {
      PangoContext *context = pango_layout_get_context (pl_evt);
      orig_font = pango_context_get_font_description (context);
    }
  PangoFontDescription *font = pango_font_description_copy (orig_font);
  int s = CLAMP (month_view->width / 7 / 14, 8, 14) * PANGO_SCALE;
  pango_font_description_set_size (font, s);
  pango_layout_set_font_description (pl_evt, font);

  /* Set day of week headings font to black */
  PangoAttrList *attrs = pango_attr_list_new ();
  PangoAttribute *black = pango_attr_foreground_new (0,0,0);
  black->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
  black->end_index = PANGO_ATTR_INDEX_TO_TEXT_END;
  pango_attr_list_insert (attrs, black);
  black = NULL;
  pango_layout_set_attributes (pl_evt, attrs);
  pango_attr_list_unref(attrs);
  attrs = NULL;

  gdk_draw_rectangle (drawable, light_gray_gc, 1,
		      0, 0, month_view->width, month_view->title_height);
  gdk_draw_line (drawable, black_gc, 0, month_view->title_height - 1,
		 month_view->width, month_view->title_height - 1);
  

  /* Draw the columns' titles.  */

  /* 0, 7 are Sunday.  */
  const nl_item days_of_week[] = 
  { ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, ABDAY_1 };
  const nl_item *days = &days_of_week[week_starts_sunday ? 0 : 1];
  PangoLayout *pl = gtk_widget_create_pango_layout (widget, NULL);

  /* Set event text font to black */
  attrs = pango_attr_list_new ();
  black = pango_attr_foreground_new (0,0,0);
  black->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
  black->end_index = PANGO_ATTR_INDEX_TO_TEXT_END;
  pango_attr_list_insert (attrs, black);
  black = NULL;
  pango_layout_set_attributes (pl, attrs);
  pango_attr_list_unref(attrs);
  attrs = NULL;

  gint col;
  for (col = 0; col < 7; col ++)
    {
      gchar *s = g_locale_to_utf8 (nl_langinfo (days[col]), -1,
				   NULL, NULL, NULL);
      pango_layout_set_text (pl, s, -1);
      g_free (s);

      PangoRectangle pr;
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      
      gint x, w;
      gtk_month_view_cell_box (month_view, col, 0,
			       &x, NULL, &w, NULL, NULL);

      gtk_paint_layout (widget->style,
			drawable,
			GTK_WIDGET_STATE (widget),
			FALSE,
			&area,
			widget,
			"label",
			x + (w - pr.width) / 2, 1,
			pl);
    }
  g_object_unref (pl);

  /* Paint the cells and events.  */
  gint row;
  for (row = 0; row < month_view->weeks; row ++)
    {
      gint y, h;

      gtk_month_view_cell_box (month_view, 0, row,
			       NULL, &y, NULL, &h, NULL);

      for (col = 0; col < 7; col ++)
	{
	  gint day = col + (7 * row);
	  struct day *c = &month_view->day[day];
	  gint x, w;

	  GdkGC *bg;
	  gtk_month_view_cell_box (month_view, col, row,
				   &x, NULL, &w, NULL, &bg);

	  pango_layout_set_width (pl_evt, w * PANGO_SCALE);
	  int trying_no_wrap = FALSE;
	restart:
	  gdk_draw_rectangle (drawable, bg, TRUE, x, y + 1, w, h);

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
				  "%d", g_date_get_day (&c->date));

	  pango_layout_set_text (pl_evt, buffer, written);
	  gtk_paint_layout (widget->style,
			    drawable,
			    GTK_WIDGET_STATE (widget),
			    FALSE,
			    &area,
			    widget,
			    "label",
			    left, top,
			    pl_evt);

	  PangoRectangle pr;
	  pango_layout_get_pixel_extents (pl_evt, NULL, &pr);
	  left += pr.width + 3;

	  GSList *iter;
	  for (iter = c->events; iter; iter = iter->next)
	    {
	      Event *ev = iter->data;
	      if (! event_get_visible (ev, NULL))
		continue;

	      char *time = NULL;
	      if (! event_get_untimed (ev))
		{
		  time_t s = event_get_start (ev);
		  GDate start;
		  g_date_set_time_t (&start, s);

		  if (g_date_compare (&start, &c->date) == 0)
		    {
		      struct tm tm;
		      localtime_r (&s, &tm);
		      time = strftime_strdup_utf8_locale (_("%-H:%M"), &tm);
		    }
		}

	      char *text;
	      char *summary = event_get_summary (ev, NULL);
	      if (time)
		{
		  text = g_strdup_printf ("%s %s", time, summary ?: "");
		  g_free (summary);
		  g_free (time);
		}
	      else
		text = summary;

	      /* Replace '\n''s with spaces.  */
	      char *t;
	      for (t = strchr (text, '\n'); t; t = strchr (t, '\n'))
		*t = ' ';

	      pango_layout_set_text (pl_evt, text, -1);
	      g_free (text);

	      pango_layout_get_pixel_extents (pl_evt, NULL, &pr);
	      if (top + pr.height > y + h && ! trying_no_wrap)
		{
		  trying_no_wrap = TRUE;
		  pango_layout_set_width (pl_evt, -1);
		  goto restart;
		}

	      GdkColor color;
	      if (event_get_color (ev, &color, NULL))
		{
		  GdkGC *color_gc;

		  color_gc = gdk_gc_new (widget->window);
		  gdk_gc_copy (color_gc, widget->style->black_gc);

		  gdk_colormap_alloc_color
		    (gdk_window_get_colormap (widget->window), &color,
		     FALSE, TRUE);
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
				drawable,
				GTK_WIDGET_STATE (widget),
				FALSE,
				&area,
				widget,
				"label",
				left + 1, top,
				pl_evt);

	      top += pr.height + 2;
	      left = x + 1;
	    }

	  gdk_gc_unref (bg);
	}
    }

  gtk_month_view_draw_focus (month_view, -1);

  gdk_gc_unref (light_gray_gc);

  g_object_unref (pl_evt);
}

static gboolean
gtk_month_view_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GtkMonthView *month_view = GTK_MONTH_VIEW (widget);

  if (event->window == month_view->title_area)
    {
      render (month_view);

      GtkWidget *viewport
	= GTK_BIN (GTK_BIN (month_view->panned_window)->child)->child;
      int x, y;
      gtk_widget_translate_coordinates (viewport, month_view->draw,
					0, 0, &x, &y);

      gdk_draw_drawable (month_view->title_area, widget->style->black_gc,
			 month_view->draw_cache,
			 x, 0,
			 0, 0,
			 month_view->draw->allocation.width,
			 month_view->title_height);

      return FALSE;
    }
  else
    return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}

static gboolean
draw_expose_event (GtkWidget *widget, GdkEventExpose *event,
		   GtkMonthView *month_view)
{
  if (event->count > 0)
    return FALSE;

  render (month_view);

  GtkScrolledWindow *scrolled_window
    = GTK_SCROLLED_WINDOW (GTK_BIN (month_view->panned_window)->child);
  GtkWidget *viewport = GTK_BIN (scrolled_window)->child;
  int x, y;
  gtk_widget_translate_coordinates (viewport, month_view->draw, 0, 0, &x, &y);

  gdk_draw_drawable (widget->window,
		     widget->style->black_gc, month_view->draw_cache,
		     event->area.x, MAX (y + month_view->title_height,
					 event->area.y),
		     event->area.x, MAX (y + month_view->title_height,
					 event->area.y),
		     event->area.width, event->area.height);

  return TRUE;
}

static void
hadjustment_value_changed (GtkAdjustment *adj, GtkMonthView *month_view)
{
  gdk_window_invalidate_rect (month_view->title_area, NULL, FALSE);
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
  /* Destroy outstanding events.  */
  int i;
  for (i = 0; i < MAX_DAYS; i ++)
    if (month_view->day[i].events)
      {
	/* Destroy outstanding events.  */
	event_list_unref (month_view->day[i].events);
	month_view->day[i].events = NULL;
      }

  GDate focused_day;
  gtk_view_get_date (GTK_VIEW (month_view), &focused_day);

  /* 0 => Monday.  */
  GDate period_start = focused_day;
  g_date_set_day (&period_start, 1);
  first_day (&period_start, &month_view->weeks);

  GDate period_end = period_start;
  /* The day following the last day.  */
  g_date_add_days (&period_end, month_view->weeks * 7);

  /* Initialize the days.  */
  GDate d;
  for (i = 0, d = period_start;
       i < month_view->weeks * 7;
       i ++, g_date_add_days (&d, 1))
    month_view->day[i].date = d;

  month_view->focused_day
    = g_date_days_between (&period_start, &focused_day);

  /* Get the events for the period.  */
  struct tm start_tm;
  g_date_to_struct_tm (&period_start, &start_tm);
  struct tm end_tm;
  g_date_to_struct_tm (&period_end, &end_tm);

  GSList *events
    = event_db_list_for_period (event_db,
				mktime (&start_tm), mktime (&end_tm) - 1,
				NULL);

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

      time_t e = s + event_get_duration (ev) - 1;
      GDate end;
      if (event_get_untimed (ev))
	{
	  end = start;

	  int days = event_get_duration (ev) / (24 * 60 * 60);
	  if (days > 0)
	    days --;
	  g_date_add_days (&end, days);
	}
      else
	{
	  g_date_set_time_t (&end, e);
	}
      g_date_clamp (&start, &period_start, &period_end);
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

  draw_cache_destroy (month_view);
  gdk_window_invalidate_rect (GTK_WIDGET (month_view->draw)->window,
			      NULL, FALSE);
  gdk_window_invalidate_rect (month_view->title_area,
			      NULL, FALSE);
}

static void
viewport_size_allocate (GtkWidget *widget, GtkAllocation *allocation,
			GtkMonthView *month_view)
{
  month_view->pending_update_extents = TRUE;

  if (! month_view->pending_reload)
    /* We don't call update_extents as it will invalidate the draw
       area and we don't actually know if the size has even
       changed.  */
    update_extents_hard (month_view);

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (month_view->title_area,
			    GTK_WIDGET (month_view)->allocation.x,
			    GTK_WIDGET (month_view)->allocation.y,
			    allocation->width,
			    month_view->title_height);
}

static void
edge_flip (GtkWidget *widget, enum PannedWindowEdge edge,
	   GtkMonthView *month_view)
{
  GDate date;
  gtk_view_get_date (GTK_VIEW (month_view), &date);

  if (edge == PANNED_NORTH || edge == PANNED_NORTH_WEST)
    {
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
      scroll_to_focused_day (month_view);
    }
  else if (edge == PANNED_WEST)
    {
      g_date_set_day (&date, 1);
      g_date_subtract_months (&date, 1);
      int weeks;
      first_day (&date, &weeks);
      g_date_add_days (&date,
		       MIN (month_view->focused_day / 7 * 7 + 6,
			    weeks * 7 - 1));

      gtk_view_set_date (GTK_VIEW (month_view), &date);
      scroll_to_focused_day (month_view);
    }
  else if (edge == PANNED_SOUTH || edge == PANNED_SOUTH_EAST)
    {
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
      scroll_to_focused_day (month_view);
    }
  else if (edge == PANNED_EAST)
    {
      g_date_set_day (&date, 1);
      g_date_add_months (&date, 1);
      int weeks;
      first_day (&date, &weeks);
      g_date_add_days (&date,
		       MIN (month_view->focused_day / 7 * 7,
			    weeks * 7 - 1));
    }
  else
    return;

  gtk_view_set_date (GTK_VIEW (month_view), &date);
  scroll_to_focused_day (month_view);
}

static gboolean
button_release_event (GtkWidget *widget, GdkEventButton *event,
		      GtkMonthView *month_view)
{
  int col, row;
  gtk_month_view_cell_at (month_view, event->x, event->y, &col, &row);

  int day = col + row * 7;
  if (day < 0 || day >= 7 * month_view->weeks)
    return FALSE;

  if (event->button == 1
      && ! panned_window_is_panning (month_view->panned_window))
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
	/* The user clicked on the focused day.  Zoom to that day.  */
	set_time_and_day_view (mktime (&tm));
      else
	gtk_view_set_time (GTK_VIEW (month_view), mktime (&tm));

      return TRUE;
    }

  return FALSE;
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

  month_view = GTK_MONTH_VIEW (g_object_new (gtk_month_view_get_type (),
					     NULL));
  GTK_WIDGET_SET_FLAGS (month_view, GTK_CAN_FOCUS);
  gtk_widget_add_events (GTK_WIDGET (month_view), GDK_KEY_PRESS_MASK);

  GtkWidget *panned_window = panned_window_new ();
  month_view->panned_window = PANNED_WINDOW (panned_window);
  g_signal_connect (G_OBJECT (month_view->panned_window), "edge-flip",
		    G_CALLBACK (edge_flip), month_view);
  gtk_box_pack_start (GTK_BOX (month_view), panned_window, TRUE, TRUE, 0);
  gtk_widget_show (panned_window);

  GtkScrolledWindow *scrolled_window
    = GTK_SCROLLED_WINDOW (GTK_BIN (panned_window)->child);
  gtk_scrolled_window_set_policy (scrolled_window,
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  GtkAdjustment *hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0));
  gtk_scrolled_window_set_hadjustment (scrolled_window, hadj);
  g_signal_connect (G_OBJECT (hadj), "value-changed",
		    G_CALLBACK (hadjustment_value_changed), month_view);
	  
  month_view->draw = gtk_drawing_area_new ();
  gtk_widget_set_app_paintable (month_view->draw, TRUE);
  gtk_widget_add_events (GTK_WIDGET (month_view->draw),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (G_OBJECT (month_view->draw), "button-press-event",
		    G_CALLBACK (button_press_event), month_view);
  g_signal_connect (G_OBJECT (month_view->draw), "expose_event",
		    G_CALLBACK (draw_expose_event), month_view);
  g_signal_connect (G_OBJECT (month_view->draw), "button-release-event",
		    G_CALLBACK (button_release_event), month_view);
  gtk_scrolled_window_add_with_viewport (scrolled_window,
					 month_view->draw);
  gtk_widget_show (month_view->draw);

  GtkWidget *viewport = GTK_BIN (GTK_BIN (panned_window)->child)->child;
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
  g_signal_connect (G_OBJECT (viewport), "size-allocate",
		    G_CALLBACK (viewport_size_allocate), month_view);

  gtk_view_set_time (GTK_VIEW (month_view), time);

  return GTK_WIDGET (month_view);
}
