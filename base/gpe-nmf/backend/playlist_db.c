/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <assert.h>

#include "playlist_db.h"

static struct playlist *
playlist_fetch_item_internal (struct playlist *l, int *idx)
{
  struct playlist *v = NULL;
  GSList *i;

  switch (l->type)
    {
    case ITEM_TYPE_TRACK:
      if (*idx == 0)
	v = l;
      else
	(*idx)--;
      break;

    case ITEM_TYPE_LIST:
      i = l->data.list;
      while (v == NULL && i != NULL)
	{
	  struct playlist *p = i->data;
	  v = playlist_fetch_item_internal (p, idx);
	  i = i->next;
	}
      break;
    }

  return v;
}

struct playlist *
playlist_fetch_item (struct playlist *l, guint idx)
{
  return playlist_fetch_item_internal (l, &idx);
}

struct playlist *
playlist_new_list (void)
{
  struct playlist *i = g_malloc (sizeof (struct playlist));
  memset (i, 0, sizeof (*i));
  i->type = ITEM_TYPE_LIST;
  return i;
}

struct playlist *
playlist_new_track (void)
{
  struct playlist *i = g_malloc (sizeof (struct playlist));
  memset (i, 0, sizeof (*i));
  i->type = ITEM_TYPE_TRACK;
  return i;
}
