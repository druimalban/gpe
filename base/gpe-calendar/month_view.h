/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef MONTH_VIEW_H
#define MONTH_VIEW_H

struct _GtkMonthView;
typedef struct _GtkMonthView GtkMonthView;

#define GTK_MONTH_VIEW(obj) \
  GTK_CHECK_CAST (obj, gtk_month_view_get_type (), struct _GtkMonthView)
#define GTK_MONTH_VIEW_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gtk_month_view_get_type (), MonthViewClass)
#define GTK_IS_MONTH_VIEW(obj) GTK_CHECK_TYPE (obj, gtk_month_view_get_type ())

/* Return GType of a month view.  */
extern GType gtk_month_view_get_type (void);

extern GtkMonthView *gtk_month_view_new (void);

#endif
