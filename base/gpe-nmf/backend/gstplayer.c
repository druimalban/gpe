/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "player.h"
#include <gst/gst.h>

#include <libintl.h>
#include <assert.h>

#define _(x) gettext(x)

struct player
{
  struct playlist *list;
  struct playlist *cur;
  guint idx;

  GstElement *filesrc, *decoder, *audiosink, *thread;
};

player_t
player_new (void)
{
  player_t p = g_malloc (sizeof (struct player));
  memset (p, 0, sizeof (struct player));

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
  p->cur = NULL;
}

gboolean
play_track (player_t p, struct playlist *t)
{
  return TRUE;
}

static void
player_run_track (player_t p)
{
  struct playlist *l = p->cur;

  assert (l);
  assert (!p->thread);
  assert (!p->filesrc);  
  assert (!p->decoder);
  assert (!p->audiosink);

  p->thread = gst_thread_new ("thread");

  p->filesrc = gst_element_factory_make ("filesrc", "disk_source");
  g_object_set (G_OBJECT (p->filesrc), "location", l->data.track.url, NULL);
  
  p->decoder = gst_element_factory_make ("spider", "decoder");

  p->audiosink = gst_element_factory_make ("esdsink", "play_audio");

  gst_bin_add_many (GST_BIN (p->thread), p->filesrc, p->decoder, p->audiosink, NULL);

  gst_element_connect (p->filesrc, p->decoder);
  gst_element_connect (p->decoder, p->audiosink);

  gst_element_set_state (p->thread, GST_STATE_PLAYING);
}

gboolean
player_play (player_t p)
{
  return TRUE;
}

void
player_status (player_t p, struct player_status *s)
{
  memset (s, 0, sizeof (*s));
}

void 
player_next_track (player_t p)
{
}

void 
player_prev_track (player_t p)
{
}

void 
player_set_volume (player_t p, int v)
{
}

void
player_fill_in_playlist (struct playlist *t)
{
}
