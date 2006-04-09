/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef WEEK_VIEW_H
#define WEEK_VIEW_H

struct _GtkWeekView;
typedef struct _GtkWeekView GtkWeekView;

#define GTK_WEEK_VIEW(obj) \
  GTK_CHECK_CAST (obj, gtk_week_view_get_type (), struct _GtkWeekView)
#define GTK_WEEK_VIEW_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gtk_week_view_get_type (), WeekViewClass)
#define GTK_IS_WEEK_VIEW(obj) GTK_CHECK_TYPE (obj, gtk_week_view_get_type ())

/* Return GType of a week view.  */
extern GType gtk_week_view_get_type (void);

extern GtkWidget *gtk_week_view_new (time_t time);

#endif
