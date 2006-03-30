/*
 *  Copyright (C) 2004 Luca De Cicco <ldecicco@gmx.net> 
 *  Copyright (C) Neal H. Walfield <neal@walfield.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *
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
static GSList *ol_sets_to_rectangles (const GtkDayRender *dr, GNode * node);

/* Number of seconds a day render covers.  */
#define period  (60 * 60 * 24)

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
static struct event_rect *event_rect_new (const GtkDayRender * dr,
					  const event_t event,
					  gint column, gint columns);

/* Delete an event rectangle returned by event_rect_new.  */
static void event_rect_delete (struct event_rect *ev);

static void event_rect_draw (const GtkDayRender *dr,
			     guint canvas_width, guint canvas_height,
			     const struct event_rect *event_rectangle,
			     GdkGC * gc);

static void gtk_day_render_update_extents (GtkDayRender *day_render);

struct _GtkDayRender
{
  GtkDrawingArea drawing_area;

  GdkGC *normal_gc;		/* Normal event color.  */
  GdkGC *ol_gc;			/* Overlapping areas color. */
  guint cols;			/* Number of columns, i.e. how many hours in a row. */
  guint gap;			/* Gap between event boxes. */
  time_t date;			/* Date of this day */
  GSList *events;		/* Events associated to this day. */
  GSList *event_rectangles;

  /* The height and width of the display window.  */
  guint width;
  guint height;

  /* Whether to draw the hours or not.  */
  gboolean hour_column;

  /* Cache of gray.  */
  GdkGC *gray_gc;

  /* The width of the time column.  */
  guint time_width;

  /* Number of rows.  */
  guint rows;
  /* The height of a row in pixels.  */
  guint row_height;
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
  day_render->width = 0;
  day_render->height = 0;
}

GtkWidget *
gtk_day_render_new (GdkGC *app_gc,
		    GdkGC *overl_gc,
		    time_t date,
		    guint cols, guint gap,
		    gboolean hour_column, guint rows, GSList *events)
{
  GtkWidget *widget;
  GtkDayRender *day_render;
  struct tm time;

  g_return_val_if_fail (cols >= 0, NULL);

  widget = gtk_type_new (gtk_day_render_get_type ());
  day_render = GTK_DAY_RENDER (widget);

  localtime_r (&date, &time);
  time.tm_hour = 0;
  time.tm_min = 0;

  day_render->rows = rows;
  day_render->date = mktime (&time);
  day_render->cols = cols;
  day_render->gap = gap;
  day_render->hour_column = hour_column;

  day_render->normal_gc = app_gc;
  g_object_ref (day_render->normal_gc);
  day_render->ol_gc = overl_gc;
  g_object_ref (day_render->ol_gc);

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

  g_object_unref (day_render->ol_gc);
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
  guint i;
  guint top;
  GSList *er;

  g_return_val_if_fail (widget, FALSE);
  g_return_val_if_fail (GTK_IS_DAY_RENDER (widget), FALSE);
  g_return_val_if_fail (event, FALSE);

  if (event->count > 0)
    return FALSE;

  day_render = GTK_DAY_RENDER (widget);

  white_gc = widget->style->white_gc;
  black_gc = widget->style->black_gc;

  pl = gtk_widget_create_pango_layout (widget, NULL);
  
  if (! day_render->gray_gc)
    day_render->gray_gc = pen_new (widget, 58905, 58905, 56610);

  /* Start with a white background.  */
  gdk_draw_rectangle (widget->window, white_gc, TRUE,
		      0, 0, day_render->width - 1, day_render->height - 1);
  if (day_render->hour_column)
    /* Draw the hour column's background.  */
    gdk_draw_rectangle (widget->window, day_render->gray_gc, TRUE,
			0, 0, day_render->time_width, day_render->height - 1);

  /* And now each row's background.  */
  for (i = 0; i < day_render->rows; i ++)
    {
      top = i * (gfloat) day_render->height / day_render->rows;

      /* Draw a gray line to separating each row.  */
      gdk_draw_line (widget->window, day_render->gray_gc,
		     day_render->time_width, top, day_render->width - 1, top);

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

	  tm = day_render->date + i * ((gfloat) period / day_render->rows);
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

  /* Draw a line at the bottom of the last row.  */
  gdk_draw_line (widget->window, day_render->gray_gc,
		 day_render->time_width, day_render->height - 1,
		 day_render->width - 1, day_render->height - 1);
  if (day_render->hour_column)
    gdk_draw_line (widget->window, white_gc,
		   0, day_render->height - 1,
		   day_render->time_width - 1, day_render->height - 1);

  if (day_render->hour_column)
    /* Draw the black vertical line dividing the hour column from the
       events.  */
    gdk_draw_line (widget->window, black_gc,
		   day_render->time_width, 0,
		   day_render->time_width, day_render->height - 1);

  g_object_unref (pl);

  /* Now, draw the events.  */
  for (er = day_render->event_rectangles; er; er = er->next)
    event_rect_draw (day_render, day_render->width, day_render->height,
		     (struct event_rect *) (er->data), day_render->normal_gc);

  return FALSE;
}

static gboolean
gtk_day_render_configure (GtkWidget *widget, GdkEventConfigure *event)
{
  GtkDayRender *day_render = GTK_DAY_RENDER (widget);
  gboolean update = FALSE;

  if (day_render->width != event->width)
    {
      day_render->width = event->width;
      update = TRUE;
    }

  if (day_render->height != event->height)
    {
      day_render->height = event->height;
      update = TRUE;
    }

  if (update)
    gtk_widget_queue_draw (GTK_WIDGET (day_render));

  return FALSE;
}

static gboolean
gtk_day_render_button_press (GtkWidget *widget, GdkEventButton *event)
{
  GtkDayRender *day_render = GTK_DAY_RENDER (widget);
  gfloat x = event->x / day_render->width;
  gfloat y = event->y / day_render->height;
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

  day_render->event_rectangles
    = ol_sets_to_rectangles (day_render,
			     find_overlapping_sets (day_render->events));

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
  gint i;
  PangoLayout *pl;
  guint width;
  guint height;
    
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

      tm = day_render->date + i * ((gfloat) period / day_render->rows);
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

#ifdef IS_HILDON
  width = width * 1.6;
#endif

  if (day_render->hour_column)
    day_render->time_width = width + 4;
  else
    day_render->time_width = 0;

  day_render->row_height = height + 6;
    
  gtk_widget_set_size_request (GTK_WIDGET (day_render),
			       -1, day_render->row_height * day_render->rows);

  g_object_unref (pl);
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

static void
event_rect_draw (const GtkDayRender *dr,
		 guint canvas_width, guint canvas_height,
		 const struct event_rect *event_rectangle, GdkGC *gc)
{
  GdkDrawable *w;
  PangoLayout *pl;
  event_details_t evd;
  gint x, y, width, height;
  GdkRectangle gr;
  gchar *buffer;
  gint arc_size;
  PangoRectangle pr;

  w = GTK_WIDGET (dr)->window;
  pl = gtk_widget_create_pango_layout (GTK_WIDGET (dr), NULL);

  evd = event_db_get_details (event_rectangle->event);
  buffer = g_strdup_printf ("<span size='small'>%s</span>", evd->summary);

  /* Rectangle used to write appointment summary */
  x = dr->time_width + (gint) ((canvas_width - dr->time_width)
			       * event_rectangle->x) + dr->gap;
  y = (gint) (canvas_height * event_rectangle->y) + 1;
  width = (gint) ((canvas_width - dr->time_width) * event_rectangle->width)
    - dr->gap - 1;
  height = (gint) (canvas_height * event_rectangle->height) - 1;


  gr.width = width - 1;
  gr.height = height;
  gr.x = x + 2;
  gr.y = y + 1;

  pango_layout_set_markup (pl, buffer, strlen (buffer));
  pango_layout_get_pixel_extents (pl, &pr, NULL);
  pango_layout_set_width (pl, PANGO_SCALE * gr.width);
  pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);

  arc_size = dr->row_height / 2 - 1;
  if (arc_size < 7)
    /* Lower bound */
    arc_size = 7;

  /* "Flood" the rectangle.  */
  if (height > 2 * arc_size)
    gdk_draw_rectangle (w, dr->normal_gc, TRUE,
			x, y + arc_size, width, height - 2 * arc_size);
  if (width > 2 * arc_size)
    gdk_draw_rectangle (w, dr->normal_gc, TRUE,
			x + arc_size, y, width - 2 * arc_size, height);

  /* Draw the outline.  */

  /* Top.  */
  gdk_draw_line (w, GTK_WIDGET (dr)->style->black_gc,
		 x + arc_size, y, x + width - 1 - arc_size, y);
  /* Bottom.  */
  gdk_draw_line (w, GTK_WIDGET (dr)->style->black_gc,
		 x + arc_size, y + height - 1,
		 x + width - 1 - arc_size, y + height - 1);
  /* Left.  */
  gdk_draw_line (w, GTK_WIDGET (dr)->style->black_gc,
		 x, arc_size + y, x, y + height - 1 - arc_size);
  /* Right.  */
  gdk_draw_line (w, GTK_WIDGET (dr)->style->black_gc,
		 x + width - 1, arc_size + y,
		 x + width - 1, y + height - 1 - arc_size);

  /* Draw the corners.  */

  /* North-west corner */
  gdk_draw_arc (w, dr->normal_gc, TRUE,
		x, y, arc_size * 2, arc_size * 2, 90 * 64, 90 * 64);
  gdk_draw_arc (w, GTK_WIDGET (dr)->style->black_gc, FALSE,
		x, y, arc_size * 2, arc_size * 2, 90 * 64, 90 * 64);
  /* North-east corner */
  gdk_draw_arc (w, dr->normal_gc, TRUE,
		x + width - 1 - 2 * arc_size, y,
		arc_size * 2, arc_size * 2, 0 * 64, 90 * 64);
  gdk_draw_arc (w, GTK_WIDGET (dr)->style->black_gc, FALSE,
		x + width - 1 - arc_size * 2, y,
		arc_size * 2, arc_size * 2, 0 * 64, 90 * 64);
  /* South-west corner */
  gdk_draw_arc (w, dr->normal_gc, TRUE,
		x, y + height - 1 - 2 * arc_size,
		arc_size * 2, arc_size * 2, 180 * 64, 90 * 64);
  gdk_draw_arc (w, GTK_WIDGET (dr)->style->black_gc, FALSE,
		x, y + height - 1 - 2 * arc_size,
		arc_size * 2, arc_size * 2, 180 * 64, 90 * 64);
  /* South-east corner */
  gdk_draw_arc (w, dr->normal_gc, TRUE,
		x + width - 1 - 2 * arc_size, y + height - 1 - 2 * arc_size,
		arc_size * 2, arc_size * 2, 270 * 64, 90 * 64);
  gdk_draw_arc (w, GTK_WIDGET (dr)->style->black_gc, FALSE,
		x + width - 1 - 2 * arc_size, y + height - 1 - 2 * arc_size,
		arc_size * 2, arc_size * 2, 270 * 64, 90 * 64);	

  /* Write summary... */
  gtk_paint_layout (GTK_WIDGET (dr)->style, w, GTK_WIDGET_STATE (dr),
		    FALSE, &gr, GTK_WIDGET (dr), "label", gr.x, gr.y, pl);

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
			   dr->normal_gc,
			   bell_pb,
			   0, 0,
			   x + width - width_pix - 1,
			   y + 1, MIN (width_pix, width - 1),
			   MIN (height_pix, height - 1),
			   GDK_RGB_DITHER_NORMAL, 0, 0);
	}
    }

  g_object_unref (pl);
  g_free (buffer);
}

static struct event_rect *
event_rect_new (const GtkDayRender * dr, const event_t event,
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
      if (event->start < dr->date)
	/* Event starts prior to DR's start.  */
	ev_rect->y = 0;
      else
	ev_rect->y = ((gfloat) (event->start - dr->date)) / period;

      if (event->duration == 0)
	ev_rect->height = 1.0 / dr->rows;
      else if (event->start + event->duration < dr->date + period)
	/* Event ends prior to DR's end.  */
	ev_rect->height
	  = (gfloat) (event->start + event->duration - dr->date) / period
	  - ev_rect->y;
      else
	ev_rect->height = 1 - ev_rect->y;
    }

  ev_rect->x = (gfloat) column / columns;
  ev_rect->width = 1.0 / columns;

  ev_rect->event = event;

  return ev_rect;
}

static void
event_rect_delete (struct event_rect *ev)
{
  g_free (ev);
}

static GSList *
ol_sets_to_rectangles (const GtkDayRender *dr, GNode * node)
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

	  event_rect = event_rect_new (dr, ev, column, columns);
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
