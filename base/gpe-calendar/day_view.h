/*
 * Copyright (C) 2002, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef DAY_VIEW_H
#define DAY_VIEW_H

struct _GtkDayView;
typedef struct _GtkDayView GtkDayView;

#define GTK_DAY_VIEW(obj) \
  GTK_CHECK_CAST (obj, gtk_day_view_get_type (), struct _GtkDayView)
#define GTK_DAY_VIEW_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gtk_day_view_get_type (), DayViewClass)
#define GTK_IS_DAY_VIEW(obj) GTK_CHECK_TYPE (obj, gtk_day_view_get_type ())

/* Return GType of a day view.  */
extern GType gtk_day_view_get_type (void);

/* Create a new day view.  */
extern GtkWidget *gtk_day_view_new (time_t time);

/* Scroll.  */
extern void gtk_day_view_scroll (GtkDayView *day_view, gboolean force);

#endif
