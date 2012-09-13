/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2006 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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

#include <sqlite3.h>
/* define own sqlite3 equivalent of sqlite3_exec_printf */
#define sqlite3_exec_printf(handle_, query_, cb_, cookie_, err_, args_...) \
  ({ char *q_ = sqlite3_mprintf (query_ , ## args_); \
     int ret_ = sqlite3_exec (handle_, q_, cb_, cookie_, err_); \
     sqlite3_free (q_); \
     ret_; })

struct map
{
  int old, new;
};

static GSList *mapping;

static void
migrate_one_category (sqlite3 *db, int id, gchar *string)
{
  struct map *map;
  int new_id;
  char *err;

  if (sqlite3_exec_printf (db, "update contacts set value='MIGRATED-%d' where tag='CATEGORY' and value='%d'",
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
contacts_db_migrate_old_categories (sqlite3 *db)
{
  gint r, c;
  gchar **list;

  if (sqlite3_get_table (db, "select id,description from contacts_category", &list, &r, &c, NULL) == SQLITE_OK)
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

	  sqlite3_exec_printf (db, "update contacts set value='%d' where tag='CATEGORY' and value='MIGRATED-%d'",
			      NULL, NULL, NULL, map->new, map->old);

	  g_free (map);
	}

      sqlite3_exec_printf (db, "drop table contacts_category", NULL, NULL, NULL);

      sqlite3_free_table (list);
    }
}

