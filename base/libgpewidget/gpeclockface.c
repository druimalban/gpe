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

#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include "gpeclockface.h"

static GdkPixbuf *clock_background, *clock_background_24, *day_night_wheel;

static XftColor color;

static guint background_width, background_height;

static GtkWidgetClass *parent_class;

#define DEFAULT_RADIUS 64

struct _GpeClockFace
{
  GtkWidget widget;

  guint radius;

  guint x_offset, y_offset;

  GtkAdjustment *hour_adj, *minute_adj;
  
  XftDraw *draw;
  Picture image_pict, src_pict;
  GdkPixmap *backing_pixmap;
  GdkGC *backing_gc;
  
  gboolean dragging_minute_hand;
};

struct _GpeClockFaceClass
{
  GtkWidgetClass parent_class;
};

static void
draw_hand (GpeClockFace *clock,
	   double angle, 
	   guint length)
{
  GdkPoint points[5];
  XPointDouble poly[5];
  int i;
  double sa = sin (angle), ca = cos (angle);

  points[0].x = 3;
  points[0].y = 0;

  points[1].x = 3;
  points[1].y = length - 4;

  points[2].x = 0;
  points[2].y = length;

  points[3].x = -3;
  points[3].y = length - 4;

  points[4].x = -3;
  points[4].y = 0;

  for (i = 0; i < 5; i++)
    {
      /* Xrotated = X * COS(angle) - Y * SIN(angle)        
	 Yrotated = X * SIN(angle) + Y * COS(angle) */
      int x = points[i].x * ca - points[i].y * sa;
      int y = points[i].x * sa + points[i].y * ca;
      points[i].x = -x;
      points[i].y = -y;
    }

  for (i = 0; i < 5; i++)
    {
      poly[i].x = points[i].x + clock->x_offset + clock->radius;
      poly[i].y = points[i].y + clock->y_offset + clock->radius;
    }

  XRenderCompositeDoublePoly (GDK_WINDOW_XDISPLAY (clock->widget.window),
			      PictOpOver,
			      clock->src_pict, 
			      clock->image_pict,
			      None,
			      0, 0, 0, 0,
			      poly, 5, 
			      EvenOddRule);
}

static GdkGC *
get_bg_gc (GdkWindow *window, GdkPixmap *pixmap)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  guint gc_mask = 0;
  GdkGCValues gc_values;

  if (private->bg_pixmap == GDK_PARENT_RELATIVE_BG && private->parent)
    {
      return get_bg_gc (GDK_WINDOW (private->parent), pixmap);
    }
  else if (private->bg_pixmap && 
           private->bg_pixmap != GDK_PARENT_RELATIVE_BG && 
           private->bg_pixmap != GDK_NO_BG)
    {
      gc_values.fill = GDK_TILED;
      gc_values.tile = private->bg_pixmap;
      gc_values.ts_x_origin = 0;
      gc_values.ts_y_origin = 0;
      
      gc_mask = (GDK_GC_FILL | GDK_GC_TILE | 
                 GDK_GC_TS_X_ORIGIN | GDK_GC_TS_Y_ORIGIN);
    }
  else
    {
      gc_values.foreground = private->bg_color;
      gc_mask = GDK_GC_FOREGROUND;
    }

  return gdk_gc_new_with_values (pixmap, &gc_values, gc_mask);
}

static gint
gpe_clock_face_expose (GtkWidget *widget,
		  GdkEventExpose *event)
{
  GdkDrawable *drawable;
  GdkPixbuf *current_background;
  GdkGC *gc, *tmp_gc;
  GdkRectangle pixbuf_rect, intersect_rect;
  double hour_angle, minute_angle;
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);
  Display *dpy;

  g_return_val_if_fail (widget != NULL, TRUE);

  drawable = widget->window;

  gc = widget->style->black_gc;

  clock->x_offset = (widget->allocation.width / 2) - clock->radius;
  clock->y_offset = (widget->allocation.height / 2) - clock->radius;

  dpy = GDK_WINDOW_XDISPLAY (drawable);

  if (event)
    {
      gdk_gc_set_clip_rectangle (gc, &event->area);
      gdk_gc_set_clip_rectangle (clock->backing_gc, &event->area);
    }

  tmp_gc = get_bg_gc (drawable, clock->backing_pixmap);
  
  gdk_draw_rectangle (clock->backing_pixmap, tmp_gc, TRUE,
		      0, 0, widget->allocation.width, widget->allocation.height);
  
  g_object_unref (tmp_gc);

  if (gtk_adjustment_get_value (clock->hour_adj) >= 12)
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
		(gtk_adjustment_get_value (clock->hour_adj) * -360 / 24) * 64,
		180 * 64);

  minute_angle = gtk_adjustment_get_value (clock->minute_adj) * 2 * M_PI / 60;
  hour_angle = gtk_adjustment_get_value (clock->hour_adj) * 2 * M_PI / 12;

  draw_hand (clock, minute_angle, 7 * clock->radius / 8);

  draw_hand (clock, hour_angle, 3 * clock->radius / 5);

  gdk_draw_drawable (drawable, gc, clock->backing_pixmap, 0, 0, 0, 0, 
		     widget->allocation.width, widget->allocation.height);

  gdk_gc_set_clip_rectangle (gc, NULL);
  gdk_gc_set_clip_rectangle (clock->backing_gc, NULL);

  return TRUE;
}

static double
calc_angle (int x, int y)
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
  double start_angle = calc_angle (x_start, y_start);
  double hour_angle = gtk_adjustment_get_value (clock->hour_adj) * 2 * M_PI / 12;
  double minute_angle = gtk_adjustment_get_value (clock->minute_adj) * 2 * M_PI / 60;
  
  hour_angle -= start_angle;
  minute_angle -= start_angle;
  if (hour_angle < 0)
    hour_angle = -hour_angle;
  if (minute_angle < 0)
    minute_angle = -minute_angle;

  if (r > (clock->radius * 3 / 5) || (minute_angle < hour_angle))
    clock->dragging_minute_hand = TRUE;
  else
    clock->dragging_minute_hand = FALSE;

  gdk_pointer_grab (w->window, 
		    FALSE, GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
		    w->window, NULL, b->time);

  return TRUE;
}

static gboolean
gpe_clock_face_button_release (GtkWidget *w, GdkEventButton *b)
{
  gdk_pointer_ungrab (b->time);

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

  angle = calc_angle (x, y);
  
  val = (clock->dragging_minute_hand ? 60 : 12) * angle / (2 * M_PI);

  gtk_adjustment_set_value (clock->dragging_minute_hand ? clock->minute_adj : clock->hour_adj, val);

  gdk_window_get_pointer (w->window, NULL, NULL, NULL);

  return TRUE;
}

static void
adjustment_value_changed (GObject *a, GtkWidget *w)
{
  gpe_clock_face_expose (w, NULL);
}

static void
gpe_clock_face_prepare_xrender (GtkWidget *widget)
{
  XRenderPictureAttributes att;
  Display *dpy;
  GdkVisual *gv;
  GdkColormap *gcm;
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);

  clock->backing_pixmap = gdk_pixmap_new (widget->window,
					  widget->allocation.width, 
					  widget->allocation.height,
					  gdk_drawable_get_depth (widget->window));

  clock->backing_gc = gdk_gc_new (clock->backing_pixmap);

  dpy = GDK_WINDOW_XDISPLAY (widget->window);
  gv = gdk_window_get_visual (widget->window);
  gcm = gdk_drawable_get_colormap (widget->window);

  clock->draw = XftDrawCreate (dpy, GDK_WINDOW_XWINDOW (clock->backing_pixmap),
			       gdk_x11_visual_get_xvisual (gv),
			       gdk_x11_colormap_get_xcolormap (gcm));

  clock->image_pict = XftDrawPicture (clock->draw);
  clock->src_pict = XftDrawSrcPicture (clock->draw, &color);

  att.poly_edge = PolyEdgeSmooth;
  XRenderChangePicture (dpy, clock->image_pict, CPPolyEdge, &att);
}

static void
gpe_clock_face_realize (GtkWidget *widget)
{
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
			    GDK_POINTER_MOTION_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

  gpe_clock_face_prepare_xrender (widget);
}

static void
gpe_clock_face_unrealize (GtkWidget *widget)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);
  Display *dpy = GDK_WINDOW_XDISPLAY (widget->window);

  if (clock->image_pict)
    {
      XRenderFreePicture (dpy, clock->image_pict);
      clock->image_pict = 0;
    }

  if (clock->src_pict)
    {
      XRenderFreePicture (dpy, clock->src_pict);
      clock->src_pict = 0;
    }

  if (clock->draw)
    {
      XftDrawDestroy (clock->draw);
      clock->draw = NULL;
    }
  
  if (clock->backing_gc)
    {
      g_object_unref (clock->backing_gc);
      clock->backing_gc = NULL;
    }

  if (clock->backing_pixmap)
    {
      g_object_unref (clock->backing_pixmap);
      clock->backing_pixmap = NULL;
    }

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
gpe_clock_face_size_request (GtkWidget	  *widget,
			GtkRequisition *requisition)
{
  GpeClockFace *clock = GPE_CLOCK_FACE (widget);

  requisition->height = clock->radius * 2;
  requisition->width = clock->radius * 2;
}

static void
gpe_clock_face_init (GpeClockFace *clock)
{
  if (clock_background == NULL)
    clock_background = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/clock.png", NULL);
  
  if (clock_background_24 == NULL)
    clock_background_24 = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/clock24.png", NULL);
  
  if (day_night_wheel == NULL)
    day_night_wheel = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/day-night-wheel.png", NULL);

  background_width = gdk_pixbuf_get_width (clock_background);
  background_height = gdk_pixbuf_get_height (clock_background);

  // Double buffering doesn't play nicely with Render, so we will do that by hand
  gtk_widget_set_double_buffered (GTK_WIDGET (clock), FALSE);

  clock->radius = DEFAULT_RADIUS;

  clock->draw = NULL;
  clock->image_pict = clock->src_pict = 0;
  clock->backing_pixmap = NULL;
  clock->backing_gc = NULL;
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
  //  widget_class->size_allocate = gpe_clock_face_size_allocate;
  widget_class->expose_event = gpe_clock_face_expose;
  widget_class->button_press_event = gpe_clock_face_button_press;
  widget_class->button_release_event = gpe_clock_face_button_release;
  widget_class->motion_notify_event = gpe_clock_face_motion_notify;

  color.color.blue = 0xc000;
  color.color.red = color.color.green = 0;
  color.color.alpha = 0x8000;
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
gpe_clock_face_new (GtkAdjustment *hour, GtkAdjustment *minute)
{
  GpeClockFace *clock = g_object_new (gpe_clock_face_get_type (), NULL);

  clock->hour_adj = hour;
  clock->minute_adj = minute;

  g_signal_connect (G_OBJECT (hour), "value_changed", G_CALLBACK (adjustment_value_changed), clock);
  g_signal_connect (G_OBJECT (minute), "value_changed", G_CALLBACK (adjustment_value_changed), clock);

  return GTK_WIDGET (clock);
}
