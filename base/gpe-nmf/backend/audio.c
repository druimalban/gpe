/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <esd.h>
#include <glib.h>
#include <unistd.h>

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
audio_set_volume (audio_t a, guint volume)
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

  if (volume == 256)
    {
      /* No scaling required */
      write (a->fd, buf, nsamp);
      return TRUE;
    }

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
