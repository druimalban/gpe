/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef DECODER_H
#define DECODER_H

#include <glib.h>

#include "stream.h"
#include "audio.h"
#include "playlist_db.h"

typedef struct decoder *decoder_t;

struct decoder_stats
{
  unsigned int rate;
  unsigned long long time;
  unsigned long long total_time;
  gboolean finished;
};

extern gboolean decoder_fill_in_playlist (struct playlist *);
extern decoder_t decoder_open (char *name, stream_t in, audio_t out);
extern void decoder_close (decoder_t);
extern gboolean decoder_seek (decoder_t, unsigned long long time);
extern void decoder_pause (decoder_t);
extern void decoder_unpause (decoder_t);
extern void decoder_close (decoder_t);
extern void decoder_stats (decoder_t, struct decoder_stats *s);

extern void decoder_init (void);

#endif
