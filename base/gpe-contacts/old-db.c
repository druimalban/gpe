/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#include <gpe/pim-categories.h>
#include <gpe/errorbox.h>

#include <sqlite.h>

struct map
{
  int old, new;
};

GSList *mapping;

void
migrate_one_category (sqlite *db, int id, gchar *string)
{
  struct map *map;
  int new_id;
  char *err;

  if (sqlite_exec_printf (db, "update contacts set value='MIGRATED-%d' where tag='CATEGORY' and value='%d'",
			  NULL, NULL, &err, id, id) != SQLITE_OK)
    {
      gpe_error_box (err);
      free (err);
    }

  if (gpe_pim_category_new (string, &new_id))
    {
      fprintf (stderr, "old %d, new %d\n", id, new_id);

      map = g_malloc0 (sizeof (*map));
      map->old = id;
      map->new = new_id;
      mapping = g_slist_prepend (mapping, map);
    }
}

void
migrate_old_categories (sqlite *db)
{
  gint r, c;
  gchar **list;

  if (sqlite_get_table (db, "select id,description from contacts_category", &list, &r, &c, NULL) == SQLITE_OK)
    {
      int i;
      GSList *iter;

      for (i = 1; i < r; i++)
	{
	  gchar **data = &list[i * c];
	  int id = atoi (data[0]);

	  if (id)
	    migrate_one_category (db, id, data[1]);
	}

      for (iter = mapping; iter; iter = iter->next)
	{
	  struct map *map = iter->data;

	  sqlite_exec_printf (db, "update contacts set value='%d' where tag='CATEGORY' and value='MIGRATED-%d'",
			      NULL, NULL, NULL, map->new, map->old);

	  g_free (map);
	}

      sqlite_exec_printf (db, "drop table contacts_category", NULL, NULL, NULL);

      sqlite_free_table (list);
    }
}
