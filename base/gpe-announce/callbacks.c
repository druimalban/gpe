#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#include "announce.h"
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <pthread.h>

#include <gpe/libgpe.h>

#define MIXER "/dev/mixer"

#define SIZE 256
#define SNOOZE 5

/* Flag to stop alarm sound thread from playing		*/
static gboolean PlayAlarmStop = TRUE;

int fd;
int curl, curr;	
	
void play_melody(guint Tone1Pitch, guint Tone1Duration,
		guint Tone2Enable, guint Tone2Pitch, guint Tone2Duration,
		guint ToneAltCount, guint TonePause)
{
int gpe_snd_dev=-1;
int i;

	for (i=0; i<5; i++) {
		if ((gpe_snd_dev=gpe_soundgen_init()) == -1) {
			g_print("Couldn't init gpe_soundgen\n");
			sleep (1);
		} else
			break;
	}
	if (gpe_snd_dev == -1)
		return;
	
	while (!PlayAlarmStop) {
		for (i = 0; i < ToneAltCount; i++) {
			if (PlayAlarmStop)
				break;
			gpe_soundgen_play_tone1(gpe_snd_dev, Tone1Pitch, Tone1Duration);
			gpe_soundgen_play_tone2(gpe_snd_dev, Tone2Pitch, Tone2Duration);
		
		}
		gpe_soundgen_pause(gpe_snd_dev, TonePause);
	}
	gpe_soundgen_final(gpe_snd_dev);
	
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

int get_vol(int fd, int *left, int *right)
{
  int vol;
  int err = ioctl(fd, SOUND_MIXER_READ_VOLUME, &vol);  
  if(err != -1) {
    *left = vol & 0xff;
    *right = (vol >> 8) & 0xff;
  }
  return err;
}


int set_vol(int fd, int left, int right)
{
  int vol = left | (right << 8);
  int err = ioctl(fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &vol);  
  return err;
}

gint bells_and_whistles ( )
{
	char *mixer = MIXER;
  	pthread_t mythread;
	
	fd = open(mixer, O_RDONLY);
  	if(fd == -1) 
    		printf("Unable to open mixer device: %s\n", mixer);
  	
  	if(get_vol(fd, &curl, &curr) == -1)
    		printf("Unable to get volume\n");
   	
	set_vol(fd, 100, 100);
	
	PlayAlarmStop = FALSE;
	if (pthread_create(&mythread, NULL, play_alarm, NULL) != 0) {
		g_print("pthread_create() failed\n");
	} else
		pthread_detach(mythread);
	
	return(1);
	
}

gint
on_snooze_clicked                     (GtkButton       *button,
                                        char         *user_data)
{
	struct tm tm;
	time_t viewtime;

  	time (&viewtime);
  	gmtime_r(&viewtime, &tm);
   
	CurrentAlarm=(struct Alarm_t *)malloc(sizeof(struct Alarm_t));
	memset(CurrentAlarm,0,sizeof(struct Alarm_t));
	CurrentAlarm->year=tm.tm_year + 1900;
	CurrentAlarm->month=tm.tm_mon;
	CurrentAlarm->day=tm.tm_mday;
	CurrentAlarm->hour=tm.tm_hour;
	CurrentAlarm->minute=tm.tm_min+5;
	CurrentAlarm->AlarmType=6; /* melody 3 */
	CurrentAlarm->AlarmReoccurence=0; /* once */
	CurrentAlarm->Tone2Enable=TRUE;
	tm.tm_isdst=1;
	tm.tm_min+=5;
		      
	rtcd_set_alarm_tm("gpe-announce", user_data, &tm, sizeof(struct Alarm_t),CurrentAlarm);
  
	PlayAlarmStop = TRUE;
	set_vol(fd, curl, curr);
	gtk_main_quit();
	return(FALSE);
}


gint
on_ok_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	PlayAlarmStop = TRUE;
	set_vol(fd, curl, curr);
	gtk_main_quit();
	return(FALSE);
}

gint
on_mute_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	PlayAlarmStop = TRUE;
	return(FALSE);
}

