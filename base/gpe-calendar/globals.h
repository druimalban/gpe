/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <time.h>
#include <gpe/event-db.h>

extern GList *times;
extern time_t viewtime;
extern gboolean force_today;

extern void update_current_view (void);
extern void update_all_views (void);

extern guint days_in_month (guint year, guint month);
extern gboolean week_starts_monday;
extern guint week_offset;
extern gboolean day_view_combined_times;
extern void set_time_and_day_view(time_t selected_time);
extern void set_day(int year, int month, int day);

extern GdkPixmap *close_pix, *day_pix;
extern GdkBitmap *close_mask, *day_mask;

extern GtkWidget *main_window, *pop_window;

extern GdkFont *yearfont, *datefont;

#define SECONDS_IN_DAY (24*60*60)

/* stuff that could perhaps be configurable */
#define TIMEFMT "%R"
#define MONTHTIMEFMT "%a %d"
