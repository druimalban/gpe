/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include "multisync.h"

#include <nsqlc.h>

/* opie connection definition */
typedef struct {
  client_connection commondata;
  sync_pair *sync_pair;
  char* device_addr;
  char* username;

  nsqlc *calendar, *contacts, *todo;
} gpe_conn;

#define GPE_DEBUG(conn, x) fprintf (stderr, "%s\n", (x))

extern gboolean gpe_connect (gpe_conn *conn);
extern void gpe_disconnect (gpe_conn *conn);

extern gboolean gpe_load_config (gpe_conn *conn);
extern gboolean gpe_save_config (gpe_conn *conn);

extern GList *sync_calendar (GList *data, gpe_conn *conn, int newdb);
extern GList *sync_contacts (GList *data, gpe_conn *conn, int newdb);
extern GList *sync_todo (GList *data, gpe_conn *conn, int newdb);

extern GSList *fetch_uid_list (nsqlc *db, const gchar *query, ...);
extern GSList *fetch_tag_data (nsqlc *db, const gchar *query_str, guint id);
