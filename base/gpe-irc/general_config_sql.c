/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Damien Tanner <dctanner@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define _XOPEN_SOURCE

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sqlite.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>

#include <gpe/errorbox.h>

#include "general_config_sql.h"

static sqlite *sqliteh;

static const char *fname = "/.gpe/irc";

GHashTable *sql_general_config;

gint general_rows = 0;

static void
new_sql_general_tag_internal (int id, const char *property, const char *value)
{
  g_hash_table_insert (sql_general_config, (gpointer) property,
                       (gpointer) value);
}

void
new_sql_general_tag (const char *property, const char *value)
{
  char *err;

  if (sqlite_exec_printf
      (sqliteh, "insert into config values (NULL, '%q', '%q')", NULL, NULL,
       &err, property, value))
    {
      gpe_error_box (err);
      free (err);
    }

  g_hash_table_insert (sql_general_config, (gpointer) property,
                       (gpointer) value);
}

void
edit_sql_general_tag (const char *property, const char *value)
{
  char *err;

  if (sqlite_exec_printf
      (sqliteh, "update config set value='%q' where property='%q'", NULL,
       NULL, &err, value, property))
    {
      gpe_error_box (err);
      free (err);
    }

  g_hash_table_insert (sql_general_config, (gpointer) property,
                       (gpointer) value);
}

static int
sql_general_tag_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 3 && argv[0] && argv[1])
    {
      new_sql_general_tag_internal (atoi (argv[0]), g_strdup (argv[1]),
                                    g_strdup (argv[2]));
      general_rows++;
    }
  return 0;
}

static void
add_default_sql_tags (void)
{
  new_sql_general_tag ("tag_text", "black");
  new_sql_general_tag ("tag_bg", "white");
  new_sql_general_tag ("tag_nick", "blue");
  new_sql_general_tag ("tag_action", "red");
  new_sql_general_tag ("tag_own_nick", "black");
  new_sql_general_tag ("tag_channel", "italic");
  new_sql_general_tag ("tag_nick_ops", "bold");
  new_sql_general_tag ("tag_nick_highlight", "red");
}

int
general_sql_start (void)
{
  static const char *schema1_str =
    "create table config (uid INTEGER PRIMARY KEY, property TEXT, value TEXT)";

  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL)
    home = "";
  len = strlen (home) + strlen (fname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, fname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  sql_general_config = g_hash_table_new (g_str_hash, g_str_equal);

  sqlite_exec (sqliteh, schema1_str, NULL, NULL, &err);

  if (sqlite_exec (sqliteh, "select uid,property,value from config",
                   sql_general_tag_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  if (general_rows == 0)
    add_default_sql_tags ();

  g_free (buf);
  return 0;
}

void
general_sql_close (void)
{
  sqlite_close (sqliteh);
}
