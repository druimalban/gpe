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

#include <gst/gst.h>
#include <dlfcn.h>

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

/* Flag to reset sound volume after playing		*/
static gboolean VolumeReset = TRUE;

/* Flag to stop alarm sound thread from playing		*/
static gboolean PlayAlarmStop = TRUE;

/* Flag for current sound configuration */
static int sound_config;

int fd;
int curl, curr, curpcml, curpcmr;	
pthread_t SoundThread;

int buzzerfd = -1;

#define BUZZER_FILE "/dev/misc/buzzer"
#define CFG_NOSOUND 	-2
#define CFG_AUTOMATIC 	-1

/* GST player */
struct player
{
	GstElement *filesrc, *decoder, *audiosink, *thread, *volume;
	
	char *source_elem;
	char *sink_elem;
	int volume_value;
};

typedef struct player *player_t;

extern void *gst_handle;
extern char *filename;
player_t alarm_player=NULL;
extern int alarm_volume;

void play_url(player_t p, char *url);
static void player_stop (player_t p);
void player_set_volume (player_t p, int v);

player_t player_new (int);


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


/* 
 * This reads configuration from configfile. 
 * It returns a (positive) number from 0 to 100 to indicate a fixed volume
 * or a negative constant to indicate automatic volume setting (default)
 * or disabled alarm sound.
 */
int get_config (void)
{
	int result = CFG_AUTOMATIC;
	char *filename = g_strdup_printf("%s/.gpe/alarm.conf", g_get_home_dir());
	FILE *cfgfile;
	int enabled = 1;
	int automatic = 1;
	int level = 0;
	
	cfgfile = fopen(filename, "r");
	if (cfgfile)
	{
		int val = -1, ret;
		char buf[128];
		while (fgets(buf, 128, cfgfile))
		{
			ret = sscanf(buf, "enabled %d", &val);
			if (ret)
				enabled = val;
			
			ret = sscanf(buf, "automatic %d", &val);
			if (ret)
				automatic = val;
			
			ret = sscanf(buf, "level %d", &val);			
			if (ret)
			{
				if ((val >= 0) && (val <=100))
					level = val;
			}
		}
		fclose(cfgfile);
		
		if (!enabled) 
			result = CFG_NOSOUND;
		else
			if (!automatic)
				result = level;
	}
	g_free(filename);
	
	return result;	
}

int get_vol(int *left, int *right, int channel)
{
	int vol;
	int err;
	char *mixer = MIXER;

	fd = open(mixer, O_RDONLY);
	if(fd == -1)
		printf("Unable to open mixer device: %s\n", mixer);
	err = ioctl(fd, channel, &vol);
	if(err != -1) {
		*left = vol & 0xff;
		*right = (vol >> 8) & 0xff;
	}
	close(fd);
	return err;
}


int set_vol(int left, int right, int channel)
{
	int vol = left | (right << 8);
	int err;
	char *mixer = MIXER;

	fd = open(mixer, O_RDONLY);
	if(fd == -1)
		printf("Unable to open mixer device: %s\n", mixer);
	err = ioctl(fd, MIXER_WRITE(channel), &vol);
	close(fd);

return err;
}

static void schedule_alarm(const gchar *buf, time_t when)
{
	gchar *text;

	if (buf)
		text = g_strdup_printf ("gpe-announce '%s'\n", buf);
	else
		text = g_strdup_printf ("gpe-announce\n");
	schedule_set_alarm (1234, when, text, FALSE);
	
	g_free (text);
}

void play_melody(guint Tone1Pitch, guint Tone1Duration,
		 guint Tone2Enable, guint Tone2Pitch, guint Tone2Duration,
		 guint ToneAltCount, guint TonePause)
{
	int snd_dev=-1;
	int i;
	extern int times;
	
	if (sound_config == CFG_NOSOUND)
		return;

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

		if (sound_config == CFG_AUTOMATIC)
			switch (times) {
				case 0:
					set_vol(50,50,SOUND_MIXER_VOLUME);
					break;
				case 1:
					set_vol(55,55,SOUND_MIXER_VOLUME);
					break;
				case 2:
					set_vol(60,60,SOUND_MIXER_VOLUME);
					break;
				case 3:
					set_vol(65,65,SOUND_MIXER_VOLUME);
					break;
				case 4:
					set_vol(70,70,SOUND_MIXER_VOLUME);
					break;
				case 5:
					set_vol(75,75,SOUND_MIXER_VOLUME);
					break;
				case 6:
					set_vol(80,80,SOUND_MIXER_VOLUME);
					break;
				case 7:
					set_vol(85,85,SOUND_MIXER_VOLUME);
					break;
				case 8:
					set_vol(90,90,SOUND_MIXER_VOLUME);
					break;
				case 9:
					set_vol(95,95,SOUND_MIXER_VOLUME);
					break;
				case 10:
					set_vol(100,100,SOUND_MIXER_VOLUME);
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
	/* Play sound file using gst */
	if (filename != NULL && gst_handle)
	{
		alarm_player = player_new(alarm_volume);
		play_url(alarm_player,filename);
		
	}
	else
	{
		/* Play Ambulance `melody` */
		play_melody(800,400,1,1100,400,2,2000);
	        pthread_exit(NULL);
	}	
	return (NULL);
}

gint bells_and_whistles ()
{
	sound_config = get_config();
	
	VolumeReset = TRUE;
	
	open_buzzer ();
  
	if((get_vol(&curl, &curr, SOUND_MIXER_VOLUME) == -1)
		|| (get_vol(&curpcml, &curpcmr, SOUND_MIXER_PCM) == -1))
			VolumeReset = FALSE;
		
	signal (SIGINT, buzzer_off);

	if (sound_config != CFG_NOSOUND)
		set_buzzer (1000, 500);
	
	if (sound_config != CFG_NOSOUND)
	{
		if (sound_config == CFG_AUTOMATIC)
		{
			set_vol(100, 100, SOUND_MIXER_PCM);
			set_vol(50, 50, SOUND_MIXER_VOLUME);
		}
		else if (VolumeReset)
		{
			set_vol(100, 100, SOUND_MIXER_PCM);
			set_vol(sound_config, sound_config, SOUND_MIXER_VOLUME);
		}
	}
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
	if (VolumeReset)
	{
		set_vol(curl, curr, SOUND_MIXER_VOLUME);
		set_vol(curpcml, curpcmr, SOUND_MIXER_PCM);
	}
	AlarmComment = gtk_object_get_data (AlarmWin, "AlarmComment");
	HoursSpin = GTK_SPIN_BUTTON (gtk_object_get_data (AlarmWin,
				     "HoursSpin"));
	MinutesSpin = GTK_SPIN_BUTTON (gtk_object_get_data (AlarmWin,
				       "MinutesSpin"));
	time (&viewtime);
	viewtime += gtk_spin_button_get_value_as_int(HoursSpin) * 60*60 +
		    gtk_spin_button_get_value_as_int(MinutesSpin) * 60;

	if (AlarmComment)
		schedule_alarm(gtk_entry_get_text (GTK_ENTRY (AlarmComment)), viewtime);
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
	if (VolumeReset)
	{
		set_vol(curl, curr, SOUND_MIXER_VOLUME);
		set_vol(curpcml, curpcmr, SOUND_MIXER_PCM);
	}
	
	gtk_main_quit();
	return(FALSE);
}

gint on_mute_clicked (GtkButton *button, gpointer user_data)
{
	PlayAlarmStop = TRUE;
	set_buzzer (0, 0);
	return(FALSE);
}

player_t
player_new (int volume_value)
{
	player_t p = g_malloc0 (sizeof (struct player));
	char *src ;
	char *sink ;
	
	src = "filesrc";
	sink = "esdsink";
	p->source_elem = g_strdup (src);
	p->sink_elem = g_strdup (sink);
	if (volume_value>0)
	{
		p->volume_value=volume_value;
	}
	else
	{
		p->volume_value=256; /* Set to the max! */
	}
	if (gst_handle)
	{
		void (*gst_init)(int *argc, char **argv[]);
		gst_init = dlsym(gst_handle, "gst_init");
		gst_init(NULL,NULL);
	} 
	else
	{
		printf("ERROR!\n");
	}
	return p;
}


/* Stolen from gpe-nmf :) , thank you philip */
void play_url(player_t p, char *url)
{
	if (gst_handle)
	{
		GstElement* (*gst_thread_new)(const gchar *name);
		GstElement* (*gst_element_factory_make)(const gchar *factoryname,
				const gchar *name);
		
		void   (*gst_bin_add_many)(GstBin *bin,GstElement *element_1,...);
		gboolean  (*gst_element_link_many)(GstElement *element_1,
				GstElement *element_2,...);
		GstState (*gst_element_set_state) (GstElement *element,
				GstState state);
		
		gst_thread_new = dlsym(gst_handle,"gst_thread_new");
		gst_element_factory_make = dlsym(gst_handle,"gst_element_factory_make");
		gst_bin_add_many = dlsym(gst_handle,"gst_bin_add_many");
		gst_element_link_many = dlsym(gst_handle, "gst_element_link_many");
		gst_element_set_state = dlsym(gst_handle, "gst_element_set_state");
		
		p->thread = (*gst_thread_new) ("thread");
		
		p->filesrc   = gst_element_factory_make (p->source_elem, "disk_source");
		p->decoder   = gst_element_factory_make ("spider", "decoder");
		p->volume    = gst_element_factory_make ("volume", "volume");
		p->audiosink = gst_element_factory_make (p->sink_elem , "play_audio");
		g_object_set (G_OBJECT (p->filesrc), "location", url, NULL);
		
		player_set_volume(p,p->volume_value);
		gst_bin_add_many ( (p->thread), p->filesrc, p->decoder, p->volume, p->audiosink, NULL);
		gst_element_link_many (p->filesrc, p->decoder, p->volume, p->audiosink, NULL );
		gst_element_set_state (p->thread, GST_STATE_PLAYING);
	}
}


static void
player_stop (player_t p)
{	
	if (gst_handle)
	{
		void (* gst_element_set_state)();
		gst_element_set_state = dlsym(gst_handle,"gst_element_set_state");
		if (p->thread)
		{
			gst_element_set_state (p->thread, GST_STATE_NULL);
		}
	}
}

void
player_set_volume (player_t p, int v)
{
	GValue value;
	
	memset (&value, 0, sizeof (value));
	g_value_init (&value, G_TYPE_FLOAT);
	g_value_set_float (&value, (float)v / 256);
	
	g_object_set_property (G_OBJECT (p->volume), "volume", &value);
}
