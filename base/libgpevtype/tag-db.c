/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <glib.h>
#include <gpe/tag-db.h>

void
gpe_tag_list_free (GSList *tags)
{
  GSList *i;

  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *p = i->data;
      
      g_free ((void *)p->tag);
      g_free ((void *)p->value);
      g_free (p);
    }

  g_slist_free (tags);
}

GSList *
gpe_tag_list_prepend (GSList *data, const char *tag, const char *value)
{
  gpe_tag_pair *p = g_malloc (sizeof (gpe_tag_pair));

  p->tag = g_strdup (tag);
  p->value = value;

  return g_slist_prepend (data, p);
}

