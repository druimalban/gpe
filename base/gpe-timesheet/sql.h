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

struct task
{
  guint id;
  gchar *description;
  guint time_cf;
  GSList *children;
  struct task *parent;
  gboolean started;
};

typedef enum
  {
    START,
    STOP,
    NOTE
  } action_t;

extern gboolean sql_start (void);
extern GSList *tasks, *root;

extern struct task *new_task (gchar *description, struct task *parent);
extern void delete_task (struct task *t);

extern gboolean log_entry (action_t action, time_t time, struct task *task, char *info);
extern void scan_logs (GSList *);
extern void scan_journal (GSList *list);

#endif
