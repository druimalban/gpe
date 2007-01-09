/*
 * Copyright (C) 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
 * Changes (C) 2004 Chris Lord <cwiiis@handhelds.org> :
 * - Enable progress slider
 * - Fix time display
 * - Add seeking
 * Several updates and fixes 
 *      (c) 2005 Florian Boor <florian@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gpe/errorbox.h>

#include "player.h"
#include <gst/gst.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <assert.h>

#define _(x) gettext(x)

static gboolean play_track (player_t p, struct playlist *t);

player_t
player_new (void)
{
  player_t p = g_malloc0 (sizeof (struct player));
  return p;
}

void
player_destroy (player_t p)
{
  player_stop (p);
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

  srandom (time (NULL));

  arr = malloc (sizeof (int) * len);

  for (i = 0; i < len; i++)
    arr[i] = i;

  for (i = 0; i < len - 1; i++)
    {
      int num, tmp;
      num = (random () % (len - i)) + i;
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
  if (idx < 0)
    {
      fprintf (stderr, "Illegal index (%d)\n", idx);
      return;
    }
  if (p->opt_shuffle)
    {
      int i, len;
      p->idx = 0;		/* just in case it's not in the list */
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
  if (p->state != PLAYER_STATE_NEXT_TRACK)
    p->state = PLAYER_STATE_STOPPING;
  if (p->audio)
    {
      gst_element_set_state (p->audio, GST_STATE_NULL);
    }
}

void
player_stop (player_t p)
{
  if (p->audio)
    abandon_track (p);

  p->cur = NULL;
}

static void
step_track (player_t p, int n)
{
  assert (p);
  assert (n != 0);

  if (!playlist_get_length (p->list))
    return;

  if (p->state == PLAYER_STATE_PLAYING || p->state == PLAYER_STATE_PAUSED)
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
	p->idx = playlist_get_length (p->list) - 1;
    }

  if (p->opt_loop && playlist_get_length (p->list) == p->idx)
    p->idx = 0;

  if (p->opt_shuffle)
    p->cur = playlist_fetch_item (p->list, p->shuffle_list[p->idx]);
  else
    p->cur = playlist_fetch_item (p->list, p->idx);

  if (p->cur)
    play_track (p, p->cur);
}

static void
error_callback (GstElement * thread, GstElement * source, GError * error,
		gchar * debug, player_t p)
{
  g_print ("ERROR: %s: %s\n", GST_OBJECT_NAME (source), error->message);

  p->state = PLAYER_STATE_NEXT_TRACK;
  player_stop (p);
}

static void
thread_shutdown (GstElement * elt, player_t p)
{
  gst_object_unref (GST_OBJECT (p->audio));
  p->audio = NULL;
  if (p->state == PLAYER_STATE_NEXT_TRACK)
    {
      p->state = PLAYER_STATE_NULL;
      player_next_track (p);
    }
  else
    p->state = PLAYER_STATE_NULL;
}

static void
eos (GstElement * elt, player_t p)
{
  p->state = PLAYER_STATE_NEXT_TRACK;
  if (p->audio)
    {
      gst_element_set_state (p->audio, GST_STATE_NULL);
    }
}

static void
metadata_notify (GObject * obj, GObject * the_obj, GParamSpec * spec,
		 player_t p)
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
      gst_element_set_state (p->audio, GST_STATE_NULL);
    }
}

static gboolean
build_pipeline (player_t p, struct playlist *t, gboolean really_play)
{
  p->audio = gst_element_factory_make ("playbin", "playbin");

  if (!p->audio)
    {
      gpe_perror_box ("Could not create playbin");
      return FALSE;
    }

  g_object_set (G_OBJECT (p->audio), "uri", t->data.track.url, NULL);

  return TRUE;
}

static gboolean
play_track (player_t p, struct playlist *t)
{
  if (!t)
    {
      fprintf (stderr, "Nothing to play.\n");
      return FALSE;
    }
  fprintf (stderr, "player %p: play item %p [%s]\n", p, t, t->data.track.url);

  p->new_track = TRUE;

  if (!build_pipeline (p, t, TRUE))
    {
      if (p->audio)
	{
	  gst_element_set_state (p->audio, GST_STATE_NULL);
	  gst_object_unref (GST_OBJECT (p->audio));
	}
      p->audio = NULL;
      p->state = PLAYER_STATE_NULL;
      return FALSE;
    }

  g_signal_connect (p->audio, "deep_notify::metadata", G_CALLBACK (metadata_notify), p);

  p->state = PLAYER_STATE_PLAYING;
  return gst_element_set_state (p->audio, GST_STATE_PLAYING);
}

gboolean
player_play (player_t p)
{
  if (p->state != PLAYER_STATE_PAUSED)
    {
      if (p->list)
	{
	  if (p->opt_shuffle)
	    p->cur = playlist_fetch_item (p->list, p->shuffle_list[p->idx]);
	  else
	    p->cur = playlist_fetch_item (p->list, p->idx);

	  if (p->cur)
	    return play_track (p, p->cur);
	}
      else
	fprintf (stderr, "Nothing in list\n");
    }
  else
    {
      if (gst_element_set_state (p->audio, GST_STATE_PLAYING))
	{
	  p->state = PLAYER_STATE_PLAYING;
	  return TRUE;
	}
    }

  return FALSE;
}

gboolean
player_pause (player_t p)
{
  if (p->state == PLAYER_STATE_PLAYING)
    {
      if (gst_element_set_state (p->audio, GST_STATE_PAUSED))
	{
	  p->state = PLAYER_STATE_PAUSED;
	  return TRUE;
	}
      else
	return FALSE;
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
  s->state = p->state;

  if (p->state == PLAYER_STATE_PLAYING)
    {
      GstFormat format = GST_FORMAT_TIME;
      gint64 time;
      gst_element_query_position (p->audio, &format, &time);
      s->time = (double) time / (double) GST_SECOND;
      gst_element_query_duration (p->audio, &format, &time);
      s->total_time = (double) time / (double) GST_SECOND;
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

  if (!G_IS_OBJECT (p->audio))
    return;
  memset (&value, 0, sizeof (value));
  g_value_init (&value, G_TYPE_FLOAT);
  g_value_set_float (&value, (float) v / 256);

  g_object_set (G_OBJECT (p->audio), "volume", (gfloat) v / 256, NULL);
}

float
player_get_volume (player_t p)
{
  float vol;

  if (!G_IS_OBJECT (p->audio))
    return 0;
  g_object_get (G_OBJECT (p->audio), "volume", &vol, NULL);

  return (float) (vol * 256);
}

void
player_seek (player_t p, double progress)
{
  gint64 total_time = 0;
  GstFormat format = GST_FORMAT_TIME;

  gst_element_query_duration (p->audio, &format, &total_time);
  gst_element_seek (p->audio, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
		    GST_SEEK_TYPE_SET,
		    (guint64) ((double) total_time * progress),
		    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

#if 0
static void
fill_state_change (GstElement * elt, GstElementState old_state,
		   GstElementState new_state, player_t p)
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

  p->audio = gst_pipeline_new ("pipeline");

  build_pipeline (p, t, FALSE);

  g_signal_connect (p->audio, "state_change", G_CALLBACK (fill_state_change),
		    p);
  g_signal_connect (p->audio, "error", G_CALLBACK (error_callback), p);
  g_signal_connect (p->audio, "deep_notify::metadata",
		    G_CALLBACK (metadata_notify), p);

  gst_element_set_state (p->audio, GST_STATE_PLAYING);

  while (gst_bin_iterate (GST_BIN (p->audio)));

  gst_element_set_state (p->audio, GST_STATE_NULL);

  gst_object_unref (GST_OBJECT (p->audio));
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
      for (i = 0; i < len; i++)
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
