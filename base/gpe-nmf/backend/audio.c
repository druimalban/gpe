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
#include <stdio.h>

#include "audio.h"

#define ABUFSIZ 512

struct audio
{
  int fd;
  int volume;
  int rate;
  int channels;

  signed short obuf[ABUFSIZ];
};

static gboolean
audio_open_stream (audio_t a)
{
  if (a->fd != -1)
    close (a->fd);
  a->fd = esd_play_stream_fallback (ESD_BITS16 | (a->channels == 2 ? ESD_STEREO : 0) | ESD_STREAM | ESD_PLAY, 
				    a->rate, NULL, NULL);
  if (a->fd < 0)
    {
      g_free (a);
      return FALSE;
    }
  return TRUE;
}

audio_t
audio_open (void)
{
  audio_t a = g_malloc (sizeof (struct audio));
  a->rate = 44100;
  a->volume = 256;
  a->channels = 2;
  a->fd = -1;
  return a;
}

void
audio_set_rate (audio_t a, guint rate)
{
  if (a->rate != rate)
    {
      fprintf (stderr, "Set rate to %d\n", rate);
      a->rate = rate;
      audio_open_stream (a);
    }
}

void
audio_set_channels (audio_t a, guint channels)
{
  if (a->channels != channels)
    {
      a->channels = channels;
      audio_open_stream (a);
    }
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

  if (a->fd == -1)
    {
      fprintf (stderr, "warning: stream wasn't open\n");
      if (audio_open_stream (a) == FALSE)
	{
	  g_free (a);
	  return FALSE;
	}
    }


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
      if (op == ABUFSIZ)
	{
	  rv = write (a->fd, obuf, ABUFSIZ * 2);
	  if (rv != ABUFSIZ * 2)
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
