/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <gtk/gtk.h>

gint bells_and_whistles ( );

gint on_resched_clicked (GtkButton *button, gpointer user_data);

gint on_snooze_clicked (GtkButton *button, gpointer user_data);

gint on_mute_clicked (GtkButton *button, gpointer user_data);

gint on_ok_clicked (GtkButton *button, gpointer user_data);

GtkWidget* create_window (char *announcetext);

#define TIMEFMT "%X"
#define DATEFMT "%x"

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

