/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sqlite.h>

#include "todo.h"

static sqlite *sqliteh;

static char *dname = "/.gpe/todo";

static int
list_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      new_list (atoi (argv[0]), g_strdup (argv[1]));
    }
  return 0;
}

static int
item_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 5)
    {
      new_item (atoi (argv[0]), atoi (argv[1]),
		argv[2], atoi(argv[3]), argv[4]);
    }
  return 0;
}

int
sql_start (void)
{
  static const char *schema1_str = 
    "create table todo_lists (uid integer NOT NULL, description text)";
  static const char *schema2_str = 
    "create table todo_items (uid integer NOT NULL, list integer NOT NULL, "
    "summary text, description text, state integer NOT NULL, " 
    "due_by text, completed_at text)";

  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL) 
    home = "";
  len = strlen (home) + strlen (dname);
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, dname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return -1;
    }

  sqlite_exec (sqliteh, schema1_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema2_str, NULL, NULL, &err);

  if (sqlite_exec (sqliteh, "select uid,description from todo_lists",
		   list_callback, NULL, &err))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return -1;
    }

  if (sqlite_exec (sqliteh, 
		   "select uid,list,summary,state,due_by from todo_items",
		   item_callback, NULL, &err))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return -1;
    }

  return 0;
}

void
sql_close (void)
{
  sqlite_close (sqliteh);
}

void
sql_add_list (int id, const char *title)
{
  char buf[256];
  snprintf (buf, sizeof(buf), 
	    "insert into todo_lists values(%d,'%s')", 
	    id, title);
  sqlite_exec (sqliteh, buf, NULL, NULL, NULL);
}
