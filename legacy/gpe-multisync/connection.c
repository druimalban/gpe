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

#define _(x)  gettext(x)

nsqlc *
gpe_connect_one (gpe_conn *conn, const gchar *db, char **err)
{
  gchar *path;
  nsqlc *r;

  path = g_strdup_printf ("%s@%s:.gpe/%s", conn->username, conn->device_addr, db);

  fprintf (stderr, "connecting to %s\n", path);

  r = nsqlc_open_ssh (path, O_RDWR, err);

  g_free (path);

  return r;
}

void *
gpe_do_connect (void *_conn)
{
  GSList *i;
  char* errmsg = NULL;
  gboolean failed = FALSE;
  gpe_conn *conn;

  conn = (gpe_conn *)_conn;

  GPE_DEBUG(conn, "sync_connect");  
  
  /* load the connection attributes */
  if (! gpe_load_config (conn))
  {
    /* failure */
    errmsg = g_strdup (_("Failed to load configuration"));
    sync_set_requestfailederror (errmsg, conn->sync_pair);
    pthread_exit (0);
  }  

  for (i = db_list; i; i = i->next)
    {
      struct db *db = i->data;
  
      db->db = gpe_connect_one (conn, db->name, &errmsg);

      if (!db->db)
	{
	  failed = TRUE;
	  break;
	}
    }

  if (failed)
    {
      for (i = db_list; i; i = i->next)
	{
	  struct db *db = i->data;

	  gpe_disconnect (db);
	}

      sync_set_requestfailederror (g_strdup (errmsg), conn->sync_pair);
      pthread_exit (0);
    }

  sync_set_requestdone (conn->sync_pair);
  pthread_exit (0);
}

void
gpe_disconnect (struct db *db)
{
  if (db->db)
    nsqlc_close (db->db);
  db->db = NULL;
}

void *
gpe_do_get_changes (void *_conn) 
{
  GSList *i;
  GList *changes = NULL;
  sync_object_type retnewdbs = 0;
  change_info *chinfo;
  gpe_conn *conn;
  sync_object_type newdbs;

  conn = (gpe_conn *)_conn;
  newdbs = conn->newdbs;

  GPE_DEBUG(conn, "get_changes"); 

  for (i = db_list; i; i = i->next)
    {
      struct db *db = i->data;
      gchar *filename;
      FILE *fp;

      db->last_timestamp = 0;

      filename = g_strdup_printf ("%s/%s", 
				  sync_get_datapath (conn->sync_pair),
				  db->name);

      fp = fopen (filename, "r");
      if (fp)
	{
	  int i;

	  if (fscanf (fp, "%d", &i))
	    db->last_timestamp = i;

	  fclose (fp);
	}

      g_free (filename);

      nsqlc_get_time (db->db, &db->current_timestamp, NULL);

      if (conn->commondata.object_types & db->type)
	{
	  GList *local_changes = db->get_changes (db, newdbs & db->type);

	  if (local_changes)
	    {
	      db->changed = TRUE;
	      changes = g_list_concat (changes, local_changes);
	    }
	}
    }
  
  /* Allocate the change_info struct */
  chinfo = g_malloc0 (sizeof (change_info));
  chinfo->changes = changes;
  
  /* Did we detect any reset databases */
  chinfo->newdbs = retnewdbs;
  sync_set_requestdata (chinfo, conn->sync_pair);

  pthread_exit (0);
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

gboolean
store_tag_data (nsqlc *db, const gchar *table, guint id, GSList *tags, gboolean delete)
{
  if (delete)
    nsqlc_exec_printf (db, "delete from '%q' where urn=%d", NULL, NULL, NULL, table, id);

  while (tags)
    {
      gpe_tag_pair *p = tags->data;

      nsqlc_exec_printf (db, "insert into '%q' values (%d, '%q', '%q')", NULL, NULL, NULL,
			 table, id, p->tag, p->value);

      tags = tags->next;
    }

  return TRUE;
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
