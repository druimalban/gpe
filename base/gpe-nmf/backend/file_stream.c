/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "stream.h"
#include "stream_i.h"

struct file_stream
{
  struct stream_engine *engine;

  int fd;
};

struct stream_engine file_stream_struct;

stream_t
file_stream_open (gchar *name)
{
  struct file_stream *f = g_malloc (sizeof (struct file_stream));

  f->engine = &file_stream_struct;
  f->fd = open (name, O_RDONLY);

  if (f->fd < 0)
    {
      g_free (f);
      return NULL;
    }

  return (stream_t)f;
}

int
file_stream_read (struct stream *s, void *buf, int size)
{
  struct file_stream *f = (struct file_stream *)s;
  return read (f->fd, buf, size);
}

void
file_stream_close (struct stream *s)
{
  struct file_stream *f = (struct file_stream *)s;
  close (f->fd);
  g_free (s);
}

void
file_stream_seek (struct stream *s, int64_t offset, int whence)
{
  struct file_stream *f = (struct file_stream *)s;
  return lseek (f->fd, offset, whence);
}

long
file_stream_tell (struct stream *s)
{
  struct file_stream *f = (struct file_stream *)s;
  return lseek (f->fd, 0, SEEK_CUR);  
}

struct stream_engine file_stream_struct = 
  {
    "file",

    file_stream_open,
    file_stream_read,
    file_stream_close,
    file_stream_seek,
    file_stream_tell
  };

