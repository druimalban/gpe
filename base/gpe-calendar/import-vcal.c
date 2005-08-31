/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include <mimedir/mimedir-vcal.h>

#include <gpe/vevent.h>
#include <gpe/vtodo.h>

#include <sqlite.h>

#define _(x)  (x)

#define CALENDAR_DB_NAME "/.gpe/calendar"
#define TODO_DB_NAME "/.gpe/todo"

static void
do_import_vevent (MIMEDirVEvent *event)
{
  sqlite *db;
  GSList *tags, *i;
  char *buf;
  const gchar *home;
  char *err = NULL;
  int id;

  home = g_get_home_dir ();
  
  buf = g_strdup_printf ("%s%s", home, CALENDAR_DB_NAME);

  db = sqlite_open (buf, 0, &err);
  g_free (buf);

  if (db == NULL)
    {
      gpe_error_box (err);
      free (err);
      return;
    }
 
  if (sqlite_exec (db, "insert into calendar_urn values (NULL)", NULL, NULL, &err) != SQLITE_OK)
    {
      gpe_error_box (err);
      free (err);
      sqlite_close (db);
      return;
    }

  id = sqlite_last_insert_rowid (db);

  tags = vevent_to_tags (event);

  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *t = i->data;

      sqlite_exec_printf (db, "insert into calendar values ('%d', '%q', '%q')", NULL, NULL, NULL,
			  id, t->tag, t->value);
    }
  
  gpe_tag_list_free (tags);

  sqlite_close (db);
}

static void
do_import_vtodo (MIMEDirVTodo *todo)
{
  sqlite *db;
  GSList *tags, *i;
  char *buf;
  const gchar *home;
  char *err = NULL;
  int id;

  home = g_get_home_dir ();
  
  buf = g_strdup_printf ("%s%s", home, TODO_DB_NAME);

  db = sqlite_open (buf, 0, &err);
  g_free (buf);

  if (db == NULL)
    {
      gpe_error_box (err);
      free (err);
      return;
    }
 
  if (sqlite_exec (db, "insert into todo_urn values (NULL)", NULL, NULL, &err) != SQLITE_OK)
    {
      gpe_error_box (err);
      free (err);
      sqlite_close (db);
      return;
    }

  id = sqlite_last_insert_rowid (db);

  tags = vtodo_to_tags (todo);

  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *t = i->data;

      sqlite_exec_printf (db, "insert into todo values ('%d', '%q', '%q')", NULL, NULL, NULL,
			  id, t->tag, t->value);
    }
  
  gpe_tag_list_free (tags);

  sqlite_close (db);
}

static void
do_import_vcal (MIMEDirVCal *vcal)
{
  GSList *list, *iter;

  list = mimedir_vcal_get_event_list (vcal);
  
  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVEvent *vevent;
      vevent = MIMEDIR_VEVENT (list->data);
      do_import_vevent (vevent);
    }

  g_slist_free (list);

  list = mimedir_vcal_get_todo_list (vcal);

  for (iter = list; iter; iter = iter->next)
    {
      MIMEDirVTodo *vtodo;
      vtodo = MIMEDIR_VTODO (list->data);
      do_import_vtodo (vtodo);
    }

  g_slist_free (list);
}

int
import_vcal (const gchar *data, size_t len)
{
  MIMEDirProfile *profile = NULL;
  MIMEDirVCal *cal = NULL;
  gchar *str;
  GError *error = NULL;

  str = g_malloc (len + 1);
  memcpy (str, data, len);
  str[len] = 0;

  profile = mimedir_profile_new(NULL);
  mimedir_profile_parse(profile, str, &error);
  if (!error)
    cal = mimedir_vcal_new_from_profile (profile, &error);
 
  g_free (str);

  if (cal)
    {
      do_import_vcal (cal);
      g_object_unref (cal);
      g_object_unref (profile);
      return 0;
    }
  else
    {
      return -1;
    }
}
