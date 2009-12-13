/* player.c - Player support.
   Copyright (C) 2007, 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro <skyhusker@handhelds.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#define ERROR_DOMAIN() g_quark_from_static_string ("player")

#include <string.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/audio/gstaudiosink.h>

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>

#include "player.h"
#include "marshal.h"
#include "config.h"
#include "utils.h"

struct _Player {
  GObject parent;

  GstElement *playbin;
  GstState last_state;
  double volume;
  char *sink;

  void *cookie;

  guint bus_watch;
};

static void player_dispose (GObject *obj);
static void player_finalize (GObject *object);

G_DEFINE_TYPE (Player, player, G_TYPE_OBJECT);

static void
player_class_init (PlayerClass *klass)
{
  player_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = player_finalize;
  object_class->dispose = player_dispose;

  PlayerClass *player_class = PLAYER_CLASS (klass);

  player_class->state_changed_signal_id
    = g_signal_new ("state-changed",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_user_marshal_VOID__POINTER_INT,
		    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
    
  player_class->eos_signal_id
    = g_signal_new ("eos",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE, 1, G_TYPE_POINTER);

  player_class->tags_signal_id
    = g_signal_new ("tags",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_user_marshal_VOID__POINTER_OBJECT,
		    G_TYPE_NONE, 2, G_TYPE_POINTER, GST_TYPE_TAG_LIST);
}

static void
player_init (Player *pl)
{
  static int gst_init_init;
  if (! gst_init_init)
    {
      gst_init (NULL, NULL);
      gst_init_init = 1;
    }

  pl->volume = -1;
}

static void
player_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (player_parent_class)->dispose (obj);
}

static void
player_finalize (GObject *object)
{
  Player *pl = PLAYER (object);

  G_OBJECT_CLASS (player_parent_class)->finalize (object);

  if (pl->playbin)
    {
      gst_element_set_state (pl->playbin, GST_STATE_NULL);
      gst_object_unref (pl->playbin);
    }
}

static gboolean
player_bus_cb (GstBus *bus, GstMessage *message, gpointer data)
{
    Player *pl = PLAYER (data);

    switch (GST_MESSAGE_TYPE (message))
      {
      case GST_MESSAGE_ERROR:
	{
	  GError *err = NULL;
	  gchar *debug;
	  gst_message_parse_error (message, &err, &debug);

	  g_debug ("%s: %s (%s)\n", __func__, err->message, debug);

	  g_error_free (err);
	  g_free (debug);

	  g_signal_emit (pl, PLAYER_GET_CLASS (pl)->eos_signal_id, 0,
			 pl->cookie);
	  break;
	}

      case GST_MESSAGE_STATE_CHANGED:;
	GstState state, pending;
	gst_message_parse_state_changed (message, NULL, &state, &pending);

#if 0
	char *s = "unknown";
	switch (pending)
	  {
	  case GST_STATE_VOID_PENDING:
	    s = "void_pending";
	    break;
	  case GST_STATE_NULL:
	    s = "null";
	    break;
	  case GST_STATE_READY:
	    s = "ready";
	    break;
	  case GST_STATE_PAUSED:
	    s = "paused";
	    break;
	  case GST_STATE_PLAYING:
	    s = "playing";
	    break;
	  }
	char *p = s;
	switch (state)
	  {
	  case GST_STATE_VOID_PENDING:
	    s = "void_pending";
	    break;
	  case GST_STATE_NULL:
	    s = "null";
	    break;
	  case GST_STATE_READY:
	    s = "ready";
	    break;
	  case GST_STATE_PAUSED:
	    s = "paused";
	    break;
	  case GST_STATE_PLAYING:
	    s = "playing";
	    break;
	  }
#endif

	if (GST_MESSAGE_SRC (message) == GST_OBJECT (pl->playbin))
	  {
	    // g_debug ("playbin's state changed: %s, %s pending", s, p);

	    if (pending == GST_STATE_VOID_PENDING)
	      {
		pl->last_state = state;

		g_signal_emit
		  (pl, PLAYER_GET_CLASS (pl)->state_changed_signal_id,
		   0, pl->cookie, state);
	      }
	  }
	break;

      case GST_MESSAGE_EOS:
	g_signal_emit (pl, PLAYER_GET_CLASS (pl)->eos_signal_id, 0,
		       pl->cookie);
	break;

      case GST_MESSAGE_TAG:
	{
	  GstTagList *tags;
	  gst_message_parse_tag (message, &tags);

	  g_signal_emit (pl, PLAYER_GET_CLASS (pl)->tags_signal_id, 0,
			 pl->cookie, tags);

	  gst_tag_list_free (tags);
	  break;
	}

      default:
	break;
      }

    return TRUE;
}

Player *
player_new (void)
{
  Player *player = PLAYER (g_object_new (PLAYER_TYPE, NULL));
  return player;
}

static void
playbin_ensure (Player *pl)
{
  if (! pl->playbin)
    {
      char *element;
#if defined(MAEMO5)
      element = "playbin2";
#elif defined(IS_HILDON)
# if HILDON_VER == 0
      /* Evil, evil, evil, evil.  playbin is broken on Bora.  */
      element = "playbinmaemo";
# else
      element = "playbin";
# endif
#else
      element = "playbin";
#endif
      pl->playbin = gst_element_factory_make (element, "player");
      if (! pl->playbin)
	{
	  starling_error_box_fmt (_("Failed to initialize gstreamer: "
				    "could not create a %s element."),
				  element);
	  exit (1);
	}

      GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pl->playbin));
      pl->bus_watch = gst_bus_add_watch (bus, player_bus_cb, pl);
      gst_object_unref (bus);

      if (pl->volume >= 0)
	g_object_set (G_OBJECT (pl->playbin), "volume", pl->volume, NULL);
    }
}

static void
playbin_recreate (Player *pl)
{
  if (pl->playbin)
    {
      gst_element_set_state (pl->playbin, GST_STATE_NULL);

      g_source_remove (pl->bus_watch);
      g_object_unref (pl->playbin);
      pl->playbin = 0;
    }

  playbin_ensure (pl);
}

gboolean
player_set_sink (Player *pl, const gchar *sink)
{
  if (pl->sink)
    g_free (pl->sink);
  pl->sink = sink ? g_strdup (sink) : NULL;

  GstElement *audiosink = gst_element_factory_make (sink, "sink");
  if (! audiosink)
    return FALSE;

  playbin_ensure (pl);

  g_object_set (G_OBJECT (pl->playbin), "audio-sink", audiosink, NULL);

  return TRUE;
}

void
player_set_source (Player *pl, const char *source, gpointer cookie)
{
  pl->cookie = cookie;

  playbin_recreate (pl);

  if (*source == '/')
    /* It's a filename.  Create a URI.  */
    {
      struct obstack uri;
      obstack_init (&uri);

      obstack_printf (&uri, "file://");

      char *s = uri_escape_string (source);
      obstack_grow (&uri, s, strlen (s) + 1);
      g_free (s);

      char *location = obstack_finish (&uri);
      g_object_set (G_OBJECT (pl->playbin), "uri",
		    location, NULL);
      obstack_free (&uri, NULL);
    }
  else
    g_object_set (G_OBJECT (pl->playbin), "uri", source, NULL);

  gst_element_set_state (pl->playbin, GST_STATE_PAUSED);
}

char *
player_get_source (Player *pl, void **cookie)
{
  if (! pl->playbin)
    return NULL;

  if (cookie)
    *cookie = pl->cookie;

  char *source = NULL;
  g_object_get (G_OBJECT (pl->playbin), "uri", &source, NULL);
  return source;
}

double
player_get_volume (Player *pl)
{
  return pl->volume;
}

gboolean
player_set_volume (Player *pl, double volume)
{
  pl->volume = volume;

  if (pl->playbin)
    g_object_set (G_OBJECT (pl->playbin), "volume", volume, NULL);
}

gboolean
player_play (Player *pl)
{
  if (! pl->playbin)
    return TRUE;

  GstStateChangeReturn res = gst_element_set_state (pl->playbin,
						    GST_STATE_PLAYING);
  if (res == GST_STATE_CHANGE_FAILURE)
    {
      char *source = player_get_source (pl, NULL);
      g_warning ("%s: Failed to play %s", __FUNCTION__, source);
      g_free (source);

      return FALSE;
    }

  return TRUE;
}

void
player_pause (Player *pl)
{
  if (! pl->playbin)
    return;

  GstStateChangeReturn res
    = gst_element_set_state (pl->playbin, GST_STATE_PAUSED);
  if (res == GST_STATE_CHANGE_FAILURE)
    g_debug ("%s failed to change state to paused", __FUNCTION__);
}

void
player_unpause (Player *pl)
{
  if (! pl->playbin)
    /* Nothing is loaded.  */
    {
      g_signal_emit (pl, PLAYER_GET_CLASS (pl)->eos_signal_id, 0,
		     pl->cookie);
      return;
    }

  GstStateChangeReturn res
    = gst_element_set_state (pl->playbin, GST_STATE_PLAYING);
  if (res == GST_STATE_CHANGE_FAILURE)
    g_debug ("%s failed to change state to paused for playing", __FUNCTION__);
}

void
player_play_pause_toggle (Player *pl)
{
  if (! pl->playbin)
    return;

  GstState state;
  if (player_playing (pl))
    state = GST_STATE_PLAYING;
  else
    state = GST_STATE_PAUSED;

  GstStateChangeReturn res = gst_element_set_state (pl->playbin, state);
  if (res == GST_STATE_CHANGE_FAILURE)
    g_debug ("%s failed to change state to %s", __FUNCTION__,
	     state == GST_STATE_PLAYING ? "playing" : "paused");
}

gboolean
player_playing (Player *pl)
{
  if (! pl->playbin)
    return FALSE;

  GstState state = GST_STATE_PAUSED;
  if (pl->playbin)
    gst_element_get_state (pl->playbin, &state, NULL, GST_CLOCK_TIME_NONE);

  return state == GST_STATE_PLAYING;
}

gboolean
player_seek (Player *pl, GstFormat format, gint64 pos)
{
  if (! pl->playbin)
    return TRUE;

  return gst_element_seek (GST_ELEMENT (pl->playbin), 1.0, format,
			   GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
			   pos, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

gboolean
player_query_position (Player *pl, GstFormat *fmt, gint64 *pos)
{
  if (! pl->playbin)
    {
      *pos = 0;
      return TRUE;
    }
  return gst_element_query_position (pl->playbin, fmt, pos); 
}

gboolean
player_query_duration (Player *pl, GstFormat *fmt, gint64 *pos, gint n)
{
  if (! pl->playbin)
    return FALSE;
  return gst_element_query_duration (pl->playbin, fmt, pos);
}
