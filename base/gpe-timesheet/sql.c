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
#include <libintl.h>
#include <glib.h>
#include <string.h>
#include <sqlite.h>

#include <gpe/errorbox.h>

#include "sql.h"

static const char *fname = "/.gpe/timesheet";

static const char *schema_str = 
"create table tasks (id integer, description text, cftime integer, parent integer);
create table log (task integer, time integer, start boolean);";

static sqlite *sqliteh;
static guint max_task_id;

GSList *tasks, *root;

struct task *find_by_id (guint id)
{
  GSList *iter;

  for (iter = tasks; iter; iter = iter->next)
    {
      struct task *t = iter->data;
      if (t->id == id)
	return t;
    }

  return NULL;
}

static void
internal_note_task (guint id, gchar *text, guint elapsed, struct task *pt)
{
  struct task *t;

  t = g_malloc (sizeof (struct task));
  t->id = id;
  t->description = g_strdup (text);
  t->time_cf = elapsed;
  t->children = NULL;
  t->parent = pt;
  if (pt)
    pt->children = g_slist_append (pt->children, t);
  else
    root = g_slist_append (root, t);
  tasks = g_slist_append (tasks, t);
  if (id > max_task_id)
    max_task_id = id;
}

static int
load_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 4)
    {
      struct task *pt = NULL;
      guint parent = atoi (argv[3]);
      if (parent)
	pt = find_by_id (parent);
      internal_note_task (atoi (argv[0]), argv[1], atoi (argv[2]), 
			  pt);
    }

  return 0;
}

gboolean
sql_start (void)
{
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
  g_free (buf);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema_str, NULL, NULL, &err);

  if (sqlite_exec (sqliteh, "select id, description, cftime, parent from tasks", 
		   load_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }  

  return TRUE;
}

void
new_task (gchar *description, struct task *parent)
{
  char *err;
  guint new_id = max_task_id + 1;

  if (sqlite_exec_printf (sqliteh,
			  "insert into tasks values (%d, '%q', %d, %d)",
			  NULL, NULL, &err,
			  new_id, description, 0, parent ? parent->id : 0))
    {
      gpe_error_box (err);
      free (err);
      return;
    }

  internal_note_task (new_id, description, 0, parent);
}

void
delete_task (struct task *t)
{
  char *err;

  if (t->children)
    {
      GSList *iter;
      for (iter = t->children; iter; iter = iter->next)
	delete_task (iter->data);
    }

  if (sqlite_exec_printf (sqliteh,
			  "delete from tasks where id=%d",
			  NULL, NULL, &err, t->id))
    {
      gpe_error_box (err);
      free (err);
      return;
    }

  if (sqlite_exec_printf (sqliteh,
			  "delete from log where task=%d",
			  NULL, NULL, &err, t->id))
    {
      gpe_error_box (err);
      free (err);
      return;
    }

  tasks = g_slist_remove (tasks, t);
  if (t->parent)
    t->parent->children = g_slist_remove (t->parent->children, t);
  else
    root = g_slist_remove (root, t);
  g_free (t->description);
  g_free (t);
}
