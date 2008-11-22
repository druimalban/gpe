/* player.h - Player interface.
   Copyright 2007 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro
        <skyhusker@handhelds.org>

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

#ifndef PLAYER_H
#define PLAYER_H

#include <glib-object.h>

#include <gst/gstelement.h>
#include <gst/gstformat.h>

typedef struct _Player Player;
typedef struct _PlayerClass PlayerClass;

#define PLAYER_TYPE              (player_get_type ())
#define PLAYER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAYER_TYPE, Player))
#define PLAYER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PLAYER_TYPE, PlayerClass))
#define IS_PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAYER_TYPE))
#define IS_PLAYER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAYER_TYPE))
#define PLAYER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAYER_TYPE, PlayerClass))

struct _PlayerClass
{
  GObjectClass parent;

  /* "state-changed" signal: the currently playing stream changed
     state, e.g., from playing to paused.  Passes the current cookie
     (gpointer) and the new state (int).  */
  guint state_changed_signal_id;
  /* "eos" signal: the current song has ended.  Passes the cookie
     (gpointer) associated with it.  */
  guint eos_signal_id;
  /* "tags" signal: the steam emitted some tags.  Passes the cookie
     (gpointer) and a GstTagList *.  */
  guint tags_signal_id;
};

extern GType player_get_type (void);

/** player_open:

  Create a new player.  */
extern Player *player_new (void);

/* Set the player's sink.  */
extern gboolean player_set_sink (Player *self, const gchar *sink);


/** player_source_set:

    Set the player to use SOURCE.  */
extern void player_set_source (Player *pl, const char *source, gpointer cookie);

/* player_source_get:

   Return the currently playing item.  If none, returns NULL.  The
   caller must free the memory.  If COOKIE is not NULL, the cookie is
   returned in *COOKIE.  */
extern char *player_get_source (Player *pl, void **cookie);

/* player_get_volume:

   Return current volume level (between 0 and 10).  */
extern double player_get_volume (Player *pl);

/* player_set_volume:

   Set volume to a value between 0 and 10.  */
extern gboolean player_set_volume (Player *pl, double volume);

/** player_play:

  Start the playlist PL playing.  Returns TRUE if starting to play the
  current entry was successful.  */
extern gboolean player_play (Player *pl);

/** player_pause:

  Pause the playlist PL.  */
extern void player_pause (Player *pl);

/** player_unpause:

  Unpause the playlist PL.  Unlike player_play, this does not start
  from the beginning of the current entry.  */
extern void player_unpause (Player *pl);

/** player_play_pause_toggle:

  Pause the player if playing otherwise, starting playing.  */
extern void player_play_pause_toggle (Player *pl);

extern gboolean player_playing (Player *pl);

extern gboolean player_seek (Player *pl, GstFormat format, gint64 pos);

extern gboolean player_query_position (Player *pl, GstFormat *fmt,
				       gint64 *pos);

extern gboolean player_query_duration (Player *pl, GstFormat *fmt,
				       gint64 *pos, gint n);

#endif  /* PLAYER_H  */

