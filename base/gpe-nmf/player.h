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

extern player_t player_new (void);
extern void player_destroy (player_t);
extern void player_set_playlist (player_t, struct playlist *);

extern void player_play (player_t);
extern void player_stop (player_t);
extern void player_pause (player_t);
extern void player_next_track (player_t);
extern void player_prev_track (player_t);

#endif
