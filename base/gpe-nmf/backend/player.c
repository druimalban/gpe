/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "player.h"
#include "decoder.h"
#include "stream.h"
#include <esd.h>

int output;

struct player
{
  struct playlist *list;
  struct playlist *cur;
  guint idx;

  decoder_t decoder;
  stream_t stream;
  audio_t audio;
};

player_t
player_new (void)
{
  player_t p = g_malloc (sizeof (struct player));
  memset (p, 0, sizeof (struct player));
  p->audio = audio_open ();
  return p;
}

void
player_destroy (player_t p)
{
  player_stop (p);
 
  g_free (p);
}

void
player_set_playlist (player_t p, struct playlist *l)
{
  player_stop (p);

  p->list = l;
  p->idx = 0;
}

struct playlist *
player_get_playlist (player_t p)
{
  return p->list;
}

void
player_stop (player_t p)
{
  if (p->decoder)
    {
      decoder_close (p->decoder);
      p->decoder = NULL;
    }

  if (p->stream)
    {
      stream_close (p->stream);
      p->stream = NULL;
    }
}

void
player_play (player_t p)
{
  player_stop (p);

  p->cur = playlist_fetch_item (p->list, p->idx);
  if (p->cur)
    {
      p->stream = stream_open (p->cur->data.track.url);
      if (p->stream == NULL)
	{
	  p->cur = NULL;
	  return;
	}
      p->decoder = decoder_open (p->cur->data.track.url, p->stream, p->audio);
      if (p->decoder == NULL)
	{
	  stream_close (p->stream);
	  p->stream = NULL;
	  p->cur = NULL;
	  return;
	}
    }
}

void
player_status (player_t p, struct player_status *s)
{
  struct decoder_stats ds;
  s->item = p->cur;
  if (p->decoder)
    {
      decoder_stats (p->decoder, &ds);
      s->time = ds.time;
      s->finished = ds.finished;
    }
  else
    s->finished = TRUE;
}

void 
player_next_track (player_t p)
{
  if (p->cur)
    {
      player_stop (p);
      p->idx++;
      player_play (p);
    }
}

void 
player_prev_track (player_t p)
{
  if (p->idx)
    {
      player_stop (p);
      p->idx--;
      player_play (p);
    }
}

void 
player_set_volume (player_t p, int v)
{
  audio_set_volume (p->audio, v);
}
