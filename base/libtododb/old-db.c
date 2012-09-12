/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE_
#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include <sqlite3.h>

#include <gpe/errorbox.h>
#include <gpe/todo-db.h>
#include <gpe/pim-categories.h>

/* define own sqlite3 equivalent of sqlite3_exec_printf */
#define sqlite3_exec_printf(handle_, query_, cb_, cookie_, err_, args_...) \
  ({ char *q_ = sqlite3_mprintf (query_ , ## args_); \
     int ret_ = sqlite3_exec (handle_, q_, cb_, cookie_, err_); \
     sqlite3_free (q_); \
     ret_; })

extern gboolean converted_item (struct todo_item *i);

static sqlite3 *sqliteh_here;

static int
item_callback0 (void *arg, int argc, char **argv, char **names)
{
  if (argc == 6)
    {
      //int list = atoi (argv[1]);
      char *summary = argv[2];
      char *description = argv[3];
      int state = atoi (argv[4]);
      char *due = argv[5];
      struct todo_item *i = g_malloc (sizeof (struct todo_item));
      
      time_t t = (time_t)0;

      memset (i, 0, sizeof (*i));
      
      i->id = sqlite3_last_insert_rowid (sqliteh_here);

      i->what = g_strdup (description);
      i->state = state;
      i->summary = g_strdup (summary);
  
      if (due[0])
	{
	  struct tm tm;
	  memset (&tm, 0, sizeof (tm));
	  strptime (due, "%F", &tm);
	  t = mktime (&tm);
	}
	
      i->time = t;
      
      converted_item (i);
    }
  
  return 0;
}

gboolean
convert_old_db (int oldversion, sqlite3 *sqliteh)
{
  char *err;
  
  sqliteh_here = sqliteh;
  
  if (oldversion == 0) 
    {
      sqlite3_exec (sqliteh, 
		       "select uid,list,summary,description,state,due_by from todo_items",
		   item_callback0, NULL, NULL);
    }
  
  oldversion = 1; /* set equal to new version */
  
  if (sqlite3_exec_printf (sqliteh, 
			  "insert into todo_dbinfo (version) values (%d)", 
			  NULL, NULL, &err, oldversion))
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
    
  return TRUE;
}

/* Category handling */

struct map
{
  int old, new;
};

GSList *mapping;

void
migrate_one_category (sqlite3 *db, int id, gchar *string)
{
  struct map *map;
  int new_id;
  char *err;

  if (sqlite3_exec_printf (db, "update todo set value='MIGRATED-%d' where tag='CATEGORY' and value='%d'",
			  NULL, NULL, &err, id, id) != SQLITE_OK)
    {
      gpe_error_box (err);
      free (err);
    }

  if (gpe_pim_category_new (string, &new_id))
    {
      map = g_malloc0 (sizeof (*map));
      map->old = id;
      map->new = new_id;
      mapping = g_slist_prepend (mapping, map);
    }
}

void
migrate_old_categories (sqlite3 *db)
{
  gint r, c;
  gchar **list;

  if (sqlite3_get_table (db, "select id,description from todo_categories", &list, &r, &c, NULL) == SQLITE_OK)
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

	  sqlite3_exec_printf (db, "update todo set value='%d' where tag='CATEGORY' and value='MIGRATED-%d'",
			      NULL, NULL, NULL, map->new, map->old);

	  g_free (map);
	}

      sqlite3_exec (db, "drop table todo_categories", NULL, NULL, NULL);

      sqlite3_free_table (list);
    }
}
