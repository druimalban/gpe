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

static gboolean play_track (player_t p, struct playlist *t);

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

static void
abandon_track (player_t p)
{
  gst_element_set_state (p->thread, GST_STATE_NULL);  
}

void
player_stop (player_t p)
{
  if (p->thread)
    abandon_track (p);

  p->cur = NULL;
}

static void
error_callback (GObject *object, GstObject *orig, gchar *error, player_t p)
{
  g_print ("ERROR: %s: %s\n", GST_OBJECT_NAME (orig), error);

  gst_element_set_state (GST_ELEMENT (orig), GST_STATE_NULL);  
}

static void
state_change (GstElement *elt, GstElementState old_state, GstElementState new_state, player_t p)
{
  fprintf (stderr, "State changed: %d -> %d\n", old_state, new_state);

  if (old_state == GST_STATE_PLAYING && new_state == GST_STATE_PAUSED)
    {
      printf ("Play finished.\n");
      gst_element_set_state (elt, GST_STATE_NULL);
      p->thread = NULL;
    }
  else if (new_state == GST_STATE_NULL)
    {
      printf ("Next track.\n");
      
      p->idx++;
      p->cur = playlist_fetch_item (p->list, p->idx);
      
      if (p->cur)
	play_track (p, p->cur);
      else
	{
	  p->idx = 0;
	  fprintf (stderr, "No more tracks.\n");
	}
    }
}

static gboolean
play_track (player_t p, struct playlist *t)
{
  assert (t);
  assert (!p->thread);

  fprintf (stderr, "play %s\n", t->data.track.url);

  p->thread = gst_thread_new ("thread");
  g_signal_connect (p->thread, "state_change", G_CALLBACK (state_change), p);

  g_signal_connect (p->thread, "error", G_CALLBACK (error_callback), p);

  p->filesrc = gst_element_factory_make ("filesrc", "disk_source");
  g_object_set (G_OBJECT (p->filesrc), "location", t->data.track.url, NULL);
  
  p->decoder = gst_element_factory_make ("spider", "decoder");

  p->audiosink = gst_element_factory_make ("esdsink", "play_audio");

  gst_bin_add_many (GST_BIN (p->thread), p->filesrc, p->decoder, p->audiosink, NULL);

  gst_element_connect (p->filesrc, p->decoder);
  gst_element_connect (p->decoder, p->audiosink);

  gst_element_set_state (p->thread, GST_STATE_PLAYING);

  return TRUE;
}

gboolean
player_play (player_t p)
{
  if (p->list)
    {
      p->idx = 0;
      
      p->cur = playlist_fetch_item (p->list, p->idx);
      
      if (p->cur)
	play_track (p, p->cur);
    }

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
