/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "gpe_sync.h"

#include <gpe/tag-db.h>

nsqlc *
gpe_connect_one (gpe_conn *conn, gchar *db, char **err)
{
  gchar *path;
  nsqlc *r;

  path = g_strdup_printf ("%s@%s:.gpe/%s", conn->username, conn->device_addr, db);

  fprintf (stderr, "connecting to %s\n", path);

  r = nsqlc_open_ssh (path, O_RDWR, err);

  g_free (path);

  return r;
}

gboolean
gpe_connect (gpe_conn *conn)
{
  char *err = NULL;

  conn->calendar = gpe_connect_one (conn, "calendar", &err);
  if (!conn->calendar)
    fprintf (stderr, "Error: %s\n", err);
  conn->todo = gpe_connect_one (conn, "todo", &err);
  if (!conn->todo)
    fprintf (stderr, "Error: %s\n", err);
  conn->contacts = gpe_connect_one (conn, "contacts", &err);
  if (!conn->contacts)
    fprintf (stderr, "Error: %s\n", err);

  return TRUE;
}

void
gpe_disconnect (gpe_conn *conn)
{
  if (conn->calendar)
    nsqlc_close (conn->calendar);

  if (conn->contacts)
    nsqlc_close (conn->contacts);

  if (conn->todo)
    nsqlc_close (conn->todo);
}

static int
fetch_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      GSList **data = (GSList **)arg;
      gpe_tag_pair *p = g_malloc (sizeof (*p));

      p->tag = g_strdup (argv[0]);
      p->value = g_strdup (argv[1]);

      *data = g_slist_prepend (*data, p);
    }

  return 0;
}

GSList *
fetch_tag_data (nsqlc *db, const gchar *query_str, guint id)
{
  GSList *data = NULL;

  nsqlc_exec_printf (db, query_str, fetch_callback, &data, NULL, id);

  return data;
}

static int
fetch_uid_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1)
    {
      GSList **data = (GSList **)arg;

      *data = g_slist_prepend (*data, (void *)atoi (argv[0]));
    }

  return 0;
}

GSList *
fetch_uid_list (nsqlc *db, const gchar *query, ...)
{
  GSList *data = NULL;
  va_list ap;

  va_start (ap, query);

  nsqlc_exec_vprintf (db, query, fetch_uid_callback, &data, NULL, ap);

  va_end (ap);

  return data;
}
