/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "player.h"
#include <gst/gst.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>
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
  int *shuffle_list;
  gboolean new_track;

  player_state state;
  GstElement *filesrc, *decoder, *audiosink, *thread, *volume;
  GstScheduler *sched;
  GstClock *clock;

  int opt_shuffle;
  int opt_loop;

  char *source_elem;
  char *sink_elem;
};

static gboolean play_track (player_t p, struct playlist *t);

player_t
player_new (void)
{
  player_t p = g_malloc0 (sizeof (struct player));
  char *src = getenv ("NMF_SRC");
  char *sink = getenv ("NMF_SINK");

  if (!src)
    src = "gnomevfssrc";
  if (!sink)
    sink = "esdsink";

  p->source_elem = g_strdup (src);
  p->sink_elem = g_strdup (sink);

  return p;
}

void
player_destroy (player_t p)
{
  player_stop (p);

  g_free (p->source_elem);
  g_free (p->sink_elem);
  g_free (p);
}

static void
create_shuffle_list (player_t p)
{
  int len;
  int *arr;
  int i;

  assert (p);

  if (p->shuffle_list)
    {
      free (p->shuffle_list);
      p->shuffle_list = NULL;
    }

  len = playlist_get_length (p->list);
  if (len == 0)
    return;

  srandom (time(NULL));

  arr = malloc (sizeof(int) * len);

  for (i=0;i<len;i++)
    arr[i] = i;

  for (i=0;i<len-1;i++)
    {
      int num, tmp;
      num = (random() % (len-i)) + i;
      tmp = arr[i];
      arr[i] = arr[num];
      arr[num] = tmp;
    }

  p->shuffle_list = arr;
}

void
player_set_playlist (player_t p, struct playlist *l)
{
  player_stop (p);

  p->list = l;
  p->idx = 0;

  create_shuffle_list (p);
}

void
player_set_index (player_t p, gint idx)
{
  assert (idx >= 0);

  if (p->opt_shuffle)
    {
      int i, len;
      p->idx = 0; /* just in case it's not in the list */
      len = playlist_get_length (p->list);

      for (i = 0; i < len; i++)
	{
	  if (p->shuffle_list[i] == idx)
	    p->idx = i;
	}
    } 
  else  
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
step_track (player_t p, int n)
{
  assert (p);
  assert (n != 0);

  /* This handles playing - what about paused? */
  if (p->state == PLAYER_STATE_PLAYING)
    abandon_track (p);

  if (n > 0)
    {
      p->idx++;
    }
  else if (n < 0)
    {
      if (p->idx)
	p->idx--;
      else if (p->opt_loop)
	p->idx = playlist_get_length(p->list) - 1;
    }

  if (p->opt_loop && playlist_get_length(p->list) == p->idx)
    p->idx = 0;

  if (p->opt_shuffle)
    p->cur = playlist_fetch_item (p->list, p->shuffle_list[p->idx]);
  else
    p->cur = playlist_fetch_item (p->list, p->idx);

  if (p->cur)
    play_track (p, p->cur);
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

      step_track (p, 1);
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
	  fprintf (stderr, "player %p: Setting element state to NULL.\n", p);
	  gst_element_set_state (elt, GST_STATE_NULL);
	  p->state = PLAYER_STATE_NEXT_TRACK;
	}
    }
}

static void
metadata_notify (GObject *obj, GObject *the_obj, GParamSpec *spec, player_t p)
{
//  GstCaps *caps;
  GValue value;

  memset (&value, 0, sizeof (value));
  g_value_init (&value, GST_TYPE_CAPS);
  g_object_get_property (G_OBJECT (the_obj), "metadata", &value);

#if 0 
  caps = g_value_get_boxed (&value);
  if (caps)
    {
      GstProps *props = gst_caps_get_props (caps);
     
      if (props)
	{
	  gchar *artist = NULL, *title = NULL, *album = NULL;
	  
	  if (gst_props_get (props, "ARTIST", &artist))
	    {
	      g_free (p->data.track.artist);
	      p->data.track.artist = artist;
	    }

	  if (gst_props_get (props, "TITLE", &title))
	    {
	      g_free (p->title);
	      p->title = title;
	    }

	  if (gst_props_get (props, "ALBUM", &album))
	    {
	      g_free (p->data.track.album);
	      p->data.track.album = album;
	    }
	}
    }
#endif

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
  p->filesrc = gst_element_factory_make (p->source_elem, "disk_source");
  p->decoder = gst_element_factory_make ("spider", "decoder");
  p->volume = gst_element_factory_make ("volume", "volume");
  p->audiosink = gst_element_factory_make (really_play ? p->sink_elem : "fakesink", "play_audio");

  if (!p->filesrc || !p->decoder || !p->volume || !p->audiosink)
    {
      fprintf (stderr, "Element creation failed\n");
      return;
    }

  g_object_set (G_OBJECT (p->filesrc), "location", t->data.track.url, NULL);

  gst_bin_add_many (GST_BIN (p->thread), p->filesrc, p->decoder, p->volume, p->audiosink, NULL);

  gst_element_connect (p->filesrc, p->decoder);
  gst_element_connect (p->decoder, p->volume);
  gst_element_connect (p->volume, p->audiosink);
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
      if (p->opt_shuffle)
	p->cur = playlist_fetch_item (p->list, p->shuffle_list[p->idx]);
      else
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
#if 0
      fprintf (stderr, "player %p: tick %llx\n", p, t);
#endif
      s->time = t;
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
  GValue value;

  memset (&value, 0, sizeof (value));
  g_value_init (&value, G_TYPE_FLOAT);
  g_value_set_float (&value, (float)v / 256);
  
  g_object_set_property (G_OBJECT (p->volume), "volume", &value);
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

void
player_set_loop (player_t p, int on)
{
  p->opt_loop = on;
}

void
player_set_shuffle (player_t p, int on)
{
  p->opt_shuffle = on;

  if (!p->shuffle_list)
    return;

  if (on)
    {
      /* Reset the shuffle list & put the current track at
       * the start of the list, then select it. */
      int len, i;
      create_shuffle_list (p);
      len = playlist_get_length (p->list);
      for (i=0;i<len;i++)
	{
	  if (p->shuffle_list[i] == p->idx)
            {
	      p->shuffle_list[i] = p->shuffle_list[0];
	      p->shuffle_list[0] = p->idx;
	      break;
	    }
	}
      player_set_index (p, p->idx);
    } 
  else 
    {
      /* Find the right un-shuffled spot */
      player_set_index (p, p->shuffle_list[p->idx]);
    }
}
