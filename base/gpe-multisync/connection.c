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

#include "multisync.h"

#include "gpe_sync.h"

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
