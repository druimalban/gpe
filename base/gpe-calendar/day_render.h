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

/* Create a new day_render object.  DATE is start of the period that
   the day render is displaying, DURATION is the duration (in
   seconds).  ROWS is the number of rows to use (thus a row is
   DURATION / ROWS seconds long).  ROWS_HARD_FIRST indicates the first
   row which must be displayed even if it contains no events.
   ROWS_HARD indicates the number of rows which must be shown starting
   with row ROWS_HARD_FIRST.  Event rectangles will be colored with
   APP_GC.  GUTTER is the number of pixels to leave empty around an
   event rectangle.  HOUR_COLUMN indicates if an hour column should be
   drawn.  EVENTS is a list of events to display.  */
extern GtkWidget *gtk_day_render_new (time_t date, gint duration,
				      gint rows,
				      gint rows_hard_first,
				      gint rows_hard,
				      GdkGC *app_gc, gint gutter,
				      gboolean hour_column, 
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

#endif /* DAY_RENDER_H */
