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
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gpe/pixmaps.h>

#include "day_render.h"
#include "day_view.h"
#include "globals.h"

#define _(x) gettext(x)

// #define DEBUG
#ifdef DEBUG
#define D(x) x
#else
#define D(x) do { } while (0)
#endif

static GdkPixbuf *bell_pb;

struct event_rect
{
  gfloat width;
  gfloat height;
  gfloat x;
  gfloat y;
  Event *event;
};

static struct event_rect * event_rect_new (GtkDayRender *day_render,
					   Event *event,
					   gfloat x, gfloat width);

/* Delete an event rectangle returned by event_rect_new.  */
static void event_rect_delete (struct event_rect *ev);

static void gtk_day_render_update_extents (GtkDayRender *day_render);

struct _GtkDayRender
{
  GtkDrawingArea widget;

  /* Appointment event color.  */
  GdkColor app;
  GdkGC *app_gc;
  /* Color to highlight the current time.  */
  GdkGC *time_gc;

  /* When we draw the hour bar, we'd like to update it every 10
     minutes.  */
  guint hour_bar_repaint;
  /* Center of the hour_bar.  */
  int hour_bar_pos;

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
     coordinate.  */
  gint height;
  gint offset_y;

  /* Whether to draw an hour column or not.  */
  gboolean hour_column;

  /* Cache of gray.  */
  GdkGC *gray_gc;

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

  /* Width of the time column in pixels.  */
  gint time_width;
  /* The minimum height of a row in pixels.  */
  gint row_height_min;
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

static void gtk_day_render_size_request (GtkWidget *widget,
					 GtkRequisition *requisition);
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
  widget_class->size_request = gtk_day_render_size_request;
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
		    GdkColor app, gint gap,
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

  day_render->duration = duration;
  day_render->rows = rows;
  day_render->rows_hard_first = rows_hard_first;
  day_render->rows_hard = rows_hard;
  day_render->app = app;
  day_render->gap = gap;
  day_render->hour_column = hour_column;
  gtk_day_render_set_events (day_render, events, date);

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

  if (day_render->app_gc)
    g_object_unref (day_render->app_gc);
  if (day_render->time_gc)
    g_object_unref (day_render->time_gc);

  if (day_render->hour_bar_repaint > 0)
    /* Cancel any outstanding timeout.  */
    g_source_remove (day_render->hour_bar_repaint);

  for (er = day_render->event_rectangles; er; er = er->next)
    event_rect_delete ((struct event_rect *) er->data);
  g_slist_free (day_render->event_rectangles);

  event_list_unref (day_render->events);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define HOUR_BAR_HEIGHT(dr) ((dr)->visible_height / (dr)->rows_visible)

static gboolean hour_bar_move (gpointer d);

/* Calculate the position of the hour bar.  */
static void
hour_bar_calc (GtkDayRender *day_render)
{
  time_t t = time (NULL) - day_render->date;
  time_t start = day_render->rows_visible_first
    * day_render->duration / day_render->rows;
  time_t end = (day_render->rows_visible_first + day_render->rows_visible)
    * day_render->duration / day_render->rows;

  if (start <= t && t <= end)
    {
      day_render->hour_bar_pos
	= day_render->visible_height * (t - start) / (end - start);

      if (! day_render->hour_bar_repaint)
	/* Repaint approximately every 10 minutes.  */
	day_render->hour_bar_repaint
	  = g_timeout_add (10 * 60 * 1000, hour_bar_move, day_render);
    }
  else
    day_render->hour_bar_pos = -1;
}

/* Callback to move the hour bar periodically.  */
static gboolean
hour_bar_move (gpointer d)
{
  GtkDayRender *day_render = GTK_DAY_RENDER (d);
  int top = day_render->hour_bar_pos - HOUR_BAR_HEIGHT (day_render) / 2;
  int bottom = day_render->hour_bar_pos + HOUR_BAR_HEIGHT (day_render) / 2;

  gtk_widget_queue_draw_area (GTK_WIDGET (day_render),
			      0, top,
			      day_render->visible_width, bottom - top + 1);

  hour_bar_calc (day_render);

  if (day_render->hour_bar_pos >= 0)
    /* Still visible.  */
    {
      top = day_render->hour_bar_pos - HOUR_BAR_HEIGHT (day_render) / 2;
      bottom = day_render->hour_bar_pos + HOUR_BAR_HEIGHT (day_render) / 2;

      gtk_widget_queue_draw_area (GTK_WIDGET (day_render),
				  0, top,
				  day_render->visible_width, bottom - top + 1);

      return TRUE;
    }
  else
    /* The hour bar is no longer visible: don't recalculate.  */
    {
      day_render->hour_bar_repaint = 0;
      return FALSE;
    }
}

static void
gtk_day_render_size_request (GtkWidget *widget,
			     GtkRequisition *requisition)
{
  GtkDayRender *day_render = GTK_DAY_RENDER (widget);
  PangoLayout *pl = gtk_widget_create_pango_layout (widget, NULL);
  PangoContext *context = pango_layout_get_context (pl);
  PangoFontMetrics *metrics
    = pango_context_get_metrics (context,
				 widget->style->font_desc,
				 pango_context_get_language (context));

  day_render->row_height_min = (pango_font_metrics_get_ascent (metrics)
				+ pango_font_metrics_get_descent (metrics))
    / PANGO_SCALE + 1;

  requisition->height
    = MAX (requisition->height,
	   day_render->row_height_min * day_render->rows_visible);

  requisition->width
    = MAX (requisition->width,
	   pango_font_metrics_get_approximate_char_width (metrics) * 24 / PANGO_SCALE);

  g_object_unref (pl);
  pango_font_metrics_unref (metrics);
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
  pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
  pango_layout_set_wrap (pl, PANGO_WRAP_WORD_CHAR);

  if (! day_render->gray_gc)
    day_render->gray_gc = pen_new (widget, 58905, 58905, 56610);
  if (! day_render->app_gc)
    day_render->app_gc = pen_new (widget, day_render->app.red,
				  day_render->app.green,
				  day_render->app.blue);
  if (day_render->hour_column)
    /* Draw the hour column's background.  */
    {
      gdk_draw_rectangle (widget->window, day_render->gray_gc, TRUE,
			  0, 0,
			  day_render->visible_width - 1,
			  day_render->visible_height - 1);

      if (day_render->hour_bar_pos >= 0)
	/* Paint the hour bar.  */
	{
	  if (! day_render->time_gc)
	    /* Accent green.  */
	    day_render->time_gc = pen_new (widget, 0x4600, 0xA000, 0x4600);

	  gdk_draw_rectangle (widget->window, day_render->time_gc, TRUE,
			      0,
			      day_render->hour_bar_pos
			      - HOUR_BAR_HEIGHT (day_render) / 2,
			      day_render->visible_width - 1,
			      HOUR_BAR_HEIGHT (day_render) / 2);
	}

      int time_width = 0;
      for (i = day_render->rows_visible_first;
	   i <= day_render->rows_visible_first + day_render->rows_visible;
	   i ++)
	{
	  time_t t;
	  struct tm ftm;
	  char timebuf[10];
	  char buf[60], *buffer;
	  GdkRectangle gr;
	  PangoRectangle pr;

	  top = i * day_render->height / day_render->rows
	    - day_render->offset_y;

	  t = day_render->date
	    + (gfloat) i * day_render->duration / day_render->rows;
	  /* XXX: On Mips and Arm based machines we often end up with
	     up to 60 seconds error!  We only require the hour
	     component so add a few minutes to make sure that we get
	     the right hour.  */
	  t += 5 * 60;
	  localtime_r (&t, &ftm);

	  char *am = nl_langinfo (AM_STR);
	  strftime (timebuf, sizeof (timebuf),
		    am && *am ? "%I%P" : "%H", &ftm);
	  snprintf (buf, sizeof (buf), "<span font_desc='normal'>%s</span>",
		    *timebuf == '0' ? &timebuf[1] : timebuf);
	  buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	  pango_layout_set_markup (pl, buffer, -1);
	  pango_layout_get_pixel_extents (pl, NULL, &pr);

	  gr.width = pr.width;
	  gr.height = pr.height;
	  gr.x = 1;
	  gr.y = top;

	  if (! day_render->time_width)
	    time_width = MAX (time_width, pr.width + 2);

	  gtk_paint_layout (widget->style,
			    widget->window,
			    GTK_WIDGET_STATE (widget),
			    FALSE, &gr, widget, "label", gr.x, gr.y, pl);

	  g_free (buffer);
	}
      if (! day_render->time_width)
	day_render->time_width = time_width;
    }
  else
    day_render->time_width = 0;

  /* Start with a white background.  */
  gdk_draw_rectangle (widget->window, white_gc, TRUE,
		      day_render->time_width, 0,
		      day_render->visible_width - 1,
		      day_render->visible_height - 1);

  if (day_render->hour_column && day_render->hour_bar_pos >= 0)
    /* Draw the hour bar over the white background.  */
    gdk_draw_rectangle (widget->window, day_render->time_gc, TRUE,
			day_render->time_width,
			day_render->hour_bar_pos
			- HOUR_BAR_HEIGHT (day_render) / 2,
			day_render->visible_width - 1,
			HOUR_BAR_HEIGHT (day_render) / 2);

  /* And now each row's background.  */
  for (i = day_render->rows_visible_first;
       i <= day_render->rows_visible_first + day_render->rows_visible;
       i ++)
    {
      top = i * day_render->height / day_render->rows - day_render->offset_y;

      /* Draw a gray line to separate each row but don't draw over the
	 hour bar.  */
      if ((day_render->hour_bar_pos >= 0
	   && ! ((day_render->hour_bar_pos - HOUR_BAR_HEIGHT (day_render) / 2
		  <= top)
		 && (top <= day_render->hour_bar_pos
		     + HOUR_BAR_HEIGHT (day_render) / 2)))
	  || day_render->hour_bar_pos == -1)
	{
	  if (day_render->hour_column)
	    gdk_draw_line (widget->window, white_gc,
			   0, top, day_render->time_width - 1, top);

	  gdk_draw_line (widget->window, day_render->gray_gc,
			 day_render->time_width, top,
			 day_render->visible_width - 1, top);
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
      gint x, y, width, height;
      GdkRectangle gr;
      gchar *buffer;
      gint arc_size;
      PangoRectangle pr;

      w = GTK_WIDGET (day_render)->window;

      /* Rectangle used to write appointment summary */
      x = day_render->time_width
	+ (gint) ((day_render->visible_width - day_render->time_width)
		  * event_rectangle->x)
	+ day_render->gap;
      y = (gint) (day_render->height * event_rectangle->y)
        - day_render->offset_y;
      width = (gint) ((day_render->visible_width - day_render->time_width)
		      * event_rectangle->width) - 2 * day_render->gap;
      height = (gint) (day_render->height * event_rectangle->height);
      if (height < day_render->row_height_min)
	height = day_render->row_height_min;

      arc_size = day_render->row_height_min / 2 - 1;
      if (arc_size < 7)
	/* Lower bound */
	arc_size = 7;

      /* "Flood" the rectangle.  */
      if (height > 2 * arc_size)
	gdk_draw_rectangle (w, day_render->app_gc, TRUE,
			    x, y + arc_size, width, height - 2 * arc_size);
      if (width > 2 * arc_size)
	gdk_draw_rectangle (w, day_render->app_gc, TRUE,
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
      gdk_draw_arc (w, day_render->app_gc, TRUE,
		    x, y, arc_size * 2, arc_size * 2, 90 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x, y, arc_size * 2, arc_size * 2, 90 * 64, 90 * 64);
      /* North-east corner */
      gdk_draw_arc (w, day_render->app_gc, TRUE,
		    x + width - 1 - 2 * arc_size, y,
		    arc_size * 2, arc_size * 2, 0 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x + width - 1 - arc_size * 2, y,
		    arc_size * 2, arc_size * 2, 0 * 64, 90 * 64);
      /* South-west corner */
      gdk_draw_arc (w, day_render->app_gc, TRUE,
		    x, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 180 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 180 * 64, 90 * 64);
      /* South-east corner */
      gdk_draw_arc (w, day_render->app_gc, TRUE,
		    x + width - 1 - 2 * arc_size, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 270 * 64, 90 * 64);
      gdk_draw_arc (w, GTK_WIDGET (day_render)->style->black_gc, FALSE,
		    x + width - 1 - 2 * arc_size, y + height - 1 - 2 * arc_size,
		    arc_size * 2, arc_size * 2, 270 * 64, 90 * 64);	

      /* Write summary... */
      const char *summary = event_get_summary (event_rectangle->event);
      const char *location = event_get_location (event_rectangle->event);
      const char *description = event_get_description (event_rectangle->event);

      char *until = NULL;
      time_t end = event_get_start (event_rectangle->event)
	+ event_get_duration (event_rectangle->event) - 1;
      if (end > day_render->date + day_render->duration)
	{
	  struct tm tm;
	  localtime_r (&end, &tm);

	  until = strftime_strdup_utf8_locale (_("Until %b %d"), &tm);
	}

      char *text (const char *loc_desc_sep)
	{
	  return
	    g_strdup_printf ("<b>%s</b>%s%s%s%s%s%s",
			     summary,
			     (summary && summary[0]
			      && ((location && location[0])
				  || (description && description[0])))
			     ? ": " : "",
			     location ? location : "",
			     location && location[0] ? loc_desc_sep : "",
			     description ? description : "",
			     until && until[0] ? loc_desc_sep : "",
			     until ? until : "");
	}

      buffer = text ("\n");

      gr.width = width - 1;
      gr.height = height - 2;
      gr.x = x + 1;

      pango_layout_set_width (pl, PANGO_SCALE * gr.width);
      pango_layout_set_markup (pl, buffer, -1);

      pango_layout_get_pixel_extents (pl, NULL, &pr);
      if (pr.height >= gr.height)
	/* Text doesn't fit, try removing "\n" between the location
	   and the description.  */
	{
	  g_free (buffer);
	  buffer = text ("; ");
	  pango_layout_set_markup (pl, buffer, -1);
	  pango_layout_get_pixel_extents (pl, NULL, &pr);
	}
      if (pr.height >= gr.height)
	/* Try replaing all "\n"'s with spaces.  */
	{
	  char *p;
	  for (p = strchr (buffer, '\n'); p; p = strchr (p, '\n'))
	    *p = ' ';
	  pango_layout_set_markup (pl, buffer, -1);
	  pango_layout_get_pixel_extents (pl, NULL, &pr);
	}

      if (pr.height >= gr.height)
	/* Still doesn't fit, simply cut off the bottom of the
	   text.  */
	gr.y = y + 1;
      else
	/* Vertically center the text.  */
	gr.y = (y + 1) + (gr.height - pr.height) / 2;

      g_free (buffer);
      g_free (until);

      gtk_paint_layout (GTK_WIDGET (day_render)->style, w,
			GTK_WIDGET_STATE (day_render),
			FALSE, &gr, GTK_WIDGET (day_render), "label",
			gr.x, gr.y, pl);

      if (event_get_alarm (event_rectangle->event))
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
			       day_render->app_gc,
			       bell_pb,
			       0, 0,
			       x + width - width_pix - 1,
			       y + 1, MIN (width_pix, width - 1),
			       MIN (height_pix, height - 1),
			       GDK_RGB_DITHER_NORMAL, 0, 0);
	    }
	}
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
  gfloat height_min
    = (gfloat) day_render->row_height_min / day_render->height;
  for (er = day_render->event_rectangles; er; er = er->next)
    {
      struct event_rect *e_r = er->data;
      if ((x >= e_r->x && x <= e_r->x + e_r->width)
	  && y >= e_r->y && y <= e_r->y + MAX (height_min, e_r->height))
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

static void
calc_events_positions (GtkDayRender *day_render)
{
  GSList *er;

  /* Free any extant rectangles.  */
  for (er = day_render->event_rectangles; er; er = er->next)
    event_rect_delete ((struct event_rect *) er->data);
  g_slist_free (day_render->event_rectangles);
  day_render->event_rectangles = NULL;

  /* If there are no events, then we are done.  */
  if (! day_render->events)
    return;

  /* Return TRUE if A and B overlap, FALSE otherwise.  */
  gboolean overlap (Event *a, Event *b)
    {
      if (event_get_start (a) <= event_get_start (b)
	  && (event_get_start (b)
	      < event_get_start (a) + event_get_duration (a)))
	/* Start of B occurs during A.  */
	return TRUE;
      if (event_get_start (b) <= event_get_start (a)
	  && (event_get_start (a)
	      < event_get_start (b) + event_get_duration (b)))
	/* Start of A occurs during B.  */
	return TRUE;
      if (event_get_start (b) < event_get_start (a) + event_get_duration (a)
	  && (event_get_start (a) + event_get_duration (a)
	      <= event_get_start (b) + event_get_duration (b)))
	/* End of A occurs during B.  */
	return TRUE;
      /* We don't need to check if the end of B occurs during A as
	 this cannot occur without one of the above conditions also
	 occuring.  */
      return FALSE;
    }

  /* Returns -1 if A occurs before B (or, if they start at the same
     time, if A is shorter).  Returns 1 if B occurs before A (or, if
     they start at the same time, if B is shorter).  Otherwise 0.  */
  gint cmp (Event *a, Event *b)
    {
      if (event_get_start (a) < event_get_start (b))
	return -1;
      else if (event_get_start (b) < event_get_start (a))
	return 1;
      else if (event_get_duration (a) < event_get_duration (b))
	return -1;
      else if (event_get_duration (b) < event_get_duration (a))
	return 1;
      return 0;
    }

  /* Calculate the number of other events with which each event
     directly overlaps.  */
  int n = g_slist_length (day_render->events);
  struct info
  {
    Event *event;
    int direct_overlap;
  };
  struct info info[n];
  memset (info, 0, sizeof (info));

  int i, j;
  GSList *a, *b;
  for (a = day_render->events, i = 0; a; a = a->next, i ++)
    {
      info[i].event = a->data;

      for (b = a->next, j = i + 1; b; b = b->next, j ++)
	if (overlap (a->data, b->data))
	  {
	    info[i].direct_overlap ++;
	    info[j].direct_overlap ++;
	  }
    }

  /* A and B overlap.  Returns TRUE if A dominates B.  An event
     dominates another if it directly overlaps with more events than
     the other or, if the direct overlap is equivalent and it occurs
     earlier.  */
  gboolean dominates (struct info *a, struct info *b)
    {
      if (a->direct_overlap > b->direct_overlap)
	return TRUE;
      if (b->direct_overlap > a->direct_overlap)
	return FALSE;
      return cmp (a->event, b->event) < 0;
    }

  /* Given node N, insert it into the tree.  */
  void insert (GNode *root, GNode *node)
    {
      struct info *n = node->data;
      GNode *child;

      D (printf ("%s:%d: inserting %s\n",
		 __func__, __LINE__, event_get_summary (n->event)));

      child = root->children;
      while (child)
	{
	  struct info *c = child->data;

	  if (overlap (c->event, n->event))
	    /* CHILD and NODE overlap.  */
	    {
	      D (printf ("%s:%d: %.10s overlaps with %.10s\n",
			 __func__, __LINE__,
			 event_get_summary (n->event),
			 event_get_summary (c->event)));

	      if (dominates (n, c))
		/* N dominates C (and transitively, everything which C
		   dominates is also dominated by N).  Replace C with
		   N and proceed to insert C under N.  If may also be
		   the case that N dominates some of the events
		   following C.  Consider those.  One of those may in
		   turn dominate C.  */
		{
		  g_node_insert_before (root, child, node);

		  GNode *next;
		  do
		    {
		      D (printf ("%s:%d: %s (%d) dominates %s (%d)\n",
				 __func__, __LINE__,
				 event_get_summary (n->event),
				 n->direct_overlap,
				 event_get_summary (c->event),
				 c->direct_overlap));

		      next = child->next;
		      g_node_unlink (child);
		      g_node_append (node, child);

		      if (! next)
			break;

		      child = next;
		      c = child->data;
		    }
		  while (overlap (c->event, n->event)
			 && dominates (n, c));

		  if (next && overlap (c->event, n->event) && dominates (c, n))
		    {
		      D (printf ("%s:%d: and is dominated by %.10s (%d)\n",
				 __func__, __LINE__,
				 event_get_summary (c->event),
				 c->direct_overlap));
		      g_node_unlink (node);
		      g_node_prepend (child, node);
		    }
		  break;
		}
	      else
		{
		  D (printf (" continuing with %.10s as root\n",
			     event_get_summary (c->event)));

		  /* C dominates N.  N belongs under C.  */
		  root = child;
		  child = child->children;
		}
	    }
	  else if (cmp (n->event, c->event) < 0)
	    {
	      D (printf ("%s:%d: %.10s occurs before %.10s\n",
			 __func__, __LINE__,
			 event_get_summary (n->event),
			 event_get_summary (c->event)));
	      g_node_insert_before (root, child, node);
	      break;
	    }
	  else if (! child->next)
	    {
	      /* CHILD ends before NODE but a child of CHILD could
		 overlap with NODE.  */
	      child = child->children;
	    }
	  else
	    child = child->next;
	}

      if (! child)
	/* Occurs after any child.  */
	{
	  D (printf ("%s:%d: %.10s occurs after any child of %.10s\n",
		     __func__, __LINE__,
		     event_get_summary (n->event),
		     root->data
		     ? event_get_summary (((struct info *) root->data)->event)
		     : "ROOT"));
	  g_node_append (root, node);
	}
    }

  GNode *root = g_node_new (NULL);
  for (i = 0; i < n; i ++)
    insert (root, g_node_new (&info[i]));

  void traverse (GNode *node, gfloat left)
    {
      if (node->data)
	{
	  struct info *info = node->data;
	  int cols = g_node_max_height (node);
	  gfloat width = left / cols;

	  D (printf ("Event %.10s at depth=%d, x=%g, width=%g\n",
		     event_get_summary (info->event),
		     g_node_depth (node), 1 - left, width));
	  event_rect_new (day_render, info->event, 1 - left, width);

	  left -= width;
	}

      GNode *i;
      for (i = node->children; i; i = i->next)
	traverse (i, left);
    }

  traverse (root, 1);

  g_node_destroy (root);
}

void
gtk_day_render_set_events (GtkDayRender *day_render, GSList *events,
			   time_t date)
{
  day_render->date = date;

  event_list_unref (day_render->events);
  day_render->events = events;

  day_render->event_earliest = day_render->date + day_render->duration;
  day_render->event_latest = day_render->date;

  calc_events_positions (day_render);

  gtk_day_render_update_extents (day_render);
  gtk_widget_queue_draw (GTK_WIDGET (day_render));
}

/* Update the calculated extents (DAY_RENDER->ROW_HEIGHT_MIN) of
   DAY_RENDER.  */
static void
gtk_day_render_update_extents (GtkDayRender *day_render)
{
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

  hour_bar_calc (day_render);

  if (day_render->row_height_min)
    /* Request a window size.  */
    gtk_widget_set_size_request (GTK_WIDGET (day_render), -1,
				 day_render->row_height_min
				 * day_render->rows_visible);
}

static struct event_rect *
event_rect_new (GtkDayRender *day_render, Event *event,
		gfloat x, gfloat width)
{
  struct event_rect *ev_rect = g_malloc (sizeof (struct event_rect));

  if (is_reminder (event))
    {
      ev_rect->y = 0;
      ev_rect->height = 1;
    }
  else
    {
      if (event_get_start (event) < day_render->date)
	/* Event starts prior to DAY_RENDER's start.  */
	ev_rect->y = 0;
      else
	ev_rect->y = ((gfloat) (event_get_start (event) - day_render->date))
	  / day_render->duration;

      if (event_get_duration (event) == 0)
	ev_rect->height = 1.0 / day_render->rows;
      else if (event_get_start (event) + event_get_duration (event)
	       < day_render->date + day_render->duration)
	/* Event ends prior to DAY_RENDER's end.  */
	ev_rect->height
	  = (gfloat) (event_get_start (event) + event_get_duration (event)
		      - day_render->date)
	  / day_render->duration - ev_rect->y;
      else
	ev_rect->height = 1 - ev_rect->y;
    }

  ev_rect->event = event;
  ev_rect->x = x;
  ev_rect->width = width;

  /* Is this the earliest event so far?  */
  if (event_get_start (event) < day_render->event_earliest)
    day_render->event_earliest = MAX (event_get_start (event),
				      day_render->date);
  /* The latest?  */
  if (day_render->event_latest
      < event_get_start (event)
      + (is_reminder (event) && event_get_duration (event) == 0
	 ? 60 * 60 : event_get_duration (event)))
    day_render->event_latest
      = MIN (event_get_start (event) +
	     (is_reminder (event) && event_get_duration (event) == 0
	      ? 60 * 60 : event_get_duration (event)),
	     day_render->date + day_render->duration);

  day_render->event_rectangles
    = g_slist_prepend (day_render->event_rectangles, ev_rect);

  return ev_rect;
}

static void
event_rect_delete (struct event_rect *ev)
{
  g_free (ev);
}
