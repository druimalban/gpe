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
#include <libintl.h>
#include <assert.h>

#define _(x) gettext(x)

struct player
{
  struct playlist *list;
  struct playlist *cur;
  guint idx;

  decoder_t decoder;
  stream_t stream;
  audio_t audio;

  void (*error_func)(gchar *);
};

void
player_error (player_t player, gchar *s)
{
  if (player->error_func)
    player->error_func (s);
}

void
player_error_handler (player_t player, void (*error_func)(gchar *))
{
  player->error_func = error_func;
}

player_t
player_new (void)
{
  player_t p = g_malloc (sizeof (struct player));
  memset (p, 0, sizeof (struct player));
  p->audio = audio_open ();
  if (p->audio == NULL)
    {
      g_free (p);
      return NULL;
    }
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

void
player_set_index (player_t p, gint idx)
{
  assert (idx >= 0);
  
  p->idx = idx;
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

gboolean
player_play (player_t p)
{
  player_stop (p);

  if (p->list == NULL)
    {
      player_error (p, _("No playlist"));
      return FALSE;
    }

  p->cur = playlist_fetch_item (p->list, p->idx);
  if (p->cur)
    {
      p->stream = stream_open (p->cur->data.track.url);
      if (p->stream == NULL)
	{
	  player_error (p, _("Cannot open stream"));
	  p->cur = NULL;
	  return FALSE;
	}
      p->decoder = decoder_open (p, p->cur->data.track.url, p->stream, p->audio);
      if (p->decoder == NULL)
	{
	  player_error (p, _("Cannot decode this stream"));
	  stream_close (p->stream);
	  p->stream = NULL;
	  p->cur = NULL;
	  return FALSE;
	}
    }
  return TRUE;
}

struct stream *
player_next_stream (player_t p)
{
  stream_close (p->stream);
  p->stream = NULL;
  p->idx++;
  p->cur = playlist_fetch_item (p->list, p->idx);
  if (p->cur)
    {
      p->stream = stream_open (p->cur->data.track.url);
      if (p->stream == NULL)
	p->cur = NULL;
    }
      
  return p->stream;
}

void
player_status (player_t p, struct player_status *s)
{
  struct decoder_stats ds;
  s->changed = FALSE;
  if (p->decoder)
    {
      decoder_stats (p->decoder, &ds);
      if (ds.finished)
	{
	  s->time = 0;
	}
      else
	{
	  s->time = ds.time;
	  s->total_time = ds.total_time;
	  s->changed = TRUE;
	  s->sample_rate = ds.rate;
	}
    }
  else
    s->time = s->total_time = 0;

  s->item = p->cur;
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
