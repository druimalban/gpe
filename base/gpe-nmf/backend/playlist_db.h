/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef PLAYLIST_DB_H
#define PLAYLIST_DB_H

#include <glib.h>

typedef enum
  {
    ITEM_TYPE_LIST,
    ITEM_TYPE_TRACK
  } item_type;

struct playlist
{
  gchar *title;
  item_type type;
  struct playlist *parent;
  union
  {
    GSList *list;
    struct 
    {
      gchar *url;
      gchar *artist;
      gchar *album;

      guint volume;
      guint treble;
      guint bass;
    } track;
  } data;
};

extern struct playlist *playlist_fetch_item (struct playlist *, guint);
extern        int       playlist_get_length (struct playlist *);

extern struct playlist *playlist_new_list (void);
extern struct playlist *playlist_new_track (void);

extern             void playlist_free (struct playlist *);

extern struct playlist *playlist_xml_load (gchar *);
extern struct playlist *playlist_m3u_load (gchar *);

#endif
