/*
 * Copyright (C) 2002, 2005, 2006 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef DAY_VIEW_H
#define DAY_VIEW_H

extern GtkWidget *day_view (void);
extern gboolean day_view_combined_times;
extern int day_view_init ();
extern gboolean day_view_button_press (GtkWidget * widget, GdkEventButton * event, gpointer d);
extern void day_view_scroll (gboolean);

typedef struct day_page {
  GtkWidget *widget;
  guint width;
  guint height;
  guint height_min;
  guint time_width;
} *day_page_t ;


#endif
