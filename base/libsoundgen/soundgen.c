/* soundgen.c - Generate sine-wave sounds and play them on /dev/dsp.

   Copyright (C) 2002 by Nils Faerber <nils@kernelconcepts.de>

   You should have received a copy of the GNU Library General Public
   License along with the this library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <esd.h>
#include <pthread.h>
#include <signal.h>
#include <glib.h>
#include <string.h>

#include "soundgen.h"
#include "buzzer.h"

#define DEBUG 1

#define SAMPLE_RATE 44100
#define BITS 16
#define BUZZER_FILE "/dev/misc/buzzer"
#define MIXER "/dev/mixer"
#define CFG_NOSOUND 	-2
#define CFG_AUTOMATIC 	-1
#define CFG_NAUTO 	-3

/* Flag to stop alarm sound thread from playing		*/
static gboolean PlayAlarmStop = TRUE;

/* Flag for current sound configuration */
static int sound_config;

int fd;
int curl, curr, curpcml, curpcmr;	
pthread_t SoundThread;

int buzzerfd = -1;

static short *_tone1_buffer, *_tone2_buffer;

void soundgen_open_buzzer (void)
{
	buzzerfd = open (BUZZER_FILE, O_WRONLY);
}

int soundgen_set_buzzer (int on, int off)
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

void soundgen_buzzer_off (int sig)
{
	soundgen_set_buzzer (0, 0);
}

/* 
 * This reads configuration from configfile. 
 * It returns a (positive) number from 0 to 100 to indicate a fixed volume
 * or a negative constant to indicate automatic volume setting (default)
 * or disabled alarm sound.
 */
int soundgen_get_config (void)
{
	int result = CFG_AUTOMATIC;
	gchar *filename = g_strdup_printf("%s/.gpe/alarm.conf", g_get_home_dir());
	FILE *cfgfile;
	gboolean enabled = TRUE;
	gboolean automatic = TRUE;
	gint level = 0;
	GKeyFile *alarmfile;
	GError *err = NULL;
	gint i;
	
	alarmfile = g_key_file_new();
	
	if (!g_key_file_load_from_file (alarmfile, filename,
                                    G_KEY_FILE_NONE, &err))
	{
		g_printerr ("%s\n", err->message);
		g_error_free(err);
		if (alarmfile)
			g_key_file_free (alarmfile);
	}
	else
	{
		i = g_key_file_get_boolean (alarmfile, "Settings", "enabled", &err);
		if (err) 
		{
			g_error_free(err);
			err = NULL;
		}
		else
			enabled = i;
		
		i = g_key_file_get_boolean (alarmfile, "Settings", "automatic", &err);
		if (err) 
		{
			g_error_free(err);
			err = NULL;
		}
		else
			automatic = i;
	
		i = g_key_file_get_integer (alarmfile, "Settings", "level", &err);
		if (err) 
		{
			g_error_free(err);
			err = NULL;
		}
		else
			if ((i >= 0) && (i <=100))
				level = i;
		}
		if (alarmfile)
			g_key_file_free (alarmfile);
		
	}
	if (!enabled) 
		result = CFG_NOSOUND;
	else
		if (!automatic)
			result = level;
		
	g_free(filename);
	
	return result;	
}

int soundgen_get_vol(int *left, int *right, int channel)
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


int soundgen_set_vol(int left, int right, int channel)
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

void soundgen_play_tone1(int snddev, unsigned int freq, unsigned int duration)
{
static unsigned int lfreq=0;
static double OneGo;
static double PiFrac;
double sineval;
unsigned int pduration,aduration;
int i;

	if ((lfreq != freq) || (_tone1_buffer == NULL)) {
		lfreq=freq;

		OneGo=(double)SAMPLE_RATE / (double)freq;
		PiFrac=(M_PI * 2) / OneGo;
		_tone1_buffer=NULL;
		_tone1_buffer=(short *)malloc((2 * (unsigned int)OneGo + 1) * sizeof(short));

		for (i=0; i<=OneGo; i++) {
			sineval=sin(i*PiFrac) * (double)32768;
			_tone1_buffer[i*2]=(short)sineval;
			_tone1_buffer[(i*2)+1]=(short)sineval;
		}
	}

	aduration = 0;
	pduration = duration * (SAMPLE_RATE / 250);

	while (aduration < pduration) {
		aduration+=write(snddev,_tone1_buffer,(unsigned int)OneGo * 4);
	}
}



void soundgen_play_tone2(int snddev, unsigned int freq, unsigned int duration)
{
static unsigned int lfreq=0;
static double OneGo;
static double PiFrac;
double sineval;
unsigned int pduration,aduration;
int i;

	if ((lfreq != freq) || (_tone2_buffer == NULL)) {
		lfreq=freq;

		OneGo=(double)SAMPLE_RATE / (double)freq;
		PiFrac=(M_PI * 2) / OneGo;
		_tone2_buffer=NULL;
		_tone2_buffer=(short *)malloc((2 * (unsigned int)OneGo + 1) * sizeof(short));

		for (i=0; i<=OneGo; i++) {
			sineval=sin(i*PiFrac) * (double)32768;
			_tone2_buffer[i*2]=(short)sineval;
			_tone2_buffer[(i*2)+1]=(short)sineval;
		}
	}

	aduration = 0;
	pduration = duration * (SAMPLE_RATE / 250);

	while (aduration < pduration) {
		aduration+=write(snddev,_tone2_buffer,(unsigned int)OneGo * 4);
	}
}

void soundgen_pause(int snddev, unsigned int duration)
{
unsigned short *nullbuffer;
size_t bufsize;

	bufsize= 2 * duration * (SAMPLE_RATE / 2500) * sizeof(short);
	nullbuffer=(unsigned short *)malloc(bufsize);
	memset(nullbuffer,0,bufsize);
	write(snddev,nullbuffer,bufsize);
	free(nullbuffer);
}

int soundgen_init(void)
{
int dspfd;

        if ((dspfd = esd_play_stream_fallback(ESD_STEREO | ESD_BITS16 | ESD_STREAM | ESD_PLAY, 44100, NULL, NULL)) < 0) {
#ifdef DEBUG
		perror("error opening /dev/dsp");
#endif
		return (-1);
	}

	_tone1_buffer = NULL;
	_tone2_buffer = NULL;

return dspfd;
}


int soundgen_final(int snddev)
{
	if (_tone1_buffer != NULL) {
		free(_tone1_buffer);
		_tone1_buffer=NULL;
	}
	if (_tone2_buffer != NULL) {
		free(_tone2_buffer);
		_tone2_buffer=NULL;
	}
	if (close(snddev) != 0) {
#ifdef DEBUG
		perror("soundgen_final");
#endif
		return (-1);
	} else {
		return 0;
	}
}

int soundgen_alarm_stop ()
{
	PlayAlarmStop = TRUE;
	soundgen_buzzer_off(0);
	pthread_join(SoundThread, NULL);
	if (sound_config == CFG_AUTOMATIC)
	{
		soundgen_set_vol(curl, curr, SOUND_MIXER_VOLUME);
		soundgen_set_vol(curpcml, curpcmr, SOUND_MIXER_PCM);
	}
	
	return(FALSE);
}

void soundgen_play_melody(int snd_dev, guint Tone1Pitch, guint Tone1Duration,
		 guint Tone2Enable, guint Tone2Pitch, guint Tone2Duration,
		 guint ToneAltCount, guint TonePause)
{
	int i;
	int times=0;
	
	if (sound_config == CFG_NOSOUND)
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
					soundgen_set_vol(50,50,SOUND_MIXER_VOLUME);
					break;
				case 1:
					soundgen_set_vol(55,55,SOUND_MIXER_VOLUME);
					break;
				case 2:
					soundgen_set_vol(60,60,SOUND_MIXER_VOLUME);
					break;
				case 3:
					soundgen_set_vol(65,65,SOUND_MIXER_VOLUME);
					break;
				case 4:
					soundgen_set_vol(70,70,SOUND_MIXER_VOLUME);
					break;
				case 5:
					soundgen_set_vol(75,75,SOUND_MIXER_VOLUME);
					break;
				case 6:
					soundgen_set_vol(80,80,SOUND_MIXER_VOLUME);
					break;
				case 7:
					soundgen_set_vol(85,85,SOUND_MIXER_VOLUME);
					break;
				case 8:
					soundgen_set_vol(90,90,SOUND_MIXER_VOLUME);
					break;
				case 9:
					soundgen_set_vol(95,95,SOUND_MIXER_VOLUME);
					break;
				case 10:
					soundgen_set_vol(100,100,SOUND_MIXER_VOLUME);
					break;
				default:
					break;
			}
					
			times++;
			if (times>20) PlayAlarmStop = TRUE;

	}
	soundgen_final(snd_dev);
	
}

void *soundgen_play_alarm()
{
	int snd_dev;

	/* Play Ambulance `melody` */
	snd_dev=soundgen_init();
	soundgen_play_melody(snd_dev,800,400,1,1100,400,2,2000);
	pthread_exit(NULL);
	
	return (NULL);
}

int soundgen_alarm_start ()
{
	sound_config = soundgen_get_config();

	if (sound_config == CFG_AUTOMATIC) 
		if((soundgen_get_vol(&curl, &curr, SOUND_MIXER_VOLUME) == -1)
			|| (soundgen_get_vol(&curpcml, &curpcmr, SOUND_MIXER_PCM) == -1))
				sound_config = CFG_NAUTO;  /* could not get volume, so we will not try to change it */

	
	soundgen_open_buzzer ();
  
	signal (SIGINT, soundgen_buzzer_off);

	soundgen_set_buzzer (1000, 500);
	
	if (sound_config != CFG_NOSOUND)
	{
		if (sound_config == CFG_AUTOMATIC)
		{
			soundgen_set_vol(100, 100, SOUND_MIXER_PCM);
			soundgen_set_vol(50, 50, SOUND_MIXER_VOLUME);
		}
		else if (sound_config != CFG_NAUTO)
		{
			soundgen_set_vol(100, 100, SOUND_MIXER_PCM);
			soundgen_set_vol(sound_config, sound_config, SOUND_MIXER_VOLUME);
		}
	}
	PlayAlarmStop = FALSE;
	if (pthread_create(&SoundThread, NULL, soundgen_play_alarm, NULL) != 0)
		g_print("pthread_create() failed\n");

	return(1);
}
