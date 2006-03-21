/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef SQL_H
#define SQL_H

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>

struct task
{
  guint id;
  gchar *description;
  guint time_cf;
  //GSList *children; // is it really necessary?
  guint parent;
  guint todo_id;
  gboolean started;
};

typedef enum
  {
    START,
    STOP,
    NOTE
  } action_t;

typedef enum
{
  ID,
  ICON_STARTED,
  DESCRIPTION,
  STATUS,
  STARTED,
  TODO_ID,
  NUM_COLS
} columns_store;

extern gboolean sql_start (void);
//extern GSList *tasks, *root;
extern GSList *children_list;

extern struct task *new_task (gchar *description, guint parent, guint todo_id);
//extern void delete_task (struct task *t);
extern void delete_task (int idx);
extern void delete_children (int idx);

extern gboolean log_entry (action_t action, time_t time, struct task *task, const char *info);
extern gboolean check_if_item_has_logs (int id);
extern void scan_logs (GSList *);
//extern void scan_journal (struct task *tstart);
extern void scan_journal (gint id);

extern int load_to_treestore(void *arg, int argc, char **argv, char **names);

extern GtkTreeStore *global_task_store;
extern GtkTreeIter *global_task_iter;

#endif
