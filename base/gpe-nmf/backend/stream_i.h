/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef STREAM_I_H
#define STREAM_I_H

#include <glib.h>
#include "stream.h"

struct stream_engine
{
  gchar *scheme;

  stream_t (*open)(gchar *name);
  int (*read) (stream_t, void *buf, int size);
  void (*close) (stream_t);
  int (*seek) (stream_t, int64_t, int);
  long (*tell) (stream_t);
};

#endif
