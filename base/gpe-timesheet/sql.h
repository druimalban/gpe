/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef SQL_H
#define SQL_H

#include <glib.h>

struct task
{
  guint id;
  gchar *description;
  guint time_cf;
  GSList *children;
  struct task *parent;
};

extern gboolean sql_start (void);
extern GSList *tasks;

#endif
