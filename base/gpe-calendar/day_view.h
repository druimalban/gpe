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

#include <gtk/gtk.h>
#include <time.h>

#define TYPE_DAY_VIEW (day_view_get_type ())
#define DAY_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DAY_VIEW, DayView))
#define DAY_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_DAY_VIEW, DayViewClass))
#define IS_DAY_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DAY_VIEW))
#define IS_DAY_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_DAY_VIEW))
#define DAY_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_DAY_VIEW, DayViewClass))

struct _DayView;
typedef struct _DayView DayView;

/* Return GType of a day view.  */
extern GType day_view_get_type (void);

/* Create a new day view.  */
extern GtkWidget *day_view_new (time_t time);

#endif
