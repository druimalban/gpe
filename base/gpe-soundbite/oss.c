/*   
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

int 
sound_device_open (int mode)
{
    int speed = 16000;
    int channels = 2;
    int format = AFMT_S16_LE;
    int fd;

    fd = open ("/dev/dsp", mode);
    if (fd < 0)
      {
	perror ("/dev/dsp");
	return -1;
      }
    if (ioctl (fd, SNDCTL_DSP_SPEED, &speed))
      {
	perror ("SNDCTL_DSP_SPEED");
	return -1;
      }
    if (ioctl (fd, SNDCTL_DSP_CHANNELS, &channels))
      {
	perror ("SNDCTL_DSP_CHANNELS");
	return -1;
      }
    if (ioctl (fd, SNDCTL_DSP_SETFMT, &format))
      {
	perror ("SNDCTL_DSP_SETFMT");
	return -1;
      }
    
    return fd;
}
