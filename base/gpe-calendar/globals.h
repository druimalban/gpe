/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include "event-db.h"

extern GdkFont *timefont;
extern GdkFont *datefont;
extern GdkFont *yearfont;

extern GList *times;
extern guint window_x, window_y;
extern time_t viewtime;

#define VERSION "0.0 (20020510)"

extern GtkWidget *new_event(time_t t, guint timesel);
extern GtkWidget *edit_event(event_t ev);

extern void update_current_view (void);

/* stuff that could perhaps be configurable */
#define TIMEFMT "%R"
#define WEEK_STARTS_ON_MONDAY
