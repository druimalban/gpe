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

static int clock_radius = 64, border = 2;

static guint x_offset, y_offset;

static gboolean hand = TRUE;

static GtkAdjustment *hour_adj, *minute_adj;
static GdkPixbuf *clock_background, *day_night_wheel;

static Display *dpy;

static XftColor color;

static XftDraw *draw;

static Picture image_pict, src_pict;

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
	   GdkGC *gc, 
	   double angle, 
	   guint x_offset, guint y_offset,
	   guint clock_radius,
	   guint length)
{
  GdkPoint points[5];
  XPointDouble poly[5];
  int i;
  double sa = sin (angle), ca = cos (angle);

  dpy = GDK_WINDOW_XDISPLAY (drawable);

  if (! draw)
    draw = make_draw (drawable);
  if (! image_pict)
    image_pict = XftDrawPicture (draw);
  if (! src_pict)
    src_pict = XftDrawSrcPicture (draw, &color);

  points[0].x = 2;
  points[0].y = 0;

  points[1].x = 2;
  points[1].y = length - 4;

  points[2].x = 0;
  points[2].y = length;

  points[3].x = -2;
  points[3].y = length - 4;

  points[4].x = -2;
  points[4].y = 0;

  for (i = 0; i < 5; i++)
    {
      int x = points[i].x * ca - points[i].y * sa;
      int y = points[i].x * sa + points[i].y * ca;
      points[i].x = -x;
      points[i].y = -y;
    }

  for (i = 0; i < 5; i++)
    {
      points[i].x += x_offset + clock_radius;
      points[i].y += y_offset + clock_radius;

      poly[i].x = points[i].x;
      poly[i].y = points[i].y;
    }

  /* Xrotated = X * COS(angle) - Y * SIN(angle)        
     Yrotated = X * SIN(angle) + Y * COS(angle) */

  {
    XRenderPictureAttributes att;
    att.poly_edge = PolyEdgeSmooth;
    XRenderChangePicture (dpy, image_pict, CPPolyEdge, &att);
  }

  XRenderCompositeDoublePoly (dpy,
			      PictOpOver,
			      src_pict, 
			      image_pict,
			      None,
			      0, 0, 0, 0,
			      poly, 5, EvenOddRule);
}

static gint
draw_clock (GtkWidget *widget,
	    GdkEventExpose *event,
	    gpointer user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *white_gc;
  GdkRectangle pixbuf_rect, intersect_rect;
  double hour_angle, minute_angle;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  black_gc = widget->style->black_gc;

  x_offset = (widget->allocation.width / 2) - clock_radius;
  y_offset = (widget->allocation.height / 2) - clock_radius;

  if (event)
    {
      gdk_gc_set_clip_rectangle (black_gc, &event->area);
      gdk_gc_set_clip_rectangle (white_gc, &event->area);
    }

  /*
  gdk_draw_arc (drawable, black_gc, TRUE, 
		x_offset, y_offset,
		clock_radius * 2, clock_radius * 2, 
		0, 360 * 64);

  gdk_draw_arc (drawable, white_gc, TRUE, 
		x_offset + border, y_offset + border,
		(clock_radius - border) * 2, (clock_radius - border) * 2,
		0, 360 * 64);
  */

  /*
  gdk_draw_arc (drawable, black_gc, TRUE, 
		x_offset + clock_radius - 3, y_offset + clock_radius - 3,
		6, 6, 
		0, 360 * 64);
  */

  if (event)
  {
    pixbuf_rect.x = x_offset;
    pixbuf_rect.y = y_offset;
    pixbuf_rect.width = gdk_pixbuf_get_width (clock_background);
    pixbuf_rect.height = gdk_pixbuf_get_height (clock_background);

    if (gdk_rectangle_intersect (&pixbuf_rect, &event->area, &intersect_rect) == TRUE)
      gdk_pixbuf_render_to_drawable (clock_background, drawable, black_gc, intersect_rect.x - x_offset, intersect_rect.y - y_offset, intersect_rect.x, intersect_rect.y, intersect_rect.width, intersect_rect.height, GDK_RGB_DITHER_NONE, 0, 0);
  }
  else
    gdk_pixbuf_render_to_drawable (clock_background, drawable, black_gc, 0, 0, x_offset, y_offset, gdk_pixbuf_get_width (clock_background), gdk_pixbuf_get_height (clock_background), GDK_RGB_DITHER_NONE, 0, 0);
 
  gdk_pixbuf_render_to_drawable (day_night_wheel, drawable, white_gc, 0, 0, (x_offset + clock_radius) - (gdk_pixbuf_get_width (day_night_wheel) / 2), (y_offset + clock_radius) - (gdk_pixbuf_get_height (day_night_wheel) / 2), -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

  minute_angle = gtk_adjustment_get_value (minute_adj) * 2 * M_PI / 60;
  hour_angle = gtk_adjustment_get_value (hour_adj) * 2 * M_PI / 12;

  draw_hand (drawable, black_gc, minute_angle, x_offset, y_offset, clock_radius, 
	     7 * clock_radius / 8);

  draw_hand (drawable, black_gc, hour_angle, x_offset, y_offset, clock_radius, 
	     2 * clock_radius / 3);

  gdk_gc_set_clip_rectangle (black_gc, NULL);
  gdk_gc_set_clip_rectangle (white_gc, NULL);

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
  gint x_start = b->x - x_offset - clock_radius;
  gint y_start = b->y - y_offset - clock_radius;
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

  if (r > (clock_radius * 2 / 3) || (minute_angle < hour_angle))
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
  gint x = m->x - x_offset - clock_radius;
  gint y = m->y - y_offset - clock_radius;
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

  gtk_widget_set_usize (w, clock_radius * 2 + 4, clock_radius * 2 + 4);

  gtk_widget_add_events (GTK_WIDGET (w), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (w), "button-press-event", G_CALLBACK (button_down), NULL);
  g_signal_connect (G_OBJECT (w), "button-release-event", G_CALLBACK (button_up), NULL);
  g_signal_connect (G_OBJECT (w), "motion-notify-event", G_CALLBACK (button_drag), NULL);

  hour_adj = hadj;
  minute_adj = madj;

  g_signal_connect (G_OBJECT (w), "expose_event", G_CALLBACK (draw_clock), NULL);

  g_signal_connect (G_OBJECT (hadj), "value_changed", G_CALLBACK (adjustment_value_changed), w);
  g_signal_connect (G_OBJECT (madj), "value_changed", G_CALLBACK (adjustment_value_changed), w);

  gtk_widget_set_double_buffered (GTK_WIDGET (w), FALSE);

  color.color.blue = 0xffff;
  color.color.red = color.color.green = 0;
  color.color.alpha = 0x4000;

  return w;
}
