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
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#include "announce.h"
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <pthread.h>

#include <gpe/soundgen.h>
#include <gpe/schedule.h>

#include <linux/types.h>
#include <linux/ioctl.h>

#include "buzzer.h"

#define MIXER "/dev/mixer"

#define SIZE 256
#define SNOOZE 5

/* Flag to stop alarm sound thread from playing		*/
static gboolean PlayAlarmStop = TRUE;

int fd;
int curl, curr;	
pthread_t SoundThread;

int buzzerfd = -1;

#define BUZZER_FILE "/dev/misc/buzzer"

void open_buzzer (void)
{
	buzzerfd = open (BUZZER_FILE, O_WRONLY);
}

int set_buzzer (int on, int off)
{
	struct buzzer_time t;

	if (buzzerfd == -1)
		return 0;

	t.on_time = on;
	t.off_time = off;

	if (ioctl (buzzerfd, IOC_SETBUZZER, &t))
	{
		perror ("IOC_SETBUZZER");
		return -1;
	}

	return 0;
}

void buzzer_off (int sig)
{
	set_buzzer (0, 0);
	exit (128 + sig);
}

int get_vol(int *left, int *right)
{
	int vol;
	int err;
	char *mixer = MIXER;

	fd = open(mixer, O_RDONLY);
	if(fd == -1)
		printf("Unable to open mixer device: %s\n", mixer);
	err = ioctl(fd, SOUND_MIXER_READ_VOLUME, &vol);
	if(err != -1) {
		*left = vol & 0xff;
		*right = (vol >> 8) & 0xff;
	}
	close(fd);
	return err;
}


int set_vol(int left, int right)
{
	int vol = left | (right << 8);
	int err;
	char *mixer = MIXER;

	fd = open(mixer, O_RDONLY);
	if(fd == -1)
		printf("Unable to open mixer device: %s\n", mixer);
	err = ioctl(fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &vol);
	close(fd);

return err;
}

static void schedule_alarm(const gchar *buf, time_t when)
{
	gchar *text;

	if (buf)
		text = g_strdup_printf ("/usr/bin/gpe-announce '%s'\n", buf);
	else
		text = g_strdup_printf ("/usr/bin/gpe-announce\n");
	schedule_set_alarm (1234, when, text);
	g_free (text);
}

void play_melody(guint Tone1Pitch, guint Tone1Duration,
		 guint Tone2Enable, guint Tone2Pitch, guint Tone2Duration,
		 guint ToneAltCount, guint TonePause)
{
	int snd_dev=-1;
	int i;
	extern int times;

	for (i=0; i<5; i++) {
		if ((snd_dev=soundgen_init()) == -1) {
			g_print("Couldn't init soundgen\n");
			sleep (1);
		} else
			break;
	}
	if (snd_dev == -1)
		return;

	while (!PlayAlarmStop) {
		for (i = 0; i < ToneAltCount; i++) {
			if (PlayAlarmStop)
				break;
			else soundgen_play_tone1(snd_dev, Tone1Pitch,
						 Tone1Duration);
			if (PlayAlarmStop)
				break;
			else soundgen_play_tone2(snd_dev, Tone2Pitch,
						 Tone2Duration);
		}
		soundgen_pause(snd_dev, TonePause);

		switch (times) {
				case 0:
					set_vol(50,50);
					break;
				case 1:
					set_vol(55,55);
					break;
				case 2:
					set_vol(60,60);
					break;
				case 3:
					set_vol(65,65);
					break;
				case 4:
					set_vol(70,70);
					break;
				case 5:
					set_vol(75,75);
					break;
				case 6:
					set_vol(80,80);
					break;
				case 7:
					set_vol(85,85);
					break;
				case 8:
					set_vol(90,90);
					break;
				case 9:
					set_vol(95,95);
					break;
				case 10:
					set_vol(100,100);
					break;
				default:
					break;
			}
					
			times++;
			if (times>20) PlayAlarmStop = TRUE;

	}
	soundgen_final(snd_dev);
	
}


void *play_alarm()
{
	int test=6;

	switch (test) {
		case 1:
			play_melody(500,50,0,0,0,1,1000);
			break;
		case 2:
			play_melody(800,50,0,0,0,1,1000);
			break;
		case 3:
			play_melody(1100,50,0,0,0,1,1000);
			break;
		case 4:
			play_melody(500,100,1,800,100,10,2000);
			break;
		case 5:
			play_melody(800,50,1,1100,50,10,2000);
			break;
		case 6:
			play_melody(800,400,1,1100,400,2,2000);
			break;
		default:
			break;
	}
	pthread_exit(NULL);
	
return (NULL);
}

gint bells_and_whistles ()
{
	open_buzzer ();
  
	if(get_vol(&curl, &curr) == -1)
		printf("Unable to get volume\n");

	signal (SIGINT, buzzer_off);

	set_buzzer (1000, 500);

	set_vol(50,50);
	PlayAlarmStop = FALSE;
	if (pthread_create(&SoundThread, NULL, play_alarm, NULL) != 0) {
		g_print("pthread_create() failed\n");
		gtk_main_quit();
	}
	return(1);
}

gint on_snooze_clicked (GtkButton *button, gpointer user_data)
{
	GtkObject *AlarmWin = GTK_OBJECT (user_data);
	gpointer AlarmComment;
	GtkSpinButton *HoursSpin, *MinutesSpin;
	time_t viewtime;

	PlayAlarmStop = TRUE;
	set_buzzer (0, 0);
	pthread_join(SoundThread, NULL);
	set_vol(curl, curr);

	AlarmComment = gtk_object_get_data (AlarmWin, "AlarmComment");
	HoursSpin = GTK_SPIN_BUTTON (gtk_object_get_data (AlarmWin,
				     "HoursSpin"));
	MinutesSpin = GTK_SPIN_BUTTON (gtk_object_get_data (AlarmWin,
				       "MinutesSpin"));
	time (&viewtime);
	viewtime += gtk_spin_button_get_value_as_int(HoursSpin) * 60*60 +
		    gtk_spin_button_get_value_as_int(MinutesSpin) * 60;

	if (AlarmComment)
		schedule_alarm(gtk_entry_get_text (GTK_ENTRY (AlarmComment)),
			       viewtime);
	else
		schedule_alarm(NULL, viewtime);

	gtk_main_quit();
	return(FALSE);
}


gint on_ok_clicked (GtkButton *button, gpointer user_data)
{
	PlayAlarmStop = TRUE;
	set_buzzer (0, 0);
	pthread_join(SoundThread, NULL);
	set_vol(curl, curr);
	
	gtk_main_quit();
	return(FALSE);
}

gint on_mute_clicked (GtkButton *button, gpointer user_data)
{
	PlayAlarmStop = TRUE;
	set_buzzer (0, 0);
	return(FALSE);
}
