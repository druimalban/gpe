/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define _BSD_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

#include "gpeclockface.h"

#define BACKGROUND_IMAGE TRUE

#ifdef BACKGROUND_IMAGE
static GdkPixbuf *clock_background, *clock_background_24;
static guint background_width, background_height;
#endif

static GdkPixbuf *day_night_wheel;

static GtkWidgetClass *parent_class;

#define DEFAULT_RADIUS 64
#define DEFAULT_BORDER 2

struct _GpeClockFace
{
  GtkWidget widget;

  guint radius;
  guint border;
  double hand_width;

  gboolean use_background_image;
  gboolean label_hours;

  guint x_offset, y_offset;

  GtkAdjustment *hour_adj, *minute_adj, *second_adj;

#ifdef HAVE_CAIRO
  cairo_t *cr;
  cairo_surface_t *surface;
#endif

  GdkPixmap *backing_pixmap;
  GdkGC *backing_gc;
  GdkGC *backing_poly_gc;
  
  gboolean dragging_minute_hand;
  gboolean do_grabs;
  gboolean grabbed;

  gulong hour_handler, minute_handler, second_handler;

  double hour_angle, minute_angle, second_angle;
};

struct _GpeClockFaceClass
{
  GtkWidgetClass parent_class;
};

typedef struct
{
  double x, y;
} FakeXPointDouble;

static void
draw_hand (GpeClockFace *clock,
	   double angle, 
	   double length,
	   double thick)
{
  FakeXPointDouble points[5], poly[5];
  int i;
  double sa = sin (angle), ca = cos (angle);

  points[0].x = thick;
  points[0].y = 0;

  points[1].x = thick;
  points[1].y = length - 4;

  points[2].x = 0;
  points[2].y = length;

  points[3].x = -thick;
  points[3].y = length - 4;

  points[4].x = -thick;
  points[4].y = 0;

  for (i = 0; i < 5; i++)
    {
      /* Xrotated = X * COS(angle) - Y * SIN(angle)        
	 Yrotated = X * SIN(angle) + Y * COS(angle) */
      double x = (double)points[i].x * ca - (double)points[i].y * sa;
      double y = (double)points[i].x * sa + (double)points[i].y * ca;
      poly[i].x = -x + clock->x_offset + clock->radius;
      poly[i].y = -y + clock->y_offset + clock->radius;
    }

#ifdef HAVE_CAIRO
  cairo_set_rgb_color (clock->cr, 0, 0, 0.8);
  cairo_set_alpha (clock->cr, 0.5);

  cairo_move_to (clock->cr, poly[0].x, poly[0].y);
  cairo_line_to (clock->cr, poly[1].x, poly[1].y);
  cairo_line_to (clock->cr, poly[2].x, poly[2].y);
  cairo_line_to (clock->cr, poly[3].x, poly[3].y);
  cairo_line_to (clock->cr, poly[4].x, poly[4].y);

  cairo_fill (clock->cr);
#else
    {
      GdkPoint gpoints[5];

      for (i = 0; i < 5; i++)
	{
	  gpoints[i].x = poly[i].x;
	  gpoints[i].y = poly[i].y;
	}

      gdk_draw_polygon (clock->backing_pixmap,
			clock->backing_poly_gc,
			TRUE,
			gpoints,
			5);
    }
#endif
}

static void
hand_angles (GpeClockFace *clock)
{
  if (clock->second_adj != NULL)
    clock->second_angle = gtk_adjustment_get_value (clock->second_adj) * 2 * M_PI / 60;
  clock->minute_angle = gtk_adjustment_get_value (clock->minute_adj) * 2 * M_PI / 60;
  clock->hour_angle = gtk_adjustment_get_value (clock->hour_adj) * 2 * M_PI / 12
    + gtk_adjustment_get_value (clock->minute_adj) * 2 * M_PI / 60 / 12;
}

static gint
gpe_clock_face_expose (GtkWidget *widget,
		       GdkEventExpose *event)
{
  GdkDrawable *drawable;
  GdkGC *gc;
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);
  Display *dpy;
  int cur_hour;
  gboolean afternoon;
  GdkGC *white_gc;

  g_return_val_if_fail (widget != NULL, TRUE);

  drawable = widget->window;

  gc = widget->style->black_gc;
  white_gc = widget->style->white_gc;

  clock->x_offset = (widget->allocation.width / 2) - clock->radius;
  clock->y_offset = (widget->allocation.height / 2) - clock->radius;

  cur_hour = gtk_adjustment_get_value (clock->hour_adj);
  afternoon = (cur_hour >= 12) ? TRUE : FALSE;

  dpy = GDK_WINDOW_XDISPLAY (drawable);

  if (event)
    {
      gdk_gc_set_clip_rectangle (clock->backing_gc, &event->area);
      if (clock->backing_poly_gc)
	gdk_gc_set_clip_rectangle (clock->backing_poly_gc, &event->area);
    }

  gdk_draw_rectangle (clock->backing_pixmap, clock->backing_gc,
		      TRUE, 0, 0, widget->allocation.width, widget->allocation.height);

  gdk_gc_set_clip_rectangle (gc, NULL);
  gdk_gc_set_clip_rectangle (white_gc, NULL);

  if (clock->use_background_image)
    {
      GdkRectangle pixbuf_rect, intersect_rect;
      GdkPixbuf *current_background;

      if (afternoon)
	current_background = clock_background_24;
      else
	current_background = clock_background;
      
      if (event)
	{
	  pixbuf_rect.x = clock->x_offset;
	  pixbuf_rect.y = clock->y_offset;
	  pixbuf_rect.width = background_width;
	  pixbuf_rect.height = background_height;
	  
	  if (gdk_rectangle_intersect (&pixbuf_rect, &event->area, &intersect_rect) == TRUE)
	    gdk_pixbuf_render_to_drawable (current_background, 
					   clock->backing_pixmap,
					   clock->backing_gc, 
					   intersect_rect.x - clock->x_offset, intersect_rect.y - clock->y_offset, 
					   intersect_rect.x, intersect_rect.y,
					   intersect_rect.width, intersect_rect.height,
					   GDK_RGB_DITHER_NONE, 0, 0);
	}
      else
	gdk_pixbuf_render_to_drawable (current_background, 
				       clock->backing_pixmap,
				       clock->backing_gc, 
				       0, 0, clock->x_offset, clock->y_offset,
				       gdk_pixbuf_get_width (clock_background), 
				       gdk_pixbuf_get_height (clock_background), 
				       GDK_RGB_DITHER_NONE, 0, 0);
      

      gdk_pixbuf_render_to_drawable (day_night_wheel, 
				     clock->backing_pixmap, 
				     clock->backing_gc, 
				     0, 0, 
				     (clock->x_offset + clock->radius) - (gdk_pixbuf_get_width (day_night_wheel) / 2), 
				     (clock->y_offset + clock->radius) - (gdk_pixbuf_get_height (day_night_wheel) / 2), 
				     -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
      
      gdk_draw_arc (clock->backing_pixmap, 
		    widget->style->white_gc, TRUE,
		    ((clock->x_offset + clock->radius) - (gdk_pixbuf_get_width (day_night_wheel) / 2)) - 2,
		    ((clock->y_offset + clock->radius) - (gdk_pixbuf_get_height (day_night_wheel) / 2)) - 2,
		    gdk_pixbuf_get_width (day_night_wheel) + 4,
		    gdk_pixbuf_get_height (day_night_wheel) + 4,
		    (gtk_adjustment_get_value (clock->hour_adj) * -360 / 24) * 64
		    + gtk_adjustment_get_value (clock->minute_adj) * -(360 * 64 / 24) / 60,
		    180 * 64);
    }
  else
    {
#ifdef HAVE_CAIRO
      cairo_set_alpha (clock->cr, 1.0);

      cairo_set_rgb_color (clock->cr, 1, 1, 1);
      cairo_arc (clock->cr,
		 clock->x_offset + clock->radius,
		 clock->y_offset + clock->radius,
		 clock->radius,
		 0,
		 2 * M_PI);
      cairo_fill (clock->cr);
      cairo_set_rgb_color (clock->cr, 0, 0, 0);
      cairo_arc (clock->cr,
		 clock->x_offset + clock->radius,
		 clock->y_offset + clock->radius,
		 clock->radius,
		 0,
		 2 * M_PI);
      cairo_stroke (clock->cr);
#else
      gdk_draw_arc (clock->backing_pixmap, gc, TRUE,
		    clock->x_offset, clock->y_offset,
		    clock->radius * 2, clock->radius * 2,
		    0, 360 * 64);
  
      gdk_draw_arc (clock->backing_pixmap, white_gc, TRUE,
		    clock->x_offset + clock->border, clock->y_offset + clock->border,
		    (clock->radius - clock->border) * 2, (clock->radius - clock->border) * 2,
		    0, 360 * 64);
#endif

      if (clock->label_hours)
	{
	  PangoLayout *pl;
	  int i;
  
	  pl = gtk_widget_create_pango_layout (widget, "");

	  for (i = 0; i < 12; i++)
	    {
	      double angle, dx, dy;
	      int x, y, width, height;
	      char buf[3];
	      
	      angle = 2 * M_PI * i / 12;
	      
	      dx = sin (angle) * (clock->radius - clock->border - 10);
	      dy = -cos (angle) * (clock->radius - clock->border - 10);

	      x = clock->x_offset + clock->radius + dx;
	      y = clock->y_offset + clock->radius + dy;

	      sprintf (buf, "%d", afternoon ? (i + 12) : i);

	      pango_layout_set_text (pl, buf, strlen (buf));
	      pango_layout_get_size (pl, &width, &height);
	      width /= PANGO_SCALE;
	      height /= PANGO_SCALE;

	      gtk_paint_layout (widget->style,
				clock->backing_pixmap,
				GTK_WIDGET_STATE (widget),
				FALSE,
				NULL,
				widget,
				"label",
				x - (width / 2), y - (height / 2), pl);
	    }
	  
	  g_object_unref (pl);
	}
    }

  if (clock->second_adj != NULL)
    draw_hand (clock, clock->second_angle, 7 * clock->radius / 8, 1);
  draw_hand (clock, clock->minute_angle, 7 * clock->radius / 8, clock->hand_width);
  draw_hand (clock, clock->hour_angle, 3 * clock->radius / 5, clock->hand_width);

  gdk_draw_drawable (drawable, gc, clock->backing_pixmap, 0, 0, 0, 0, 
		     widget->allocation.width, widget->allocation.height);

  gdk_gc_set_clip_rectangle (clock->backing_gc, NULL);
  if (clock->backing_poly_gc)
    gdk_gc_set_clip_rectangle (clock->backing_poly_gc, NULL);

  return TRUE;
}

static double
angle_from_xy (int x, int y)
{
  double r;
  double quad = M_PI_2;;

  if (x < 0)
    quad = M_PI + M_PI_2;

  r = (double)y / (double)x;
  return atan (r) + quad;
}

static gboolean
gpe_clock_face_button_press (GtkWidget *w, GdkEventButton *b)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (w);
  gint x_start = b->x - clock->x_offset - clock->radius;
  gint y_start = b->y - clock->y_offset - clock->radius;
  double r = sqrt (x_start * x_start + y_start * y_start);
  double start_angle = angle_from_xy (x_start, y_start);
  double hour_angle = ((int)gtk_adjustment_get_value (clock->hour_adj) % 12) * 2 * M_PI / 12;
  double minute_angle = gtk_adjustment_get_value (clock->minute_adj) * 2 * M_PI / 60;

  hour_angle -= start_angle;
  minute_angle -= start_angle;
  hour_angle = fabs (hour_angle);
  minute_angle = fabs (minute_angle);

  if (r > (clock->radius * 3 / 5) || (minute_angle < hour_angle))
    clock->dragging_minute_hand = TRUE;
  else
    clock->dragging_minute_hand = FALSE;

  if (clock->do_grabs)
    {
      if (gdk_pointer_grab (w->window, FALSE, 
			    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
			    w->window, NULL, b->time) == GDK_GRAB_SUCCESS)
	clock->grabbed = TRUE;
    }

  return TRUE;
}

static gboolean
gpe_clock_face_button_release (GtkWidget *w, GdkEventButton *b)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (w);

  if (clock->grabbed)
    {
      clock->grabbed = FALSE;
      gdk_pointer_ungrab (b->time);
    }

  return TRUE;
}

static gboolean
gpe_clock_face_motion_notify (GtkWidget *w, GdkEventMotion *m)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (w);
  gint x = m->x - clock->x_offset - clock->radius;
  gint y = m->y - clock->y_offset - clock->radius;
  int val;
  double angle;

  angle = angle_from_xy (x, y);

  if (clock->dragging_minute_hand)
    {  
      double old_val = gtk_adjustment_get_value (clock->minute_adj);

      val = 60 * angle / (2 * M_PI);

      if (old_val > 45 && val < 15)
	{
	  double hour = gtk_adjustment_get_value (clock->hour_adj);
	  hour++;
	  if (hour > 23)
	    hour = 0;
	  gtk_adjustment_set_value (clock->hour_adj, hour);
	}
      else if (old_val < 15 && val > 45)
	{
	  double hour = gtk_adjustment_get_value (clock->hour_adj);
	  hour--;
	  if (hour < 0)
	    hour = 23;
	  gtk_adjustment_set_value (clock->hour_adj, hour);
	}

      gtk_adjustment_set_value (clock->minute_adj, val);
    }
  else
    {
      double old_val = gtk_adjustment_get_value (clock->hour_adj);

      val = 12 * angle / (2 * M_PI);

      if ((((old_val > 9 && val < 3)               /* dragging forwards over midday */
	    || (old_val > 12                       /* still in the afternoon */
		&& ! (old_val < 15 && val > 9)))   /* dragging backwards over midday */
	   && ! (old_val > 21 && val < 3))         /* dragging forwards over midnight */
	  || (old_val < 3 && val > 9))             /* dragging backwards over midnight */
	val += 12;

      gtk_adjustment_set_value (clock->hour_adj, val);
    }

  hand_angles (clock);

  gdk_window_get_pointer (w->window, NULL, NULL, NULL);

  return TRUE;
}

static void
adjustment_value_changed (GObject *a, GtkWidget *w)
{
  hand_angles (GPE_CLOCK_FACE (w));

  gtk_widget_queue_draw (w);
}

static void
gpe_clock_face_prepare_xrender (GtkWidget *widget)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);
  GdkGCValues gcv;
  Display *dpy;
  GdkVisual *gv;
  GdkColormap *gcm;

  clock->backing_pixmap = gdk_pixmap_new (widget->window,
					  widget->allocation.width, 
					  widget->allocation.height,
					  gdk_drawable_get_depth (widget->window));

  clock->backing_gc = gdk_gc_new (clock->backing_pixmap);
  gdk_gc_get_values (widget->style->bg_gc[GTK_STATE_NORMAL], &gcv);
  gdk_gc_set_foreground (clock->backing_gc, &gcv.foreground);

  dpy = GDK_WINDOW_XDISPLAY (widget->window);
  gv = gdk_window_get_visual (widget->window);
  gcm = gdk_drawable_get_colormap (widget->window);
      
#ifdef HAVE_CAIRO
  clock->cr = cairo_create ();
  clock->surface = cairo_xlib_surface_create (dpy, GDK_WINDOW_XWINDOW (clock->backing_pixmap), 
					      gdk_x11_visual_get_xvisual (gv), 0, 
					      gdk_x11_colormap_get_xcolormap (gcm));
  cairo_set_target_surface (clock->cr, clock->surface);
#else
  clock->backing_poly_gc = gdk_gc_new (clock->backing_pixmap);
#endif
}

static void
gpe_clock_face_unprepare_xrender (GtkWidget *widget)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);

  if (clock->backing_gc)
    {
      g_object_unref (clock->backing_gc);
      clock->backing_gc = NULL;
    }

#ifdef HAVE_CAIRO
  cairo_destroy (clock->cr);
  cairo_surface_destroy (clock->surface);
#else
  if (clock->backing_poly_gc)
    {
      g_object_unref (clock->backing_poly_gc);
      clock->backing_poly_gc = NULL;
    }
#endif
      
  if (clock->backing_pixmap)
    {
      g_object_unref (clock->backing_pixmap);
      clock->backing_pixmap = NULL;
    }
}

static void
gpe_clock_face_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      gpe_clock_face_unprepare_xrender (widget);
      gpe_clock_face_prepare_xrender (widget);
    }
}

static void
gpe_clock_face_realize (GtkWidget *widget)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
    
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

  gpe_clock_face_prepare_xrender (widget);

  clock->hour_handler = g_signal_connect (G_OBJECT (clock->hour_adj), "value_changed", 
					  G_CALLBACK (adjustment_value_changed), clock);
  clock->minute_handler = g_signal_connect (G_OBJECT (clock->minute_adj), "value_changed", 
					    G_CALLBACK (adjustment_value_changed), clock);
  if (clock->second_adj != NULL)
    clock->second_handler = g_signal_connect (G_OBJECT (clock->second_adj), "value_changed", 
					      G_CALLBACK (adjustment_value_changed), clock);
}

GdkBitmap *
gpe_clock_face_get_shape (GpeClockFace *clock)
{
  GdkBitmap *bitmap;
  GdkGC *zero_gc, *one_gc;
  GdkColor zero, one;
  GtkWidget *widget;
  int width, height;

  widget = GTK_WIDGET (clock);

  width = clock->radius * 2;
  height = clock->radius * 2;

  bitmap = gdk_pixmap_new (widget->window, width, height, 1);

  zero_gc = gdk_gc_new (bitmap);
  one_gc = gdk_gc_new (bitmap);

  zero.pixel = 0;
  one.pixel = 1;

  gdk_gc_set_foreground (zero_gc, &zero);
  gdk_gc_set_foreground (one_gc, &one);

  gdk_draw_rectangle (bitmap, zero_gc, TRUE, 0, 0, width, height);

  gdk_draw_arc (bitmap, one_gc, TRUE,
		clock->x_offset, clock->y_offset,
		clock->radius * 2, clock->radius * 2,
		0, 360 * 64);

  g_object_unref (one_gc);
  g_object_unref (zero_gc);

  return bitmap;
}

static void
gpe_clock_face_unrealize (GtkWidget *widget)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);

  g_signal_handler_disconnect (clock->hour_adj, clock->hour_handler);
  g_signal_handler_disconnect (clock->minute_adj, clock->minute_handler);
  if (clock->second_adj != NULL)
    g_signal_handler_disconnect (clock->second_adj, clock->second_handler);

  gpe_clock_face_unprepare_xrender (widget);

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
gpe_clock_face_size_request (GtkWidget	  *widget,
			     GtkRequisition *requisition)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);

  requisition->height = (clock->radius + 1) * 2;
  requisition->width = (clock->radius + 1) * 2;
}

static void
gpe_clock_face_init (GpeClockFace *clock)
{
#ifdef BACKGROUND_IMAGE
  if (clock_background == NULL)
    clock_background = gdk_pixbuf_new_from_file (PREFIX "/share/libgpewidget/clock.png", NULL);
  
  if (clock_background_24 == NULL)
    clock_background_24 = gdk_pixbuf_new_from_file (PREFIX "/share/libgpewidget/clock24.png", NULL);

  background_width = gdk_pixbuf_get_width (clock_background);
  background_height = gdk_pixbuf_get_height (clock_background);
#endif
  
  if (day_night_wheel == NULL)
    day_night_wheel = gdk_pixbuf_new_from_file (PREFIX "/share/libgpewidget/day-night-wheel.png", NULL);

  // Double buffering doesn't play nicely with Render, so we will do that by hand
  gtk_widget_set_double_buffered (GTK_WIDGET (clock), FALSE);

  clock->radius = DEFAULT_RADIUS;
  clock->border = DEFAULT_BORDER;
  clock->use_background_image = TRUE;
  clock->label_hours = FALSE;
  clock->hand_width = 3;

  clock->backing_pixmap = NULL;
  clock->backing_gc = NULL;
  clock->backing_poly_gc = NULL;
  clock->grabbed = FALSE;
  clock->do_grabs = TRUE;
}

static void
gpe_clock_face_class_init (GpeClockFaceClass * klass)
{
  GObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_widget_get_type ());
  oclass = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->realize = gpe_clock_face_realize;
  widget_class->unrealize = gpe_clock_face_unrealize;
  widget_class->size_request = gpe_clock_face_size_request;
  widget_class->size_allocate = gpe_clock_face_size_allocate;
  widget_class->expose_event = gpe_clock_face_expose;
  widget_class->button_press_event = gpe_clock_face_button_press;
  widget_class->button_release_event = gpe_clock_face_button_release;
  widget_class->motion_notify_event = gpe_clock_face_motion_notify;
}

void
gpe_clock_face_set_do_grabs (GpeClockFace *clock, gboolean yes)
{
  clock->do_grabs = yes;
}

void
gpe_clock_face_set_radius (GpeClockFace *clock, guint radius)
{
  clock->radius = radius;
  if (clock->radius != DEFAULT_RADIUS)
    clock->use_background_image = FALSE;
}

void
gpe_clock_face_set_use_background_image (GpeClockFace *clock, gboolean yes)
{
  clock->use_background_image = yes;
}

void
gpe_clock_face_set_label_hours (GpeClockFace *clock, gboolean yes)
{
  clock->label_hours = yes;
}

void
gpe_clock_face_set_hand_width (GpeClockFace *clock, double width)
{
  clock->hand_width = width;
}

GType
gpe_clock_face_get_type (void)
{
  static GType clock_type = 0;

  if (! clock_type)
    {
      static const GTypeInfo clock_info =
      {
	sizeof (GpeClockFaceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gpe_clock_face_class_init,
	(GClassFinalizeFunc) NULL,
	NULL /* class_data */,
	sizeof (GpeClockFace),
	0 /* n_preallocs */,
	(GInstanceInitFunc) gpe_clock_face_init,
      };

      clock_type = g_type_register_static (GTK_TYPE_WIDGET, "GpeClockFace", &clock_info, (GTypeFlags)0);
    }
  return clock_type;
}

GtkWidget *
gpe_clock_face_new (GtkAdjustment *hour, GtkAdjustment *minute, GtkAdjustment *second)
{
  GpeClockFace *clock = g_object_new (gpe_clock_face_get_type (), NULL);

  clock->hour_adj = hour;
  clock->minute_adj = minute;
  clock->second_adj = second;

  hand_angles (clock);

  return GTK_WIDGET (clock);
}
