/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <glib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern gboolean schedule_set_alarm (guint id, time_t start, const gchar *action, gboolean calendar_alarm);
extern gboolean schedule_cancel_alarm (guint id, time_t start);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
