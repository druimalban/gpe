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

typedef enum
  {
    PLAYER_STATE_NULL,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_STOPPING,
    PLAYER_STATE_NEXT_TRACK
  } player_state;

struct player
{
  struct playlist *list;
  struct playlist *cur;
  guint idx;
  gboolean new_track;

  player_state state;
  GstElement *filesrc, *decoder, *audiosink, *thread;
  GstScheduler *sched;
  GstClock *clock;
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
  p->state = PLAYER_STATE_STOPPING;
  if (p->thread)
    {
      gst_element_set_state (p->thread, GST_STATE_NULL);
    }
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
thread_shutdown (GstElement *elt, player_t p)
{
  player_state old_player_state = p->state;

  fprintf (stderr, "player %p: thread has stopped.\n", p);

  p->thread = NULL;
  p->state = PLAYER_STATE_NULL;

  if (old_player_state == PLAYER_STATE_NEXT_TRACK)
    {
      fprintf (stderr, "player %p: next track.\n", p);
      
      p->idx++;
      p->cur = playlist_fetch_item (p->list, p->idx);
      
      if (p->cur)
	play_track (p, p->cur);
      else
	{
	  p->idx = 0;
	  fprintf (stderr, "player %p: no more tracks.\n", p);
	}
    }
}

static void
state_change (GstElement *elt, GstElementState old_state, GstElementState new_state, player_t p)
{
  fprintf (stderr, "player %p: elt %p state %d -> %d\n", p, elt, old_state, new_state);

  if (new_state == GST_STATE_PLAYING)
    {
      p->state = PLAYER_STATE_PLAYING;
      p->sched = GST_ELEMENT_SCHED (p->thread);
      p->clock = gst_scheduler_get_clock (p->sched);
    }
  else if (old_state == GST_STATE_PLAYING && new_state == GST_STATE_PAUSED)
    {
      fprintf (stderr, "player %p: Play finished.\n", p);
      if (p->state == PLAYER_STATE_PLAYING)
	{
	  gst_element_set_state (elt, GST_STATE_NULL);
	  p->state = PLAYER_STATE_NEXT_TRACK;
	}
    }
}

static void
metadata_notify (GObject *obj, GObject *the_obj, GParamSpec *spec, player_t p)
{
  GstCaps *caps;
  GValue value;

  memset (&value, 0, sizeof (value));
  g_value_init (&value, GST_TYPE_CAPS);
  g_object_get_property (G_OBJECT (the_obj), "metadata", &value);

  caps = g_value_get_boxed (&value);
  fprintf (stderr, "player %p: elt %p reports metadata %p\n", p, obj, caps);
  gst_caps_debug (caps, NULL);

  /* If the metadata was all we wanted, stop the thread */
  if (p->cur == NULL)
    {
      p->state = PLAYER_STATE_STOPPING;
      gst_element_set_state (p->thread, GST_STATE_NULL);
    }
}

static void
build_pipeline (player_t p, struct playlist *t, gboolean really_play)
{
  p->filesrc = gst_element_factory_make ("filesrc", "disk_source");
  g_object_set (G_OBJECT (p->filesrc), "location", t->data.track.url, NULL);
  
  p->decoder = gst_element_factory_make ("spider", "decoder");

  p->audiosink = gst_element_factory_make (really_play ? "esdsink" : "fakesink", "play_audio");

  gst_bin_add_many (GST_BIN (p->thread), p->filesrc, p->decoder, p->audiosink, NULL);

  gst_element_connect (p->filesrc, p->decoder);
  gst_element_connect (p->decoder, p->audiosink);
}

static gboolean
play_track (player_t p, struct playlist *t)
{
  assert (t);
  assert (!p->thread);

  fprintf (stderr, "player %p: play item %p [%s]\n", p, t, t->data.track.url);

  p->new_track = TRUE;

  p->thread = gst_thread_new ("thread");

  build_pipeline (p, t, TRUE);

  g_signal_connect (p->thread, "shutdown", G_CALLBACK (thread_shutdown), p);
  g_signal_connect (p->thread, "state_change", G_CALLBACK (state_change), p);
  g_signal_connect (p->thread, "error", G_CALLBACK (error_callback), p);
  g_signal_connect (p->thread, "deep_notify::metadata", G_CALLBACK (metadata_notify), p);

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

  s->item = p->cur;
  s->changed = p->new_track;
  p->new_track = FALSE;

  if (p->state == PLAYER_STATE_PLAYING)
    {
      GstClockTime t = gst_clock_get_time (p->clock);
      fprintf (stderr, "player %p: tick %llx\n", p, t);
      s->time = t;
    }
}

static void
step_track (player_t p, int n)
{
  assert (p);
  assert (n != 0);

  if (p->state == PLAYER_STATE_PLAYING)
    {
      abandon_track (p);
      
      if (n > 0)
	{
	  p->idx++;
	}
      else if (n < 0)
	{
	  if (p->idx)
	    p->idx--;
	}

      p->cur = playlist_fetch_item (p->list, p->idx);
      if (p->cur)
	play_track (p, p->cur);
    }
}

void 
player_next_track (player_t p)
{
  step_track (p, 1);
}

void 
player_prev_track (player_t p)
{
  step_track (p, -1);
}

void 
player_set_volume (player_t p, int v)
{
}

#if 0
static void
fill_state_change (GstElement *elt, GstElementState old_state, GstElementState new_state, player_t p)
{
  fprintf (stderr, "State changed: %d -> %d\n", old_state, new_state);
}
#endif

void
player_fill_in_playlist (struct playlist *t)
{
#if 0
  struct player player_state;
  player_t p = &player_state;

  memset (p, 0, sizeof (*p));

  p->thread = gst_pipeline_new ("pipeline");

  build_pipeline (p, t, FALSE);

  g_signal_connect (p->thread, "state_change", G_CALLBACK (fill_state_change), p);
  g_signal_connect (p->thread, "error", G_CALLBACK (error_callback), p);
  g_signal_connect (p->thread, "deep_notify::metadata", G_CALLBACK (metadata_notify), p);

  gst_element_set_state (p->thread, GST_STATE_PLAYING);

  while (gst_bin_iterate (GST_BIN (p->thread)));

  gst_element_set_state (p->thread, GST_STATE_NULL);
  
  gst_object_unref (GST_OBJECT (p->thread));
#endif
}
