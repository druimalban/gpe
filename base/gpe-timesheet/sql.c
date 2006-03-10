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
#include <libintl.h>

#include <gpe/errorbox.h>

#include "sql.h"

#define _(x) gettext(x)

static const char *fname = "/.gpe/timesheet";

static const char *schema_str = 
"create table tasks (id integer, description text, cftime integer, parent integer);"
"create table log (action text, task integer, time integer, info text);";

static sqlite *sqliteh;
static guint max_task_id;
static time_t time_start = 0;
static gchar *note_start = NULL;

GSList *children_list;

static const char *actions[] = { "START", "STOP" };

gboolean
log_entry (action_t action, time_t time, struct task *task, char *info)
{
  char *err;
  if (sqlite_exec_printf (sqliteh,
			  "insert into log values ('%s', %d, %d, '%q')",
			  NULL, NULL, &err,
			  actions[action], task->id, time, info))
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  free (err);
  return TRUE;
}

static struct task *
internal_note_task (guint id, gchar *text, guint elapsed, guint pt)
{
  struct task *t;

  t = g_malloc (sizeof (struct task));
  t->id = id;
  t->description = g_strdup (text);
  t->time_cf = elapsed;
  t->children = NULL;
  t->parent = pt;
  t->started = 0;

  if (id > max_task_id)
    max_task_id = id;

  return t;
}

static int
load_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 4)
  {
    guint parent = atoi (argv[3]);

    internal_note_task (atoi (argv[0]), argv[1], atoi (argv[2]), 
            parent);
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

  global_task_store = gtk_tree_store_new (NUM_COLS,
  G_TYPE_UINT,GDK_TYPE_PIXBUF, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN);

  if (sqlite_exec (sqliteh, "select id, description, cftime, parent from tasks where parent=0", load_to_treestore, NULL, &err))
  {
    gpe_error_box (err);
    free (err);
    return FALSE;
  }

  return TRUE;
}


/* this loads data from DB directly into GtkTreeViews */

int load_to_treestore(void *arg, int argc, char **argv, char **names)
{
  if (argc==4)
    {
      GtkTreeIter iter, *parent;
      char *query, *err;
      int test;

      parent=arg;
      query = g_malloc( sizeof (char)*80);

      /* insert current element into treestore */
      gtk_tree_store_append (global_task_store, &iter, parent);
      gtk_tree_store_set (global_task_store, &iter, ID, atoi(argv[0]), DESCRIPTION, argv[1],-1);
      parent=&iter;

      /* remember the max index seen until now */
      if (atoi(argv[0]) > max_task_id)
        max_task_id = atoi(argv[0]);

      /* prepare select statement for children */
      strcpy (query, "select id, description, cftime, parent from tasks where parent=");
      query = strcat(query,argv[0]);

      /*  and call recursively the load_to_treestore when reading datas
          in order to load also children */
      if (sqlite_exec (sqliteh, query, load_to_treestore, parent, &err))
      {
        gpe_error_box(err);
        g_free (err);
        return 99;
      }
      g_free (query);
      return 0;
    }
}

int initial_loading(void)
{
    char *err;
    GtkTreeIter *parent = NULL;

    sqlite_exec (sqliteh, "select id, description, cftime, parent from tasks where parent=0", load_to_treestore, parent, &err);
    free (err);
}

static int
scan_log_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      struct task *t = arg;
      if (argv[1])
	{
	  if (!strcmp (argv[1], "START"))
	    {
	      t->started = TRUE;
	      return 1;
	    }
	  else if (!strcmp (argv[1], "STOP"))
	    return 1;
	}
    }
  return 0;
}

static int
scan_journal_cb (void *arg, int argc, char **argv, char **names)
{
  time_t ti;

  if (argc == 3)
    {
      ti = atol(argv[0]);
      if (!strcmp(argv[1],"START"))
       {
         if (time_start != 0)
          {
            journal_list_add_line(time_start, ti, note_start, _("open"));
          }
           time_start = ti;
           note_start = g_strdup(argv[2]);
       }
       else if (!strcmp(argv[1],"STOP"))
        {
          if (time_start)
            {
              journal_list_add_line(time_start, ti, note_start, argv[2]);
              g_free(note_start);
              time_start = 0;
            }
        }
    }

  return 0;

}

void
scan_journal (struct task *t)
{
  GSList *iter;

  for (iter = t->children; iter; iter = iter->next)
    {
      struct task *tc = iter->data;

      tc->started = FALSE;
	  scan_journal (tc);
    }
        {
	  char *err;
	  int r;

	  r = sqlite_exec_printf (sqliteh, "select time, action, info from log where task=%d order by time", 
				  scan_journal_cb, t, &err,
				  t->id);
	  if (r != 0 && r != SQLITE_ABORT)
	    {
	      gpe_error_box (err);
	      g_free (err);
	      return;
	    }
	}
}

struct task *
new_task (gchar *description, guint parent)
{
  char *err;
  guint new_id = max_task_id + 1;

  if (sqlite_exec_printf (sqliteh,
			  "insert into tasks values (%d, '%q', %d, %d)",
			  NULL, NULL, &err,
			  new_id, description, 0, parent))
    {
      gpe_error_box (err);
      g_free (err);
      return NULL;
    }

  return internal_note_task (new_id, description, 0, parent);
}

void
delete_task (int idx)
{
  char *err;

  if (sqlite_exec_printf (sqliteh,
			  "delete from tasks where id=%d",
			  NULL, NULL, &err, idx))
  {
    gpe_error_box (err);
    free (err);
    return;
  }

  if (sqlite_exec_printf (sqliteh,
			  "delete from log where task=%d",
			  NULL, NULL, &err, idx))
    {
      gpe_error_box (err);
      free (err);
      return;
    }
}

void delete_child(gpointer data, gpointer user_data)
{
  guint idx = (guint) data;
  delete_task(idx);
}

void delete_children (int idx)
{
  children_list = NULL;

  find_children (idx);
  g_slist_foreach (children_list, delete_child, NULL);

}

int find_children_cb (void *arg, int argc, char **argv, char **names)
{
  char *err;

  if (argc=2)
    {
    if (sqlite_exec_printf (sqliteh,
                          "select id, parent from tasks where parent=%s",
                          find_children_cb,
                          NULL, &err, argv[0]))
      {
        gpe_error_box(err);
        g_free (err);
        return;
      }

    children_list = g_slist_prepend (children_list,GINT_TO_POINTER(atoi(argv[0])));

    return 0;

    }
}

int find_children (int idx)
{
  char *err;

  if (sqlite_exec_printf  (sqliteh,
                          "select id, parent from tasks where parent=%d",
                          find_children_cb, NULL, &err, idx))
    {
      gpe_error_box(err);
      free (err);
      return;
    }
}
