/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "player.h"
#include "decoder.h"

struct player
{
  struct playlist_item *list;
  guint idx;

  struct decoder *decoder;
  decoder_context decoder_context;
  struct stream *stream;
};

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
  if (p->decoder_context)
    p->decoder->close (p->decoder_context);

  if (p->stream)
    p->stream->close (p->stream);
 
  g_free (p);
}
