/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Changes (C) 2004 Chris Lord <cwiiis@handhelds.org> :
 * - Enable progress slider
 * - Fix time display
 * - Add seeking
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <gst/gst.h>

#include "playlist_db.h"

typedef enum
  {
    PLAYER_STATE_NULL,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
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
  GstElement *conv, *audio;
  GstPad *audiopad;
  
  int opt_shuffle;
  int opt_loop;

  char *source_elem;
  char *sink_elem;
};

typedef struct player *player_t;

struct player_status
{
  struct playlist *item;
  unsigned long sample_rate;
  double time, total_time;
  player_state state;
  gboolean changed;
};

extern player_t player_new (void);
extern void player_destroy (player_t);
extern void player_set_playlist (player_t, struct playlist *);
extern void player_set_index (player_t, gint i);

extern gboolean player_play (player_t);
extern void player_stop (player_t);
extern gboolean player_pause (player_t);
extern void player_next_track (player_t);
extern void player_prev_track (player_t);

extern void player_status (player_t, struct player_status *);

extern void player_set_volume (player_t, int);
extern void player_seek (player_t, double);
extern void player_set_loop (player_t, int);
extern void player_set_shuffle (player_t, int);

extern struct playlist *player_get_playlist (player_t p);

extern void player_error_handler (player_t, void (*func)(gchar *));

extern struct stream *player_next_stream (player_t p);

extern void player_fill_in_playlist (struct playlist *);

#endif
