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
