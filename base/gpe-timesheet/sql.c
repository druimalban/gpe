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
#include <gpe/todo-db.h>

#include "sql.h"

#define _(x) gettext(x)

static const char *fname = "/.gpe/timesheet";

static const char *schema_str = 
"create table tasks (id integer, description text, cftime integer, parent integer);"
"create table log (action text, task integer, time integer, info text);";
static const char *schema_new_tasks_table = "create table tasks (id integer, description text, cftime integer, parent integer, todo_id integer);";
static const char *schema_with_todo_str =
"create table tasks (id integer, description text, cftime integer, parent integer, todo_id integer);"
"create table log (action text, task integer, time integer, info text);";

static sqlite *sqliteh;
static guint max_task_id;
static time_t time_start = 0;
static gchar *note_start = NULL;

GSList *children_list;

static const char *actions[] = { "START", "STOP" };

int check_if_item_has_logs_cb(void *arg, int argc, char **argv, char **names)
{
  int *check = arg;
  *check = 1;
  return 0;
}

gboolean
check_if_item_has_logs(int id)
{
  int check=0;
  gchar *err;

  if (sqlite_exec_printf (sqliteh,
              "select task from log where task = %d",
              check_if_item_has_logs_cb, &check, &err, id))
    {
      gpe_error_box (err);
      free (err);
      return TRUE;
    }
    free(err);
    if (check)
      return TRUE;
    else
      return FALSE;
}

gboolean
log_entry (action_t action, time_t time, struct task *task, const char *info)
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
internal_note_task (guint id, gchar *text, guint elapsed, guint pt, guint todo_id)
{
  struct task *t;

  t = g_malloc (sizeof (struct task));
  t->id = id;
  t->description = g_strdup (text);
  t->time_cf = elapsed;
  //t->children = NULL;
  t->parent = pt;
  t->started = 0;
  t->todo_id = todo_id;

  if (id > max_task_id)
    max_task_id = id;

  return t;
}

static int
load_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 5)
  {
    int todo_id;
    guint parent = atoi (argv[3]);

    if (argv[4] == NULL)
      todo_id = -1;
    else
      todo_id = atoi(argv[4]);
    internal_note_task (atoi (argv[0]), argv[1], atoi (argv[2]), 
            parent, todo_id);
  }

  return 0;
}

/* used by sql_check_tasks_table_update to convert database */
static int
tasks_get_old_entries_cb (void *arg, int argc, char **argv, char **names)
{
  struct task *t = g_malloc (sizeof(struct task));
  GSList **list = (GSList **) arg;

  t->id = atoi (argv[0]);
  t->description = g_strdup (argv[1]);
  t->time_cf = atoi (argv[2]);
  t->parent = atoi (argv[3]);

  *list = g_slist_prepend (*list, t);

  return 0;
}

/* used by sql_check_tasks_table_update to convert database */
GSList *
tasks_get_old_entries (void)
{
  GSList *list = NULL;
  char *err;
  int r;

  r = sqlite_exec (sqliteh, "select id, description, cftime, parent from tasks",
                   tasks_get_old_entries_cb, &list, &err);

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}

static void
sql_check_tasks_table_update(void)
{/* check if the tasks table is already updated*/
  
  int r;
  gchar *err = NULL;
  GSList *entries = NULL, *iter=NULL;
  

  /* check if we have the field todo_id */
  r = sqlite_exec (sqliteh, "select todo_id from tasks",
                   NULL, NULL, &err);

  if (r) 
    { /* test > 0 means that we need to recreate tasks table with new column */
      /* we ask a confirmation to the user */
      GtkWidget *warning = gtk_dialog_new_with_buttons(
                            _("Warning!"), NULL, GTK_DIALOG_MODAL,
                            GTK_STOCK_OK , GTK_RESPONSE_ACCEPT,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
      GtkWidget *label = gtk_label_new(_("Your archives are from an old version of timesheet. This version will automatically upgrade your datas, but a backup copy is advisable prior to continue. Press OK to continue, or CANCEL to abort"));
      //g_signal_connect_swapped (warning, "response", G_CALLBACK (gtk_widget_destroy), GTK_WINDOW(warning));
      gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (warning)->vbox), label);
      gtk_widget_show_all (warning);
      gint result = gtk_dialog_run(GTK_DIALOG(warning));
      gtk_widget_destroy (warning);

      if (result == GTK_RESPONSE_ACCEPT)
        {
          struct task *t;
          free (err);
          r = sqlite_exec (sqliteh, "begin transaction", NULL, NULL, &err);
          if (r)
              goto error;
          entries = tasks_get_old_entries();
          r = sqlite_exec (sqliteh, "drop table tasks",
                          NULL, NULL, &err);
          if (r)
              goto error;
          r = sqlite_exec (sqliteh, schema_new_tasks_table, NULL, NULL, &err);
          if (r)
              goto error;
          for (iter = entries; iter; iter=iter->next)
            {
              t = (struct task*)iter->data;
              if (t)
                {
                    sqlite_exec_printf(sqliteh, "insert into tasks values (%d, '%s', '%d', '%d', NULL)", 
                                      NULL, NULL, &err, t->id, t->description, t->time_cf, t->parent);
                    g_free (t);
                  }
            }
          r = sqlite_exec (sqliteh, "commit transaction", NULL, NULL, &err);
          if (r)
              goto error;
          g_slist_free(entries);
    
          return;

          error:
            {
              gpe_error_box_fmt ("Couldn't convert database data to new format: %s", err);
              g_free(err);
              sqlite_exec (sqliteh, "rollback transaction", NULL, NULL, &err);
            }
        }
    }
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

  sqlite_exec (sqliteh, schema_with_todo_str, NULL, NULL, &err);

  sql_check_tasks_table_update();

  if (!global_task_store)
    global_task_store = gtk_tree_store_new (NUM_COLS,
    G_TYPE_UINT, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_UINT);
  else
    gtk_tree_store_clear(global_task_store);

  /* here we load all tasks of exclusive
  ** pertinence of timesheet, i.e. tasks with no todo parents */
  if (sqlite_exec (sqliteh, "select id, description, cftime, parent, todo_id from tasks where parent=0 and todo_id is null", load_to_treestore, NULL, &err))
  {
    gpe_error_box (err);
    free (err);
    return FALSE;
  }

  return TRUE;
}

gboolean
sql_append_todo(void)
  {
  /* this one is for loading into treestore also todo's */
  GSList *todo_list = NULL;
  GSList *list = NULL;
  GtkTreeIter iter;
  gchar *err;

  if (!todo_db_start())
    {
      todo_list = todo_db_get_items_list();

      for (list = todo_list; list; list = list->next)
        {
          struct todo_item *i = list->data;

          if (i->state == IN_PROGRESS)
            {
              gtk_tree_store_append (global_task_store, &iter, NULL);
              gtk_tree_store_set (global_task_store, &iter,
                                  ID, 0,
                                  TODO_ID, i->id,
                                  DESCRIPTION, i->summary,
                                  -1);
              /* for each of these todos, we go and check into db
              ** for tasks children of todo */
                if (sqlite_exec_printf (sqliteh, "select id, description, cftime, parent, todo_id from tasks where parent=0 and todo_id ='%d'", load_to_treestore, &iter, &err, i->id))
                  {
                    gpe_error_box (err);
                    free (err);
                    return FALSE;
                   }
             }
        }
      todo_db_stop();
    }
  return TRUE;
}

/* this is a recursive routine that
 * loads data from DB directly into GtkTreeViews
 * reading the current node and all its children and subchildren */

int 
load_to_treestore(void *arg, int argc, char **argv, char **names)
{
  if (argc==5)
    {
      GtkTreeIter iter, *parent;
      char *query, *err;
      int test;

      parent=arg;
      query = g_malloc( sizeof (char)*80);

      /* insert current element into treestore */
      gtk_tree_store_append (global_task_store, &iter, parent);
      if (argv[4] == NULL)
        gtk_tree_store_set (global_task_store, &iter, ID, atoi(argv[0]), DESCRIPTION, argv[1], -1);
      else
        gtk_tree_store_set (global_task_store, &iter, ID, atoi(argv[0]), DESCRIPTION, argv[1], TODO_ID, atoi(argv[4]), -1);
      parent=&iter;

      /* remember the max index seen until now */
      if (atoi(argv[0]) > max_task_id)
        max_task_id = atoi(argv[0]);

      /* prepare select statement for children */
      strcpy (query, "select id, description, cftime, parent, todo_id from tasks where parent=");
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

static int
scan_log_cb (void *arg, int argc, char **argv, char **names)
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
  gint *id = arg;
  gint type;

  if (argc == 4)
    {
      ti = atol(argv[0]);
      if (!strcmp(argv[1],"START"))
       {
         if (time_start != 0)
          {
            type = 0;
            journal_list_add_line(time_start, ti, note_start, _("open log! pls chk"), id, type);
          }
           time_start = ti;
           note_start = g_strdup(argv[2]);
       }
       else if (!strcmp(argv[1],"STOP"))
        {
          if (time_start)
            {
              type = 1;
              journal_list_add_line(time_start, ti, note_start, argv[2], id, type);
              g_free(note_start);
              time_start = 0;
            }
        }
    }

  return 0;

}

void
scan_journal (gint id)
{
  char *err;
  int r;
  time_start = 0;

  r = sqlite_exec_printf (sqliteh, "select time, action, info, task from log where task=%d order by time", 
			  scan_journal_cb, &id, &err,
			  id);
  if (r != 0 && r != SQLITE_ABORT)
    {
      gpe_error_box (err);
      g_free (err);
      return;
    }
}

struct task *
new_task (gchar *description, guint parent, guint todo_id)
{ /* we are challenging with two indexes here...
  ** parent is the task index parent of the one we are inserting...
  ** if todo_id has some value it means that this is a parent task
  ** which is child of a todo item */
  char *err;
  guint new_id = max_task_id + 1;

    if (todo_id == 0 || parent)
      { /* it means that it is a task which belongs only to tasks */
        if (sqlite_exec_printf (sqliteh,
                  "insert into tasks values (%d, '%q', %d, %d, NULL)",
                  NULL, NULL, &err,
                  new_id, description, 0, parent))
          {
            gpe_error_box (err);
            g_free (err);
            return NULL;
          }
      }
    else
      { /* this is a task used to connect todos with their children tasks */
        if (sqlite_exec_printf (sqliteh,
                  "insert into tasks values (%d, '%q', %d, %d, %d)",
                  NULL, NULL, &err,
                  new_id, description, 0, 0, todo_id))
          {
            gpe_error_box (err);
            g_free (err);
            return NULL;
          }
      }

  return internal_note_task (new_id, description, 0, parent, todo_id);
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

void 
delete_child(gpointer data, gpointer user_data)
{
  guint idx = (guint) data;
  delete_task(idx);
}

void 
delete_children (int idx)
{
  children_list = NULL;

  find_children (idx);
  g_slist_foreach (children_list, delete_child, NULL);
}

int 
find_children_cb (void *arg, int argc, char **argv, char **names)
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

int 
find_children (int idx)
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

void 
update_log (gchar *info, time_t oldtime, time_t newtime, int task, action_t action)
{
  char *err;
  
  if (sqlite_exec_printf  (sqliteh,
                          "update log set info='%s', time=%d where (task=%d and action = '%s' and time=%d)",
                          NULL, NULL, &err, info, newtime, task, actions[action], oldtime))
    {
      gpe_error_box(err);
      free (err);
      return;
    }
}
