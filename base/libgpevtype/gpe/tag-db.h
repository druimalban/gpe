/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef TAG_DB_H
#define TAG_DB_H

typedef struct
{
  const char *tag;
  const char *value;
} gpe_tag_pair;

extern void gpe_tag_list_free (GSList *tags);
extern GSList *gpe_tag_list_prepend (GSList *data, const char *tag, const char *value);

#endif
