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
#include <string.h>
#include <sqlite.h>

#include "todo.h"
#include "todo-sql.h"

static sqlite *sqliteh;

static char *dname = "/.gpe/todo";

struct todo_item *
add_new_item_internal(struct todo_list *list, time_t t, const char *what, 
		      item_state state, const char *summary, int id);

static int
item_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 6)
    {
      int id = atoi (argv[0]);
      int list = atoi (argv[1]);
      char *summary = argv[2];
      char *description = argv[3];
      int state = atoi (argv[4]);
      char *due = argv[5];

      GSList *iter;
      struct todo_list *l;
      time_t t = (time_t)0;
      int uid = 0;

      for (iter = lists; iter; iter = iter->next)
	{
	  struct todo_list *l = iter->data;
	  if (l->id == list)
	    break;
	}

      /* ignore items belonging to deleted / bogus lists */
      if (iter == NULL)
	return 0;
      
      add_new_item_internal ((struct todo_list *)iter->data, 
			     t, g_strdup (description), state, 
			     g_strdup (summary), uid);
    }

  return 0;
}

static int
list_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      new_list (atoi (argv[0]), g_strdup (argv[1]));
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
		   "select uid,list,summary,description,state,due_by from todo_items",
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
  char *err;
  if (sqlite_exec_printf (sqliteh, "insert into todo_lists values(%d,'%q')", 
		      NULL, NULL, &err, id, title))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
    }
}

static void
add_new_item_sql (struct todo_item *i, int list_id)
{
  char d_buf[32];
  char *due, *err;

  if (i->time)
    {
      struct tm tm;
      localtime_r (&i->time, &tm);
      strftime (d_buf, sizeof(d_buf), "%F", &tm);
      due = d_buf;
    }
  else
    due = "";
      
  if (sqlite_exec_printf (sqliteh, 
			  "insert into todo_items values(%d,%d,'%q','%q',%d,'%q','%q')",
			  NULL, NULL, &err, i->id, list_id, i->summary, i->what,
			  i->state, due, ""))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
    }
}

static gint
insert_sort_func (gconstpointer a, gconstpointer b)
{
  const struct todo_item *ia = a, *ib = b;
  return ia->time - ib->time;
}

struct todo_item *
add_new_item_internal(struct todo_list *list, time_t t, const char *what, 
		      item_state state, const char *summary, int id)
{
  struct todo_item *i = g_malloc (sizeof (struct todo_item));

  i->id = id;
  i->what = what;
  i->time = t;
  i->state = state;
  i->summary = summary;

  list->items = g_list_insert_sorted (list->items, i, insert_sort_func);

  return i;
}

void
add_new_item (struct todo_list *list, time_t t, const char *what, 
	      item_state state, const char *summary, int id)
{
  struct todo_item *i = add_new_item_internal (list, t, what, state, summary, id);
  add_new_item_sql (i, list->id);
}
