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
#include <glib.h>

typedef struct 
{
  client_connection commondata;
  sync_pair *sync_pair;
  char* device_addr;
  char* username;

  pthread_t thread;
  sync_object_type newdbs;		/* XXX */
} gpe_conn;

struct db
{
  const gchar *name;
  int type;

  GList *(*get_changes)(struct db *db, int newdb);
  gboolean (*push_object)(struct db *db, const char *obj, const char *uid,
			  char *returnuid, int *returnuidlen, GError **err);
  gboolean (*delete_object)(struct db *db, const char *uid, gboolean soft);
  
  nsqlc *db;
  time_t last_timestamp;
  time_t current_timestamp;
  gboolean changed;
};

#define GPE_DEBUG(conn, x) fprintf (stderr, "%s\n", (x))

extern GSList *db_list;

extern void gpe_disconnect (struct db *db);

extern gboolean gpe_load_config (gpe_conn *conn);
extern gboolean gpe_save_config (gpe_conn *conn);

extern GSList *fetch_uid_list (nsqlc *db, const gchar *query, ...);
extern GSList *fetch_tag_data (nsqlc *db, const gchar *query_str, guint id);

extern gboolean store_tag_data (nsqlc *db, const gchar *table, guint id, GSList *tags, gboolean delete);

extern void calendar_init (void);
extern void todo_init (void);
extern void contacts_init (void);

extern void *gpe_do_get_changes (void *conn);
extern void *gpe_do_connect (void *conn);
