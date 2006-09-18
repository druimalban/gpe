/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Damien Tanner <dctanner@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define _XOPEN_SOURCE

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sqlite.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>

#include <gpe/errorbox.h>

#include "networks_config_sql.h"

static sqlite *sqliteh;

static const char *fname = "/.gpe/irc";

GSList *sql_networks;

static gint rows = 0;

static struct sql_network *
new_sql_network_internal (int id, const char *name, const char *nick,
                          const char *real_name, const char *password)
{
  struct sql_network *e = g_malloc (sizeof (struct sql_network));

  e->id = id;
  e->name = name;
  e->nick = nick;
  e->real_name = real_name;
  e->password = password;
  e->servers = NULL;

  sql_networks = g_slist_append (sql_networks, e);

  return e;
}

static struct sql_network_server *
new_sql_network_server_internal (int id, const char *name, int port,
                                 struct sql_network *parent_network)
{
  struct sql_network_server *e =
    g_malloc (sizeof (struct sql_network_server));

  e->id = id;
  e->name = name;
  e->port = port;

  parent_network->servers = g_slist_append (parent_network->servers, e);

  return e;
}

struct sql_network *
new_sql_network (const char *name, const char *nick, const char *real_name,
                 const char *password)
{
  char *err;

  if (sqlite_exec_printf
      (sqliteh, "insert into networks values (NULL, '%q', '%q', '%q', '%q')",
       NULL, NULL, &err, name, nick, real_name, password))
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return new_sql_network_internal (sqlite_last_insert_rowid (sqliteh), name,
                                   nick, real_name, password);
}

struct sql_network_server *
new_sql_network_server (const char *name, int port,
                        struct sql_network *network)
{
  char *err;

  if (sqlite_exec_printf
      (sqliteh, "insert into servers values (NULL, '%q', '%d', '%d')", NULL,
       NULL, &err, name, port, network->id))
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return new_sql_network_server_internal (sqlite_last_insert_rowid (sqliteh),
                                          name, port, network);
}

void
edit_sql_network (struct sql_network *network)
{
  char *err;

  if (sqlite_exec_printf
      (sqliteh, "replace into networks values ('%d', '%q', '%q', '%q', '%q')",
       NULL, NULL, &err, network->id, network->name, network->nick,
       network->real_name, network->password))
    {
      gpe_error_box (err);
      free (err);
    }
}


void
edit_sql_network_server (struct sql_network_server *network_server,
                         struct sql_network *network)
{
  char *err;

  if (sqlite_exec_printf
      (sqliteh, "replace into servers values ('%d', '%q', '%d', '%d')", NULL,
       NULL, &err, network_server->id, network_server->name,
       network_server->port, network->id))
    {
      gpe_error_box (err);
      free (err);
    }
}

void
del_sql_network (struct sql_network *e)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "delete from networks where uid=%d",
                          NULL, NULL, &err, e->id))
    {
      gpe_error_box (err);
      free (err);
    }

  sql_networks = g_slist_remove (sql_networks, e);
}

void
del_sql_network_server (struct sql_network *e, struct sql_network_server *s)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "delete from servers where uid=%d",
                          NULL, NULL, &err, s->id))
    {
      gpe_error_box (err);
      free (err);
    }

  e->servers = g_slist_remove (e->servers, s);
}

void
del_sql_networks_all ()
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "delete from networks", NULL, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
    }
}

static int
sql_network_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 5 && argv[0] && argv[1])
    new_sql_network_internal (atoi (argv[0]), g_strdup (argv[1]),
                              g_strdup (argv[2]), g_strdup (argv[3]),
                              g_strdup (argv[4]));
  rows++;
  return 0;
}

static gint
find_network (gconstpointer a, gconstpointer b)
{
  int n = (int) b;
  struct sql_network *sql_net = (struct sql_network *) a;

  if (sql_net->id < n)
    return -1;
  if (sql_net->id == n)
    return 0;
  if (sql_net->id > n)
    return 1;
}

static int
sql_network_server_callback (void *arg, int argc, char **argv, char **names)
{
  GSList *server_network;

  if (argc == 4 && argv[0] && argv[1])
    {
      server_network =
        g_slist_find_custom (sql_networks, (gconstpointer) atoi (argv[3]),
                             find_network);
      if (server_network != NULL)
        new_sql_network_server_internal (atoi (argv[0]), g_strdup (argv[1]),
                                         atoi (argv[2]),
                                         (struct sql_network *)
                                         server_network->data);
    }
  return 0;
}

/* --- */

static void
add_default_sql_networks (void)
{
  struct sql_network *e = g_malloc (sizeof (struct sql_network));

  e = new_sql_network ("FreeNode", "", "", "");
  new_sql_network_server ("irc.freenode.net", 6667, e);
}

int
networks_sql_start (void)
{
  static const char *schema1_str =
    "create table networks (uid INTEGER PRIMARY KEY, name TEXT, nick TEXT, real_name TEXT, password TEXT)";
  static const char *schema2_str =
    "create table servers (uid INTEGER PRIMARY KEY, name TEXT, port INTEGER, network INTEGER)";

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
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  sqlite_exec (sqliteh, schema1_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema2_str, NULL, NULL, &err);

  if (sqlite_exec
      (sqliteh, "select uid,name,nick,real_name,password from networks",
       sql_network_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }
  if (sqlite_exec (sqliteh, "select uid,name,port,network from servers",
                   sql_network_server_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  if (rows == 0)
    add_default_sql_networks ();

  g_free (buf);
  return 0;
}

void
networks_sql_close (void)
{
  sqlite_close (sqliteh);
}
