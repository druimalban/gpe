/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "playlist_db.h"
#include "player.h"

struct playlist *
playlist_m3u_load (gchar *s)
{
  FILE *fp = fopen (s, "r");
  struct playlist *p;
  char buf[256];
  if (!fp)
    return NULL;
  p = playlist_new_list ();
  while (fgets (buf, sizeof (buf), fp))
    {
      size_t s = strlen (buf);
      struct playlist *i = playlist_new_track ();
      while (s && isspace (buf[s-1]))
	buf[--s] = 0;
      i->data.track.url = g_strdup (buf);

      player_fill_in_playlist (i);

      if (i->title == NULL)
	i->title = i->data.track.url;
      i->parent = p;
      p->data.list = g_slist_append (p->data.list, i);
    }
  fclose (fp);
  return p;
}

