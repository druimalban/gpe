/*
 * gpe-conf
 *
 * Copyright (C) 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Some bits are taken from gpe-mixer.
 *
 * GPE sound settings module, backend.
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

#include <libintl.h>
#define _(x) gettext(x)

#include "soundctrl.h"

#define DEVNAME_MIXER "/dev/mixer"
#define DEVNAME_AUDIO "/dev/audio"
static char* FILE_SETTINGS = NULL;

static int initialized = FALSE;

static char* mixer_names[] = SOUND_DEVICE_NAMES;
static char* mixer_labels[] = SOUND_DEVICE_LABELS;
static t_mixer mchannels[SOUND_MIXER_NRDEVICES];

static int devmask = 0, recmask = 0, recsrc = 0;
static int mixfd;
static int active_channels = 0;

static char* show_channels[] = {"vol", "pcm", "mic"};


void
play_sample(char *filename)
{
	int fd_dev;
	int fd_data = open(filename, O_RDONLY);
	int len;
	char buf[20000];

	if (fd_data < 0)
		return;
	else
		len = read(fd_data, buf, 20000);	
	close(fd_data);
	
	fd_dev = open(DEVNAME_AUDIO, O_WRONLY | O_NONBLOCK);
	if (len > 0)
		if (fd_dev >= 0)
		{
			write(fd_dev, buf, len);
			close(fd_dev);
		}
}


static gboolean 
channel_active(const gchar *name)
{
	int i = 0;
	
	if (!name) 
		return FALSE;

	if (!show_channels)
		return TRUE;
	
	while (show_channels[i])
		if (!strcmp(show_channels[i++], name))
			return TRUE;
		
	return (FALSE);
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
		while (fgets(buf, 128, cfgfile))
		{
			ret = sscanf(buf, "%d %d", &nc, &val);			
			if ((ret == 2) && (nc >= 0) && (nc <= SOUND_MIXER_NRDEVICES))
			{
				mchannels[nc].value = val;
				set_volume(mchannels[nc].nr, val);
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
		for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
			fprintf(cfgfile, "%d %d\n", i, mchannels[i].value);
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
		
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
	{
		if (((1 << i) & devmask) && channel_active(mixer_names[i])) 
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
		if (ioctl(mixfd, SOUND_MIXER_READ_RECMASK, &recmask) == -1) 
		{
			return;
		}
		if (ioctl(mixfd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1) 
		{
			return;
		}
	}
	
	if (mixfd < 0)
		return;
		
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
	{
		if (((1 << i) & devmask) && channel_active(mixer_names[i])) 
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
}


int 
sound_get_channels(t_mixer **channels)
{
	if (!initialized)
		return 0;
	*channels = mchannels;
	return active_channels;
}
