#include <esd.h>
#include <glib.h>

#include "audio.h"

#define BUFSIZ 512

struct audio
{
  int fd;
  int volume;

  signed short obuf[BUFSIZ];
};

audio_t
audio_open (void)
{
  audio_t a = g_malloc (sizeof (struct audio));
  a->fd = esd_play_stream_fallback (ESD_BITS16 | ESD_STEREO | ESD_STREAM | ESD_PLAY, 44100, NULL, NULL);
  a->volume = 256;
  return a;
}

void
audio_set_volume (audio_t a, int volume)
{
  a->volume = volume;
}

gboolean
audio_write (audio_t a, void *buf, unsigned int nsamp)
{
  unsigned int op = 0;
  signed short *ibuf = buf;
  signed short *obuf = a->obuf;
  unsigned int volume = a->volume;
  int rv;

  nsamp /= 2;

  while (nsamp--)
    {
      obuf[op++] = *(ibuf++) * volume / 256;
      if (op == BUFSIZ)
	{
	  rv = write (a->fd, obuf, BUFSIZ * 2);
	  if (rv != BUFSIZ * 2)
	    return FALSE;
	  op = 0;
	}
    }

  if (op)
    {
      rv = write (a->fd, obuf, op * 2);
      if (rv != op * 2)
	return FALSE;
    }

  return TRUE;
}
