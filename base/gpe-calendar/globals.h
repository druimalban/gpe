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
#include "event-db.h"

extern GdkFont *timefont;
extern GdkFont *datefont;
extern GdkFont *yearfont;

extern GList *times;
extern guint window_x, window_y;
extern time_t viewtime;

#define VERSION "0.01"

extern void update_current_view (void);

extern guint days_in_month (guint year, guint month);
extern gboolean week_starts_monday;

#define SECONDS_IN_DAY (24*60*60)

/* stuff that could perhaps be configurable */
#define TIMEFMT "%R"

/* KEEP RECURRENCE PAGE OUT UNTIL WE ACTUALLY SUPPORT IT BETTER */
#undef DO_RECURRENCE
