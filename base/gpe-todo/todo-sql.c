/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#include "errorbox.h"

#include "todo.h"
#include "todo-sql.h"

static sqlite *sqliteh;

static const char *dname = "/.gpe";
static const char *fname = "/todo";

static int next_uid;

GSList *lists;

struct todo_list *all_items;

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
      time_t t = (time_t)0;

      for (iter = lists; iter; iter = iter->next)
	{
	  struct todo_list *l = iter->data;
	  if (l->id == list)
	    break;
	}

      /* ignore items belonging to deleted / bogus lists */
      if (iter == NULL)
	return 0;

      if (next_uid < id)
	next_uid = id;

      if (due[0])
	{
	  struct tm tm;
	  memset (&tm, 0, sizeof (tm));
	  strptime (due, "%F", &tm);
	  t = mktime (&tm);
	}

      add_new_item_internal ((struct todo_list *)iter->data, 
			     t, g_strdup (description), state, 
			     g_strdup (summary), id);
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
new_unique_id (void)
{
  return ++next_uid;
}

int
new_list_id (void)
{
  int id = 0;
  int found;
  do 
    {
      GSList *t;
      id ++;
      found = 0;
      for (t = lists; t; t = t->next)
	{
	  if (((struct todo_list *)t->data)->id == id)
	    {
	      found = 1;
	      break;
	    }
	}
    } while (found);

  return id;
}

struct todo_list *
new_list (int id, const char *title)
{
  struct todo_list *t = g_malloc (sizeof (struct todo_list));
  t->items = NULL;
  t->title = title;
  t->id = id;

  lists = g_slist_append (lists, t);
  return t;
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
  static const char *schema3_str = 
    "create table next_uid (uid integer NOT NULL)";

  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL) 
    home = "";
  len = strlen (home) + strlen (dname) + strlen (fname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, dname);
  if (access (buf, F_OK))
    {
      if (mkdir (buf, 0700))
	{
	  gpe_perror_box (buf);
	  g_free (buf);
	  return -1;
	}
    }
  strcat (buf, fname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  sqlite_exec (sqliteh, schema1_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema2_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema3_str, NULL, NULL, &err);

  if (sqlite_exec (sqliteh, "select uid,description from todo_lists",
		   list_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      return -1;
    }

  if (sqlite_exec (sqliteh, 
		   "select uid,list,summary,description,state,due_by from todo_items",
		   item_callback, NULL, &err))
    {
      gpe_error_box (err);
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

gint
list_sort_func (gconstpointer a, gconstpointer b)
{
  const struct todo_item *ia = a, *ib = b;

  if (ia->time == ib->time)
    return 0;

  if (ia->time == 0)
    return 1;
  if (ib->time == 0)
    return -1;

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

  list->items = g_list_insert_sorted (list->items, i, list_sort_func);

  return i;
}

void
add_new_item (struct todo_list *list, time_t t, const char *what, 
	      item_state state, const char *summary, int id)
{
  struct todo_item *i;
  i = add_new_item_internal (list, t, what, state, summary, id);
  add_new_item_sql (i, list->id);
}

void
push_item (struct todo_item *i)
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
			  "update todo_items set summary='%q',description='%q',state=%d,due_by='%q' where uid=%d",
			  NULL, NULL, &err,
			  i->summary, i->what, i->state, due, i->id))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
    }
}

void 
delete_item (struct todo_list *list, struct todo_item *i)
{
  sqlite_exec_printf (sqliteh, "delete from todo_items where uid=%d",
		      NULL, NULL, NULL,
		      i->id);

  list->items = g_list_remove (list->items, i);

  g_free ((gpointer)i->what);
  g_free ((gpointer)i->summary);
  g_free (i);
}

void
del_list (struct todo_list *list)
{
  char *err;
  if (sqlite_exec_printf (sqliteh, "delete from todo_items where list=%d",
		      NULL, NULL, &err,
			  list->id))
    {
      gpe_error_box (err);
      free (err);
    }

  if (sqlite_exec_printf (sqliteh, "delete from todo_lists where uid=%d",
		      NULL, NULL, &err,
			  list->id))
    {
      gpe_error_box (err);
      free (err);
    }

  lists = g_slist_remove (lists, list);
}
