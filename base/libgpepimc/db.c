/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite.h>

#include <gpe/errorbox.h>

#include "gpe/pim-categories.h"

#include "internal.h"

#define _(x) (x)

#define DB_NAME "/.gpe/categories"

static const char *schema_str = "create table contacts_category (id INTEGER PRIMARY KEY, description TEXT);";

static GSList *categories;
static sqlite *db;

static int
load_one (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      GSList **list = (GSList **) arg;
      struct gpe_pim_category *c = g_malloc (sizeof (*c));

      c->id = atoi (argv[0]);
      c->name = g_strdup (argv[1]);

      *list = g_slist_prepend (*list, c);
    }

  return 0;
}

gboolean 
gpe_pim_categories_init (void)
{
  char *err;
  char *buf;
  size_t len;
  char *home = getenv ("HOME");
  if (home == NULL)
    home = "";
  
  len = strlen (home) + strlen (DB_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, DB_NAME);
  
  db = sqlite_open (buf, 0, &err);
  g_free (buf);
	
  if (db == NULL) 
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (db, schema_str, NULL, NULL, NULL);

  if (sqlite_exec (db, "select id,description from category",
		   load_one, &categories, &err))
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  
  return TRUE;
}

GSList *
gpe_pim_categories_list (void)
{
  return categories;
}

gboolean
gpe_pim_category_new (const gchar *name, gint *id)
{
  char *err;
  int r =
    sqlite_exec_printf (db,
			"insert into contacts_category values (NULL, '%q')",
			      NULL, NULL, &err, name);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  
  *id = sqlite_last_insert_rowid (db);
  return TRUE;
}

void 
gpe_pim_category_delete (struct gpe_pim_category *c)
{
}
