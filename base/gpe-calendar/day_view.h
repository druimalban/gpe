/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
extern void day_free_lists(void);
int day_view_init ();



typedef struct day_page {
	GtkWidget *widget;
	guint width;
	guint height;
	guint height_min;
	guint time_col_ratio;
} *day_page_t ;


#endif
