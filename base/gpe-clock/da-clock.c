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

#define CLOCK_RADIUS 64

static guint x_offset, y_offset;

static gboolean hand = TRUE;

static GtkAdjustment *hour_adj, *minute_adj;
static GdkPixbuf *clock_background, *day_night_wheel;

static Display *dpy;

static XftColor color;

static XftDraw *draw;
static Picture image_pict, src_pict;
static GdkPixmap *backing_pixmap;
static GdkGC *backing_gc;

static guint background_width, background_height;

static XftDraw *
make_draw (GdkDrawable *drawable)
{
  GdkVisual *gv = gdk_window_get_visual (drawable);
  GdkColormap *gcm = gdk_drawable_get_colormap (drawable);
  XftDraw *draw = XftDrawCreate (dpy, GDK_WINDOW_XWINDOW (drawable),
				 gdk_x11_visual_get_xvisual (gv),
				 gdk_x11_colormap_get_xcolormap (gcm));

  return draw;
}

static void
draw_hand (GdkDrawable *drawable, 
	   double angle, 
	   guint x_offset, guint y_offset,
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
      poly[i].x = points[i].x + x_offset + CLOCK_RADIUS;
      poly[i].y = points[i].y + y_offset + CLOCK_RADIUS;
    }

  XRenderCompositeDoublePoly (dpy,
			      PictOpOver,
			      src_pict, 
			      image_pict,
			      None,
			      0, 0, 0, 0,
			      poly, 5, EvenOddRule);
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
draw_clock (GtkWidget *widget,
	    GdkEventExpose *event,
	    gpointer user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *gc, *tmp_gc;
  GdkRectangle pixbuf_rect, intersect_rect;
  double hour_angle, minute_angle;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;

  gc = widget->style->black_gc;

  x_offset = (widget->allocation.width / 2) - CLOCK_RADIUS;
  y_offset = (widget->allocation.height / 2) - CLOCK_RADIUS;

  dpy = GDK_WINDOW_XDISPLAY (drawable);

  if (! backing_pixmap)
    {
      XRenderPictureAttributes att;
      backing_pixmap = gdk_pixmap_new (drawable,
				       widget->allocation.width, widget->allocation.height,
				       gdk_drawable_get_depth (drawable));
      backing_gc = gdk_gc_new (backing_pixmap);
      draw = make_draw (backing_pixmap);
      image_pict = XftDrawPicture (draw);
      att.poly_edge = PolyEdgeSmooth;
      XRenderChangePicture (dpy, image_pict, CPPolyEdge, &att);
      src_pict = XftDrawSrcPicture (draw, &color);

      background_width = gdk_pixbuf_get_width (clock_background);
      background_height = gdk_pixbuf_get_height (clock_background);
    }

  if (event)
    {
      gdk_gc_set_clip_rectangle (gc, &event->area);
      gdk_gc_set_clip_rectangle (backing_gc, &event->area);
    }

  tmp_gc = get_bg_gc (drawable, backing_pixmap);
  
  gdk_draw_rectangle (backing_pixmap, tmp_gc, TRUE,
		      0, 0, widget->allocation.width, widget->allocation.height);
  
  g_object_unref (tmp_gc);
  
  if (event)
    {
      pixbuf_rect.x = x_offset;
      pixbuf_rect.y = y_offset;
      pixbuf_rect.width = background_width;
      pixbuf_rect.height = background_height;

      if (gdk_rectangle_intersect (&pixbuf_rect, &event->area, &intersect_rect) == TRUE)
	gdk_pixbuf_render_to_drawable (clock_background, 
				       backing_pixmap,
				       backing_gc, 
				       intersect_rect.x - x_offset, intersect_rect.y - y_offset, 
				       intersect_rect.x, intersect_rect.y,
				       intersect_rect.width, intersect_rect.height,
				       GDK_RGB_DITHER_NONE, 0, 0);
    }
  else
    gdk_pixbuf_render_to_drawable (clock_background, 
				   backing_pixmap,
				   backing_gc, 
				   0, 0, x_offset, y_offset,
				   gdk_pixbuf_get_width (clock_background), 
				   gdk_pixbuf_get_height (clock_background), 
				   GDK_RGB_DITHER_NONE, 0, 0);
 
  gdk_pixbuf_render_to_drawable (day_night_wheel, backing_pixmap, 
				 backing_gc, 
				 0, 0, (x_offset + CLOCK_RADIUS) - (gdk_pixbuf_get_width (day_night_wheel) / 2), 
				 (y_offset + CLOCK_RADIUS) - (gdk_pixbuf_get_height (day_night_wheel) / 2), 
				 -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

  minute_angle = gtk_adjustment_get_value (minute_adj) * 2 * M_PI / 60;
  hour_angle = gtk_adjustment_get_value (hour_adj) * 2 * M_PI / 12;

  draw_hand (drawable, minute_angle, x_offset, y_offset,
	     7 * CLOCK_RADIUS / 8);

  draw_hand (drawable, hour_angle, x_offset, y_offset,
	     3 * CLOCK_RADIUS / 5);

  gdk_draw_drawable (drawable, gc, backing_pixmap, 0, 0, 0, 0, 
		     widget->allocation.width, widget->allocation.height);

  gdk_gc_set_clip_rectangle (gc, NULL);
  gdk_gc_set_clip_rectangle (backing_gc, NULL);

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

static void
button_down (GtkWidget *w, GdkEventButton *b, GtkWidget *scrolled_window)
{
  gint x_start = b->x - x_offset - CLOCK_RADIUS;
  gint y_start = b->y - y_offset - CLOCK_RADIUS;
  double r = sqrt (x_start * x_start + y_start * y_start);
  double start_angle = calc_angle (x_start, y_start);
  double hour_angle = gtk_adjustment_get_value (hour_adj) * 2 * M_PI / 12;
  double minute_angle = gtk_adjustment_get_value (minute_adj) * 2 * M_PI / 60;
  
  hour_angle -= start_angle;
  minute_angle -= start_angle;
  if (hour_angle < 0)
    hour_angle = -hour_angle;
  if (minute_angle < 0)
    minute_angle = -minute_angle;

  if (r > (CLOCK_RADIUS * 3 / 5) || (minute_angle < hour_angle))
    hand = TRUE;
  else
    hand = FALSE;

  gdk_pointer_grab (w->window, 
		    FALSE, GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
		    w->window, NULL, b->time);
}

static void
button_up (GtkWidget *w, GdkEventButton *b)
{
  gdk_pointer_ungrab (b->time);
}

static void
button_drag (GtkWidget *w, GdkEventMotion *m, GtkWidget *scrolled_window)
{
  gint x = m->x - x_offset - CLOCK_RADIUS;
  gint y = m->y - y_offset - CLOCK_RADIUS;
  int val;
  double angle;

  angle = calc_angle (x, y);
  
  val = (hand ? 60 : 12) * angle / (2 * M_PI);

  gtk_adjustment_set_value (hand ? minute_adj : hour_adj, val);

  gdk_window_get_pointer (w->window, NULL, NULL, NULL);
}

static void
adjustment_value_changed (GObject *a, GtkWidget *w)
{
  GdkRegion *region = gdk_region_rectangle (&w->allocation);
  //  gdk_window_begin_paint_region (w->window, region);
  draw_clock (w, NULL, NULL);
  //gdk_window_end_paint (w->window);
  gdk_region_destroy (region);
}

GtkWidget *
clock_widget (GtkAdjustment *hadj, GtkAdjustment *madj)
{
  GtkWidget *w = gtk_drawing_area_new ();

  clock_background = gdk_pixbuf_new_from_file ("./clock.png", NULL);
  day_night_wheel = gdk_pixbuf_new_from_file ("./day-night-wheel.png", NULL);

  gtk_widget_set_usize (w, CLOCK_RADIUS * 2 + 4, CLOCK_RADIUS * 2 + 4);

  gtk_widget_add_events (GTK_WIDGET (w), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (w), "expose_event", G_CALLBACK (draw_clock), NULL);
  g_signal_connect (G_OBJECT (w), "button-press-event", G_CALLBACK (button_down), NULL);
  g_signal_connect (G_OBJECT (w), "button-release-event", G_CALLBACK (button_up), NULL);
  g_signal_connect (G_OBJECT (w), "motion-notify-event", G_CALLBACK (button_drag), NULL);

  hour_adj = hadj;
  minute_adj = madj;

  g_signal_connect (G_OBJECT (hadj), "value_changed", G_CALLBACK (adjustment_value_changed), w);
  g_signal_connect (G_OBJECT (madj), "value_changed", G_CALLBACK (adjustment_value_changed), w);

  gtk_widget_set_double_buffered (GTK_WIDGET (w), FALSE);

  color.color.blue = 0xc000;
  color.color.red = color.color.green = 0;
  color.color.alpha = 0x8000;

  return w;
}
