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

#include "gpe_sync.h"

GList *
sync_calendar (GList *data, gpe_conn *conn, int newdb)
{
  fetch_uid_list (conn->calendar, "select distinct uid from calendar");

  return data;
}

gboolean
push_calendar (gpe_conn *conn, const char *obj, const char *uid, 
	       char *returnuid, int *returnuidlen, GError **err)
{
  return FALSE;
}

gboolean
delete_calendar (gpe_conn *conn, const char *uid, gboolean soft)
{
  return FALSE;
}
