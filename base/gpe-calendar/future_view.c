/*
 * Copyright (C) 2001 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include "gtkdatesel.h"
#include "globals.h"
#include "future_view.h"
#include "event-db.h"

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  guint max_width;
  guint max_height;
  struct tm tm;
  time_t t;
  char buf[64];
  guint x;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->dark_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  gdk_draw_rectangle (drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  time(&t);
  localtime_r(&t, &tm);
  strftime (buf, sizeof(buf), "%A, %d %b %Y", &tm);
  x = (max_width / 2) - (gdk_string_width (widget->style->font, buf) / 2);
  gdk_draw_text (drawable, widget->style->font, black_gc, x, widget->style->font->ascent + widget->style->font->descent + 2, buf, strlen(buf));

  return TRUE;
}

GtkWidget *
future_view(void)
{
  GtkWidget *draw = gtk_drawing_area_new();
  
  gtk_drawing_area_size(GTK_DRAWING_AREA(draw), 240, 13 * 20);
  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);
  return draw;
}
