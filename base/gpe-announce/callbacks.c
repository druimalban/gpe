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

#include <gpe/soundgen.h>

#define MIXER "/dev/mixer"

#define SIZE 256
#define SNOOZE 5

/* Flag to stop alarm sound thread from playing		*/
static gboolean PlayAlarmStop = TRUE;

int fd;
int curl, curr;	

static void
schedule_alarm(char *buf, time_t when)
{
  char filename[256], command[256];
  FILE *f;
	
  sprintf(filename, "/var/spool/at/%d.1234", (int)when);
  
  f=fopen(filename, "w");
	
  fprintf(f, "#!/bin/sh\n");
  fprintf(f, "/usr/bin/gpe-announce '%s'\n", buf);
  fprintf(f, "/bin/rm $0\n");
  
  fclose(f);
  
  sprintf(command, "chmod 755 %s", filename);
  		  
  system(command);
  
  sprintf(command, "echo >/var/spool/at/trigger");
  		  
  system(command);
  
}
	
void play_melody(guint Tone1Pitch, guint Tone1Duration,
		guint Tone2Enable, guint Tone2Pitch, guint Tone2Duration,
		guint ToneAltCount, guint TonePause)
{
int snd_dev=-1;
int i;

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
			soundgen_play_tone1(snd_dev, Tone1Pitch, Tone1Duration);
			soundgen_play_tone2(snd_dev, Tone2Pitch, Tone2Duration);
		
		}
		soundgen_pause(snd_dev, TonePause);
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
	time_t viewtime;

  	time (&viewtime);
  	viewtime+=5*60;
		      
	schedule_alarm(user_data, viewtime);
  
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


