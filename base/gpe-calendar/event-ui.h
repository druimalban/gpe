/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef EVENT_UI_H
#define EVENT_UI_H

#include <time.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "event-db.h"

struct Alarm_t {
	guint year;
	guint month;
	guint day;
	unsigned int hour;
	unsigned int minute;
	unsigned int AlarmType;
	unsigned int AlarmReoccurence;
	char comment[128];
	unsigned int Tone1Pitch;
	unsigned int Tone1Duration;
	unsigned int Tone2Enable;
	unsigned int Tone2Pitch;
	unsigned int Tone2Duration;
	unsigned int ToneAltCount;
	unsigned int TonePause;
} *CurrentAlarm;

extern GtkWidget *new_event(time_t t, guint timesel);
extern GtkWidget *edit_event(event_t ev);

#endif
