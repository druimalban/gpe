/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "playlist_db.h"

typedef struct player *player_t;

struct player_status
{
  struct playlist *item;
  unsigned long sample_rate;
  unsigned long long time;
  unsigned long long total_time;
  gboolean changed;
};

extern player_t player_new (void);
extern void player_destroy (player_t);
extern void player_set_playlist (player_t, struct playlist *);
extern void player_set_index (player_t, gint i);

extern gboolean player_play (player_t);
extern void player_stop (player_t);
extern void player_pause (player_t);
extern void player_next_track (player_t);
extern void player_prev_track (player_t);

extern void player_status (player_t, struct player_status *);

extern void player_set_volume (player_t, int);
extern struct playlist *player_get_playlist (player_t p);

extern void player_error_handler (player_t, void (*func)(gchar *));

extern struct stream *player_next_stream (player_t p);

#endif
