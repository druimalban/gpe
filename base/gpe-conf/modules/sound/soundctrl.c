/*
 * gpe-conf
 *
 * Copyright (C) 2004, 2006  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Some bits are taken from gpe-mixer.
 *
 * GPE sound settings module, audio backend.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gdk/gdk.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <esd.h>
#include <audiofile.h>

#include <libintl.h>
#define _(x) gettext(x)

#include "soundctrl.h"

#define DEVNAME_MIXER "/dev/mixer"
#define DEVNAME_AUDIO1 "/dev/sound/dsp"
#define DEVNAME_AUDIO2 "/dev/dsp"
#define DEVNAME_AUDIO3 "/dev/audio"

static char* FILE_SETTINGS = NULL;

static int initialized = FALSE;
static int muted = FALSE;

static char* mixer_names[] = SOUND_DEVICE_NAMES;
static char* mixer_labels[] = SOUND_DEVICE_LABELS;
static t_mixer mchannels[SOUND_MIXER_NRDEVICES];

static int devmask = 0;
static int mixfd;
static int active_channels = 0;

static char* show_channels[] = {"vol", "pcm", "mic", "pcm2", "bass", "treble", NULL};


void
play_sample(char *filename)
{
	if (filename && strlen(filename) && !access(filename, R_OK))
		esd_play_file( "wav", filename, 1 );
}


static int
channel_active(int chan)
{
	int i = 0;
	
	if (!mixer_names[chan]) 
		return FALSE;

	if (!((1 << chan) & devmask))
		return FALSE;
	
	if (!show_channels)
		return TRUE;
	
	while (show_channels[i])
		if (!strcmp(show_channels[i++], mixer_names[chan]))
			return TRUE;
		
	return (FALSE);
}


/* mute sound system, we store current settings in backup */
void 
set_mute_status(int set_mute)
{
	int i;
	muted = set_mute;
	if (muted)
	{
		for (i = 0; i < active_channels; i++)
		{
			mchannels[i].backupval = mchannels[i].value;
			mchannels[i].value = 0;
			set_volume (mchannels[i].nr, 0);
		}
	}
	else
	{
		for (i = 0; i < active_channels; i++)
		{
			mchannels[i].value = mchannels[i].backupval;
			set_volume (mchannels[i].nr, mchannels[i].value);
		}
	}
}


int
get_mute_status(void)
{
	return muted;
}


/* load and set values from file */
void 
sound_load_settings(void)
{
	FILE *cfgfile;
	
	cfgfile = fopen(FILE_SETTINGS, "r");
	if (cfgfile)
	{
		int nc = -1, val = -1, ret;
		char buf[128];
		
		memset(buf, 0, 128);
		
		while (fgets(buf, 128, cfgfile))
		{
			ret = sscanf(buf, "muted %d", &nc);
			if (ret)
				muted = nc;
			
			ret = sscanf(buf, "%d %d", &nc, &val);		
			
			memset(buf, 0, 128);
			if ((ret == 2) && (nc >= 0) && (nc <= SOUND_MIXER_NRDEVICES))
			{
				if (channel_active(mchannels[nc].nr) && !(nc && !mchannels[nc].nr))
				{
					if (muted)
					{
						set_volume(mchannels[nc].nr, 0);
						mchannels[nc].backupval = val;
						mchannels[nc].initval = val;
						mchannels[nc].value = val;
					}
					else
					{
						mchannels[nc].value = val;
						mchannels[nc].initval = val;
						set_volume(mchannels[nc].nr, val);
					}
				}
			}
		}
		fclose(cfgfile);
	}
}


/* save current settings to file */
void 
sound_save_settings(void)
{
	int i;
	FILE *cfgfile;
	
	cfgfile = fopen(FILE_SETTINGS, "w");
	if (cfgfile)
	{
		fprintf(cfgfile, 
		        "# mixer settings - file created by gpe-conf\n\n");
		fprintf(cfgfile, "muted %d\n", muted);
		for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
			fprintf(cfgfile, "%d %d\n", i, 
		            muted ? mchannels[i].backupval : mchannels[i].value);
		fclose(cfgfile);
	}
}

/* restore settings to initial settings at startup time */
void
sound_restore_settings(void)
{
	int i;
	
	if (!initialized)
		return;
		
	for (i = 0; i < active_channels; i++)
	{
		if (channel_active(mchannels[i].nr))
		{
			mchannels[i].value = mchannels[i].initval;
			set_volume(mchannels[i].nr, mchannels[i].value);
		}
	}
}


int
set_volume (int channel, int volume)
{
	volume |= (volume << 8);
	return ioctl(mixfd, MIXER_WRITE(channel), &volume);
}


int
get_volume (int channel)
{
	int value;
	
	if (ioctl(mixfd, MIXER_READ(channel), &value) != -1) 
		value = value & 0x7f;
	else
		value = 0;
	
	return value;
}

/* initializes sound subsystem and settings */
void
sound_init(void)
{
	int i;
	
	active_channels = 0;
	FILE_SETTINGS = 
		g_strdup_printf("%s/.gpe/sound.conf", g_get_home_dir());
	
	if ((mixfd = open(DEVNAME_MIXER, O_RDWR)) >= 0) 
	{
		if (ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1) 
		{
			return;
		}
	}
	
	if (mixfd < 0)
		return;
		
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
	{
		if (channel_active(i))
		{
			mchannels[active_channels].nr = i; /* channel number */
			if (i) /* name and label */
				mchannels[active_channels].label = mixer_labels[i];
			else 
				mchannels[active_channels].label = _("Volume");
			mchannels[active_channels].name = mixer_names[i];
			/* get initial value */
			mchannels[active_channels].value = get_volume(i);
			mchannels[active_channels].initval = 
				mchannels[active_channels].value;
			active_channels++;
		}
	}
	
	initialized = TRUE;
	
	/* restore all settings from configfile, e.g. mute setting */
	
	sound_load_settings();
	sound_restore_settings();
}


int 
sound_get_channels(t_mixer **channels)
{
	if (!initialized)
		return 0;
	*channels = mchannels;
	return active_channels;
}
