/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>

#include "decoder.h"
#include "decoder_i.h"
#include "playlist_db.h"

GSList *decoders;

struct generic_decoder
{
  struct decoder_engine *e;
};

static struct decoder_engine *
find_decoder_for (char *fn)
{
  GSList *iter;
  size_t l = strlen (fn);
  for (iter = decoders; iter; iter = iter->next)
    {
      struct decoder_engine *d = iter->data;
      size_t el = strlen (d->extension);
      if (el > l)
	continue;
      if (strcmp (fn + l - el, d->extension))
	continue;
      return d;
    }
  return NULL;
}

static gboolean
load_decoder (char *fn)
{
  void *dh = dlopen (fn, RTLD_LAZY);
  void *ds;

  if (dh == NULL)
    {
      fprintf (stderr, "%s: %s\n", fn, dlerror ());
      return FALSE;
    }

  ds = dlsym (dh, "the_decoder");

  if (ds == NULL)
    {
      fprintf (stderr, "%s: %s\n", fn, dlerror ());
      dlclose (dh);
      return FALSE;
    }

  decoders = g_slist_append (decoders, ds);

  return TRUE;
}

void
decoder_init (void)
{
  load_decoder ("./backend/d-mad.so");
  load_decoder ("./backend/d-vorbis.so");
}

decoder_t
decoder_open (char *name, stream_t in, audio_t out)
{
  struct decoder_engine *decoder = find_decoder_for (name);
  if (decoder == NULL)
    return NULL;

  return decoder->open (in, out);
}

void
decoder_close (decoder_t d)
{
  struct generic_decoder *g = (struct generic_decoder *)d;
  g->e->close (d);
}

void
decoder_stats (decoder_t d, struct decoder_stats *ds)
{
  struct generic_decoder *g = (struct generic_decoder *)d;
  g->e->stats (d, ds);
}

gboolean
decoder_fill_in_playlist (struct playlist *p)
{
  struct decoder_engine *e;

  if (p->type != ITEM_TYPE_TRACK)
    return FALSE;

  e = find_decoder_for (p->data.track.url);
  if (e)
    return e->fill_in_playlist (p);

  return FALSE;
}
