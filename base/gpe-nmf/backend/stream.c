/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "stream.h"
#include "stream_i.h"

struct generic_stream
{
  struct stream_engine *e;
};

stream_t 
stream_open (gchar *url)
{
  /* hack */
  extern stream_t file_stream_open (gchar *name);
  return file_stream_open (url);
}

int 
stream_read (stream_t s, void *buf, size_t len)
{
  struct generic_stream *g = (struct generic_stream *)s;
  return g->e->read (s, buf, len);
}

void 
stream_close (stream_t s)
{
  struct generic_stream *g = (struct generic_stream *)s;
  return g->e->close (s);
}
