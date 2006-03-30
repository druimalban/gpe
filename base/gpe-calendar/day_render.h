/*
 *  Copyright (C) 2004 Luca De Cicco <ldecicco@gmx.net> 
 *  Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *
 */

#ifndef DAY_RENDER_H
#define DAY_RENDER_H

#include <glib.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gpe/event-db.h>
#include "day_view.h"

struct _GtkDayRender;
typedef struct _GtkDayRender GtkDayRender;

#define GTK_DAY_RENDER(obj) \
  GTK_CHECK_CAST (obj, gtk_day_render_get_type (), struct _GtkDayRender)
#define GTK_DAY_RENDER_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gtk_day_render_get_type (), DayRenderClass)
#define GTK_IS_DAY_RENDER(obj) GTK_CHECK_TYPE (obj, gtk_day_render_get_type ())

/* Create a new day_render object.  APP_GC is the appointment color,
   OVERL_GC is the color of overlapping zones, DATE is the date,
   WIDTH, the width of the drawing area, HEIGHT, the height of the
   drawing area, COLS, the number of columns, GAP is the GAP between
   lines, OFFSET is the offset from the (0,0) of the drawing area, and
   EVENTS is a list of events for the day.  */
GtkWidget *gtk_day_render_new (GdkGC *app_gc,
			       GdkGC *overl_gc,
			       time_t date,
			       guint cols, guint gap,
			       gboolean hour_column, guint rows,
			       GSList *events);

/* Return the type of a GtkDayRender.  */
extern GType gtk_day_render_get_type (void);

/* Set the list of events connected to DAY_RENDER to EVENTS.  If
   DAY_RENDER already has a list of events, that list is first
   freed.  */
extern void gtk_day_render_set_events (GtkDayRender *day_render,
				       GSList *events);

/* Set DAY_RENDER's date to DATE.  */
extern void gtk_day_render_set_date (GtkDayRender *day_render, time_t date);

GdkGC *pen_new (GtkWidget * widget, guint red, guint green, guint blue);

#endif /* DAY_RENDER_H */
