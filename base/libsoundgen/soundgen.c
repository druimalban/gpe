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
#include <errno.h>
#include <stdlib.h>
#include <esd.h>
#include <string.h>

#define DEBUG 1

#define SAMPLE_RATE 44100
#define BITS 16

static short *_tone1_buffer, *_tone2_buffer;

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

#if 0
int soundgen_blub(int argc, char **argv)
{
int dspfd, iocval, i;
unsigned int t1freq, t1dur, t2freq=0, t2dur=0, repeat;

	if (argc < 3) {
		exit(0);
	}
	t1freq=atoi(argv[1]);
	if (t1freq <= 0) {
		exit(0);
	}
	t1dur=atoi(argv[2]);
	if (t1dur <= 0) {
		exit(0);
	}
	if (argc > 4) {
		t2freq=atoi(argv[3]);
		t2dur=atoi(argv[4]);
		repeat=atoi(argv[5]);
	} else
		repeat=atoi(argv[3]);
	if (repeat <= 0) {
		exit(0);
	}

	for (i=0; i < repeat; i++) {
		soundgen_play_tone1(dspfd,t1freq,t1dur);
		if (t2freq != 0 && t2dur != 0)
			soundgen_play_tone2(dspfd,t2freq,t2dur);
	}

	close(dspfd);
return 0;
}
#endif
