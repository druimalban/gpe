/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef STREAM_H
#define STREAM_H

#include <glib.h>
#include <inttypes.h>

typedef struct stream *stream_t;

extern stream_t stream_open (gchar *url);
extern int stream_read (stream_t, void *, size_t);
extern void stream_close (stream_t);
extern int stream_seek (stream_t, int64_t offset, int whence);
extern long stream_tell (stream_t);

#endif
