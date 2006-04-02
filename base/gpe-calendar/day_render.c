/*
 *  Copyright (C) 2004 Luca De Cicco <ldecicco@gmx.net> 
 *  Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#include <gpe/event-db.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <langinfo.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gpe/pixmaps.h>

#include "day_render.h"
#include "day_view.h"
#include "globals.h"

static GNode *find_overlapping_sets (GSList * events);
static GSList *ol_sets_to_rectangles (GtkDayRender *dr, GNode * node);

#define event_ends(event) ((event)->start + (event)->duration)

static GdkPixbuf *bell_pb;
static GSList *found_node;

struct event_rect
{
  gfloat width;
  gfloat height;
  gfloat x;
  gfloat y;
  event_t event;
};

/* Create a new event rectangle.  */
static struct event_rect *event_rect_new (GtkDayRender *day_render,
					  const event_t event,
					  gint column, gint columns);

/* Delete an event rectangle returned by event_rect_new.  */
static void event_rect_delete (struct event_rect *ev);

static void gtk_day_render_update_extents (GtkDayRender *day_render);

struct _GtkDayRender
{
  GtkDrawingArea drawing_area;

  GdkGC *normal_gc;		/* Normal event color.  */
  gint gap;			/* Gap between event boxes. */

  /* Start of period to display */
  time_t date;
  /* The length of the period (in seconds).  */
  gint duration;

  /* Events to display.  */
  GSList *events;

  /* The layout.  */
  GSList *event_rectangles;
  /* The time of the earliest event to display.  */
  time_t event_earliest;
  /* The time of the latest event to display.  */
  time_t event_latest;

  /* The height and width of the display window.  */
  gint visible_width;
  gint visible_height;

  /* Event rectangle positions are expressed in fractions of a canvas
     displaying all possible rows.  VISIBLE_WIDTH and VISIBLE_HEIGHT
     account for the visible rows.  Multiplying an event rectangle (Y)
     coordinate by HEIGHT and adding the OFFSET_Y yields the display
     coordinate.  _*/
  gint height;
  gint offset_y;

  /* Whether to draw an hour column or not.  */
  gboolean hour_column;

  /* Cache of gray.  */
  GdkGC *gray_gc;

  /* The width of the time column.  */
  gint time_width;

  /* Number of rows.  */
  gint rows;

  /* First row which must be displayed.  */
  gint rows_hard_first;
  /* Number of rows which must be displayed (starting with
     ROWS_HARD_FIRST).  */
  gint rows_hard;

  /* Given the current set of events, the first row to display and the
     number of rows.  */
  gint rows_visible_first;
  gint rows_visible;

  /* The height of a row in pixels.  */
  gint row_height;
};

typedef struct
{
  GtkDrawingAreaClass drawing_area_class;
  GtkWidgetClass parent_class;

  void (* event_clicked) (GtkDayRender *day_render);
  void (* row_clicked) (GtkDayRender *day_render);

  guint event_click_signal;
  guint row_click_signal;
} GtkDayRenderClass;

static void gtk_day_render_base_class_init (gpointer class);
static void gtk_day_render_init (GTypeInstance *instance, gpointer klass);
static void gtk_day_render_dispose (GObject *object);
static void gtk_day_render_finalize (GObject *object);

static gboolean gtk_day_render_expose (GtkWidget *widget,
				       GdkEventExpose *event);
static gboolean gtk_day_render_configure (GtkWidget *widget,
					  GdkEventConfigure *event);
static gboolean gtk_day_render_button_press (GtkWidget *widget,
					     GdkEventButton *event);

static GtkWidgetClass *parent_class;

GType
gtk_day_render_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo day_render_info =
      {
	sizeof (GtkDayRenderClass),
	gtk_day_render_base_class_init,
	NULL,
	NULL,
	NULL,
	NULL,
	sizeof (struct _GtkDayRender),
	0,
	gtk_day_render_init,
      };

      type = g_type_register_static (gtk_drawing_area_get_type (),
				     "DayRender", &day_render_info, 0);
    }

  return type;
}

static void
gtk_day_render_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkDayRenderClass *render_class;

  parent_class = g_type_class_ref (gtk_drawing_area_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_day_render_finalize;
  object_class->dispose = gtk_day_render_dispose;

  widget_class = (GtkWidgetClass *) klass;
  widget_class->expose_event = gtk_day_render_expose;
  widget_class->configure_event = gtk_day_render_configure;
  widget_class->button_press_event = gtk_day_render_button_press;

  render_class = (GtkDayRenderClass *) klass;
  render_class->event_click_signal
    = g_signal_new ("event-clicked",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (GtkDayRenderClass, event_clicked),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
  render_class->row_click_signal
    = g_signal_new ("row-clicked",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (GtkDayRenderClass, row_clicked),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__UINT,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_UINT);
}

static void
gtk_day_render_init (GTypeInstance *instance, gpointer klass)
{
  GtkDayRender *day_render = GTK_DAY_RENDER (instance);

  day_render->gray_gc = 0;
  day_render->visible_width = 0;
  day_render->visible_height = 0;
}

GtkWidget *
gtk_day_render_new (time_t date, gint duration, gint rows,
		    gint rows_hard_first,
		    gint rows_hard,
		    GdkGC *app_gc, gint gap,
		    gboolean hour_column, 
		    GSList *events)
{
  GtkWidget *widget;
  GtkDayRender *day_render;

  g_return_val_if_fail (duration > 0, NULL);
  g_return_val_if_fail (rows > 0, NULL);
  g_return_val_if_fail (rows_hard_first >= 0, NULL);
  g_return_val_if_fail (rows_hard_first < rows, NULL);
  g_return_val_if_fail (rows_hard >= 0, NULL);
  g_return_val_if_fail (rows_hard_first + rows_hard <= rows, NULL);

  widget = gtk_type_new (gtk_day_render_get_type ());
  day_render = GTK_DAY_RENDER (widget);

  day_render->date = date;
  day_render->duration = duration;
  day_render->rows = rows;
  day_render->rows_hard_first = rows_hard_first;
  day_render->rows_hard = rows_hard;
  day_render->normal_gc = app_gc;
  g_object_ref (day_render->normal_gc);
  day_render->gap = gap;
  day_render->hour_column = hour_column;
  gtk_day_render_set_events (day_render, events);

  gtk_day_render_update_extents (day_render);

  gtk_widget_add_events (widget,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  return widget;
}

static void
gtk_day_render_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_day_render_finalize (GObject *object)
{
  GtkDayRender *day_render;
  GSList *er;

  g_return_if_fail (object);
  g_return_if_fail (GTK_IS_DAY_RENDER (object));

  day_render = (GtkDayRender *) object;

  g_object_unref (day_render->normal_gc);

  for (er = day_render->event_rectangles; er; er = er->next)
    event_rect_delete ((struct event_rect *) er->data);
  g_slist_free (day_render->event_rectangles);

  event_db_list_destroy (day_render->events);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gtk_day_render_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GtkDayRender *day_render;
  GdkGC *white_gc, *black_gc;
  PangoLayout *pl;
  gint i;
  gint top;
  GSList *er;

  g_return_val_if_fail (widget, FALSE);
  g_return_val_if_fail (GTK_IS_DAY_RENDER (widget), FALSE);
  g_return_val_if_fail (event, FALSE);

  if (event->count > 0)
    return FALSE;

  day_render = GTK_DAY_RENDER (widget);

  if (day_render->rows_visible == 0)
    /* Nothing to display.  */
    return FALSE;

  white_gc = widget->style->white_gc;
  black_gc = widget->style->black_gc;

  pl = gtk_widget_create_pango_layout (widget, NULL);
  
  if (! day_render->gray_gc)
    day_render->gray_gc = pen_new (widget, 58905, 58905, 56610);

  /* Start with a white background.  */
  gdk_draw_rectangle (widget->window, white_gc, TRUE,
		      0, 0,
		      day_render->visible_width - 1,
		      day_render->visible_height - 1);
  if (day_render->hour_column)
    /* Draw the hour column's background.  */
    gdk_draw_rectangle (widget->window, day_render->gray_gc, TRUE,
			0, 0,
			day_render->time_width,
			day_render->visible_height - 1);

  /* And now each row's background.  */
  for (i = day_render->rows_visible_first;
       i <= day_render->rows_visible_first + day_render->rows_visible;
       i ++)
    {
      top = i * day_render->height / day_render->rows - day_render->offset_y;

      /* Draw a gray line to separating each row.  */
      gdk_draw_line (widget->window, day_render->gray_gc,
		     day_render->time_width, top,
		     day_render->visible_width - 1, top);

      if (day_render->hour_column)
	{
	  time_t tm;
	  struct tm ftm;
	  char timebuf[10];
	  char buf[60], *buffer;
	  GdkRectangle gr;
	  PangoRectangle pr;

	  gdk_draw_line (widget->window, white_gc,
			 0, top, day_render->time_width - 1, top);

	  tm = day_render->date
	    + i * ((gfloat) day_render->duration / day_render->rows);
	  localtime_r (&tm, &ftm);
	  strftime (timebuf, sizeof (timebuf), TIMEFMT, &ftm);
	  snprintf (buf, sizeof (buf), "<span font_desc='normal'>%s</span>",
		    timebuf);
	  buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	  pango_layout_set_markup (pl, buffer, strlen (buffer));
	  pango_layout_get_pixel_extents (pl, &pr, NULL);

	  gr.width = pr.width * 2;
	  gr.height = pr.height * 2;
	  gr.x = 1;
	  gr.y = top;

	  gtk_paint_layout (widget->style,
			    widget->window,
			    GTK_WIDGET_STATE (widget),
			    FALSE, &gr, widget, "label", gr.x, gr.y, pl);

	  g_free (buffer);
	}
    }

  if (day_render->hour_column)
    /* Draw the black vertical line dividing the hour column from the
       events.  */
    gdk_draw_line (widget->window, black_gc,
		   day_render->time_width, 0,
		   day_render->time_width, day_render->visible_height - 1);

  /* Now, draw the events.  */
  for (er = day_render->event_rectangles; er; er = er->next)
    {
      struct event_rect *event_rectangle = er->data;
      GdkDrawable *w;
      event_details_t evd;
      gint x, y, width, height;
      GdkRectangle gr;
      gchar *buffer;
      gint arc_size;
      PangoRectangle pr;

      w = GTK_WIDGET (day_render)->window;

      evd = event_db_get_details (event_rectangle->event);
      buffer = g_strdup_printf ("<span size='small'>%s</span>", evd->summary);

      /* Rectangle used to write appointment summary */
      x = day_render->time_width
	+ (gint) ((day_render->visible_width - day_render->time_width)
		  * event_rectangle->x)
	+ day_render->gap;
      y = (gint) (day_render->height * event_rectangle->y)
	+ 1 - day_render->offset_y;
      width = (gint) ((day_render->visible_width - day_render->time_width)
		      * event_rectangle->width) - 2 * day_render->gap - 1;
      height = (gint) (day_render->height * event_rectangle->height) - 1;

      gr.width = width - 1;
      gr.height = height;
      gr.x = x + 2;
      gr.y = y + 1;

      pango_layout_set_markup (pl, buffer, strlen (buffer));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      pango_layout_set_width (pl, PANGO_SCALE * gr.width);
      pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);

      arc_size = day_render->row_height / 2 - 1;
      if (arc_size < 7)
	/* Lower bound */
	arc_size = 7;

      /* "Flood" the rectangle.  */
      if (height > 2 * arc_size)
	gdk_draw_rectangle (w, day_render->normal_gc, TRUE,
			    x, y + arc_size, width, height - 2 * arc_size);
      if (width > 2 * arc_size)
	gdk_draw_rectangle (w, day_render->normal_gc, TRUE,
			    x + arc_size, y, width - 2 * arc_size, height);

      /* Draw the outline.  */

      /* Top.  */
      gdk_draw_line (w, GTK_WIDGET (day_render)->style->black_gc,
		     x + arc_size, y, x + width - 1 - arc_size, y);
      /* Bottom.  */
      gdk_draw_line (w, GTK_WIDGET (day_render)->style->black_gc,
		     x + arc_size, y + height - 1,
		     x + width - 1 - arc_size, y + height - 1);
      /* Left.  */
      gdk_draw_line (w, GTK_WIDGET (day_render)->style->black_gc,
		     x, arc_size + y, x, y + height - 1 - arc_size);
      /* Right.  */
      gdk_draw_line (w, GTK_WIDGET (day_render)->style->black_gc,
		     x + width - 1, arc_size + y,
		     x + width - 1, y + height - 1 - arc_size);

      /* Draw the corners.  */

      /* North-west corner */
      gdk_draw_arc (w, day_render->normal_gc, TRUE,
		    x, y, arc_size * 2, arc_size * 2, 90 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x, y, arc_size * 2, arc_size * 2, 90 * 64, 90 * 64);
      /* North-east corner */
      gdk_draw_arc (w, day_render->normal_gc, TRUE,
		    x + width - 1 - 2 * arc_size, y,
		    arc_size * 2, arc_size * 2, 0 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x + width - 1 - arc_size * 2, y,
		    arc_size * 2, arc_size * 2, 0 * 64, 90 * 64);
      /* South-west corner */
      gdk_draw_arc (w, day_render->normal_gc, TRUE,
		    x, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 180 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 180 * 64, 90 * 64);
      /* South-east corner */
      gdk_draw_arc (w, day_render->normal_gc, TRUE,
		    x + width - 1 - 2 * arc_size, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 270 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x + width - 1 - 2 * arc_size, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 270 * 64, 90 * 64);	

      /* Write summary... */
      gtk_paint_layout (GTK_WIDGET (day_render)->style, w,
			GTK_WIDGET_STATE (day_render),
			FALSE, &gr, GTK_WIDGET (day_render), "label",
			gr.x, gr.y, pl);

      if (event_rectangle->event->flags & FLAG_ALARM)
	{
	  if (! bell_pb)
	    /* Load the icon.  */
	    bell_pb = gpe_find_icon ("bell");
	  if (bell_pb)
	    {
	      gint width_pix, height_pix;
	      width_pix = gdk_pixbuf_get_width (bell_pb);
	      height_pix = gdk_pixbuf_get_height (bell_pb);
    
	      gdk_draw_pixbuf (w,
			       day_render->normal_gc,
			       bell_pb,
			       0, 0,
			       x + width - width_pix - 1,
			       y + 1, MIN (width_pix, width - 1),
			       MIN (height_pix, height - 1),
			       GDK_RGB_DITHER_NORMAL, 0, 0);
	    }
	}

      g_free (buffer);
    }

  g_object_unref (pl);

  return FALSE;
}

static gboolean
gtk_day_render_configure (GtkWidget *widget, GdkEventConfigure *event)
{
  GtkDayRender *day_render = GTK_DAY_RENDER (widget);
  gboolean update = FALSE;

  if (day_render->visible_width != event->width)
    {
      day_render->visible_width = event->width;
      update = TRUE;
    }

  if (day_render->visible_height != event->height)
    {
      day_render->visible_height = event->height;
      update = TRUE;
    }

  if (update)
    {
      gtk_widget_queue_draw (GTK_WIDGET (day_render));
      gtk_day_render_update_extents (day_render);
    }

  return FALSE;
}

static gboolean
gtk_day_render_button_press (GtkWidget *widget, GdkEventButton *event)
{
  GtkDayRender *day_render = GTK_DAY_RENDER (widget);
  gfloat x = event->x / day_render->visible_width;
  gfloat y = (event->y + day_render->offset_y) / day_render->height;
  GSList *er;
  GtkDayRenderClass *drclass
    = (GtkDayRenderClass *) G_OBJECT_GET_CLASS (day_render);

  /* search for an event rectangle */
  for (er = day_render->event_rectangles; er; er = er->next)
    {
      struct event_rect *e_r = er->data;
      if ((x >= e_r->x && x <= e_r->x + e_r->width)
	  && (y >= e_r->y && y <= e_r->y + e_r->height))
	/* Click was within E_R.  Send the appropriate signal.  */
	{
	  GValue args[2];
	  GValue rv;

	  args[0].g_type = 0;
	  g_value_init (&args[0], G_TYPE_FROM_INSTANCE (G_OBJECT (widget)));
	  g_value_set_instance (&args[0], widget);
        
	  args[1].g_type = 0;
	  g_value_init (&args[1], G_TYPE_POINTER);
	  g_value_set_pointer (&args[1], e_r->event);

	  g_signal_emitv (args, drclass->event_click_signal, 0, &rv);
	  break;
	}
    }

  if (! er)
    /* Click was not within an event rectangle send a canvas click
       event.  */
    {
      GValue args[2];
      GValue rv;

      args[0].g_type = 0;
      g_value_init (&args[0], G_TYPE_FROM_INSTANCE (G_OBJECT (widget)));
      g_value_set_instance (&args[0], widget);
        
      args[1].g_type = 0;
      g_value_init (&args[1], G_TYPE_INT);
      g_value_set_int (&args[1], (gint) (y * day_render->rows));

      g_signal_emitv (args, drclass->row_click_signal, 0, &rv);
    }

  return FALSE;
}

void
gtk_day_render_set_events (GtkDayRender *day_render, GSList *events)
{
  g_slist_free (day_render->events);
  day_render->events = events;

  day_render->event_earliest = day_render->date + day_render->duration;
  day_render->event_latest = day_render->date;

  day_render->event_rectangles
    = ol_sets_to_rectangles (day_render,
			     find_overlapping_sets (day_render->events));

  gtk_day_render_update_extents (day_render);
  gtk_widget_queue_draw (GTK_WIDGET (day_render));
}

void
gtk_day_render_set_date (GtkDayRender *day_render, time_t date)
{
  if (day_render->date != date)
    {
      day_render->date = date;
      gtk_widget_queue_draw (GTK_WIDGET (day_render));
    }
}

/* Update the calculated extents (DAY_RENDER->TIME_WIDTH and
   DAY_RENDER->ROW_HEIGHT) of DAY_RENDER.  */
static void
gtk_day_render_update_extents (GtkDayRender *day_render)
{
  if (! day_render->row_height)
    {
      gint i;
      PangoLayout *pl;
      gint width;
      gint height;

      pl = gtk_widget_create_pango_layout (GTK_WIDGET (day_render), NULL);

      width = 0;
      height = 0;
      for (i = 0; i < day_render->rows; i++)
	{
	  time_t tm;
	  struct tm ftm;
	  char timebuf[10];
	  char buf[60], *buffer;
	  PangoRectangle pr;

	  tm = day_render->date
	    + i * ((gfloat) day_render->duration / day_render->rows);
	  localtime_r (&tm, &ftm);
	  strftime (timebuf, sizeof (timebuf), TIMEFMT, &ftm);
        
	  snprintf (buf, sizeof (buf), "<span font_desc='normal'>%s</span>",
		    timebuf);
	  buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	  pango_layout_set_markup (pl, buffer, strlen (buffer));
	  pango_layout_get_pixel_extents (pl, &pr, NULL);
	  width = MAX (width, pr.width);
	  height = MAX (height, pr.height);

	  g_free (buffer);
	}
      g_object_unref (pl);

#ifdef IS_HILDON
      width = width * 1.6;
#endif

      if (day_render->hour_column)
	day_render->time_width = width + 4;
      else
	day_render->time_width = 0;

      day_render->row_height = height + 6;
    }

  /* Determine the first row we need to show.  */
  g_assert (day_render->date <= day_render->event_earliest);
  day_render->rows_visible_first
    = MIN (((day_render->rows
	      * (day_render->event_earliest - day_render->date))
	     + day_render->duration - 1)
	    / day_render->duration,
	   day_render->rows_hard_first);

  /* And the total number of rows.  */
  g_assert (day_render->event_latest
	    <= day_render->date + day_render->duration);
  day_render->rows_visible
    = MAX (((day_render->rows * (day_render->event_latest - day_render->date))
	    + day_render->duration - 1) / day_render->duration,
	   day_render->rows_hard_first + day_render->rows_hard)
    - day_render->rows_visible_first;

  day_render->height = day_render->visible_height
    * (gfloat) day_render->rows / day_render->rows_visible;
  day_render->offset_y = day_render->height
    * day_render->rows_visible_first / day_render->rows;

  /* Request a window size.  */
  gtk_widget_set_size_request (GTK_WIDGET (day_render), -1,
			       day_render->row_height
			       * day_render->rows_visible);

}

/**
 * Make new set used by overlapping set 
 */
static GNode *
make_set (event_t e)
{
  GSList *set = NULL;
  set = g_slist_append (set, e);

  return g_node_new (set);

}

static gboolean
events_overlap (event_t ev1, event_t ev2)
{				
  /* Event overlaps if and only if they intersect */
  if (is_reminder (ev1) && is_reminder (ev2))
    {
      return TRUE;
    }
  if (ev1 == ev2)
    {
      return FALSE;
    }
  if (ev1->start == ev2->start && event_ends (ev1) == event_ends (ev2))
    {
      return TRUE;
    }
  if ((ev1->start <= ev2->start && event_ends (ev1) < ev2->start) ||
      (ev1->start >= ev2->start && event_ends (ev2) > ev1->start))
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static gboolean
find_set_in_list (GNode * node, gpointer data)
{
  GSList *iter, *ol_set;
  event_t ev = (event_t) data;
  event_t ev2;


  if (node == NULL)
    {
      found_node = NULL;
      return TRUE;
    }

  ol_set = node->data;

  for (iter = ol_set; iter; iter = iter->next)
    {
      ev2 = (event_t) iter->data;
      if (ev2 == ev)
	continue;		/* Ignore us */

      if (events_overlap (ev, ev2))
	{
	  found_node = ol_set;
	  return TRUE;
	}
    }
  found_node = NULL;
  return FALSE;
}

/* Returns the overlapping set */
static GSList *
find_set (GNode * root, event_t ev)
{
  found_node = NULL;

  g_node_traverse (root, G_IN_ORDER, G_TRAVERSE_ALL, -1, find_set_in_list, ev);

  return found_node;
}


static GSList *
union_set (GSList * set, const event_t ev)
{
  return (g_slist_append (set, ev));
}

static GNode *
find_overlapping_sets (GSList * events)
{
  GNode *root;
  GSList *iter = NULL;
  GSList *set = NULL;

  root = g_node_new (NULL);


  for (iter = events; iter; iter = iter->next)
    {

      event_t ev = (event_t) iter->data;
      set = find_set (root, ev);

      if (set == NULL)		/* the event doesn't overlap */
	{
	  g_node_insert (root, -1, make_set (ev));
	}
      else
	{
	  set = union_set (set, ev);
	}
    }

  return root;
}

static struct event_rect *
event_rect_new (GtkDayRender *day_render, const event_t event,
		gint column, gint columns)
{
  struct event_rect *ev_rect = g_malloc (sizeof (struct event_rect));

  if (is_reminder (event))
    {
      ev_rect->y = 0;
      ev_rect->height = 1;
    }
  else
    {
      if (event->start < day_render->date)
	/* Event starts prior to DAY_RENDER's start.  */
	ev_rect->y = 0;
      else
	ev_rect->y = ((gfloat) (event->start - day_render->date))
	  / day_render->duration;

      if (event->duration == 0)
	ev_rect->height = 1.0 / day_render->rows;
      else if (event->start + event->duration
	       < day_render->date + day_render->duration)
	/* Event ends prior to DAY_RENDER's end.  */
	ev_rect->height
	  = (gfloat) (event->start + event->duration - day_render->date)
	  / day_render->duration - ev_rect->y;
      else
	ev_rect->height = 1 - ev_rect->y;
    }

  ev_rect->x = (gfloat) column / columns;
  ev_rect->width = 1.0 / columns;

  ev_rect->event = event;

  /* Is this the earliest event so far?  */
  if (event->start < day_render->event_earliest)
    day_render->event_earliest = MAX (event->start, day_render->date);
  /* The latest?  */
  if (day_render->event_latest
      < event->start + (is_reminder (event) && event->duration == 0
			? 60 * 60 : event->duration))
    day_render->event_latest
      = MIN (event->start +
	     (is_reminder (event) && event->duration == 0
	      ? 60 * 60 : event->duration),
	     day_render->date + day_render->duration);

  return ev_rect;
}

static void
event_rect_delete (struct event_rect *ev)
{
  g_free (ev);
}

static GSList *
ol_sets_to_rectangles (GtkDayRender *day_render, GNode * node)
{
  GSList *iter, *ev_rects = NULL;
  GNode *n;
  gint column;
  event_t ev;
  gint columns;
  struct event_rect *event_rect;

  for (n = node->children; n; n = n->next)
    {
      column = 0;

      columns = g_slist_length (n->data);
      for (iter = n->data; iter; iter = g_slist_next (iter))
	{
	  ev = (event_t) iter->data;

	  event_rect = event_rect_new (day_render, ev, column, columns);
	  ++column;
	  ev_rects = g_slist_append (ev_rects, event_rect);
	}
    }

  return ev_rects;
}

GdkGC *
pen_new (GtkWidget * widget, guint red, guint green, guint blue)
{
  GdkColormap *colormap;
  GdkGC *pen_color_gc;
  GdkColor pen_color;

  colormap = gdk_window_get_colormap (widget->window);
  pen_color_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (pen_color_gc, widget->style->black_gc);
  pen_color.red = red;
  pen_color.green = green;
  pen_color.blue = blue;
  gdk_colormap_alloc_color (colormap, &pen_color, FALSE, TRUE);
  gdk_gc_set_foreground (pen_color_gc, &pen_color);

  return pen_color_gc;
}
