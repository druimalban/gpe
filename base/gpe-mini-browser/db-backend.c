/*
 * gpe-mini-browser v0.17
 *
 * Basic web browser based on gtk-webcore 
 * 
 * db-backend.c Backend for sqlite to store bookmarks.
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <sqlite.h>

#include <gpe/errorbox.h>

#include "gpe-mini-browser.h"

#define DB_NAME "/.gpe/bookmarks"

//#define DEBUG /* uncomment for debug info */


static sqlite *db = NULL;

/* initialize db, return 0 if successful */
int
start_db (void)
{
  static const char *create_str =
    "create table bookmarks (bookmark TEXT NOT NULL);";

  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;

  if (home == NULL)
    home = "";
  len = strlen (home) + strlen (DB_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, DB_NAME);
  db = sqlite_open (buf, 0, &err);
  if (db == NULL)
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  sqlite_exec (db, create_str, NULL, NULL, &err);
  g_free (buf);

  return 0;
}

void
stop_db (void)
{
  sqlite_close (db);
}

int
insert_new_bookmark (char *bookmark)
{
  char *err;

  if (sqlite_exec_printf (db,
			  "insert into bookmarks values ('%s')",
			  NULL, NULL, &err, bookmark))
    {
      g_free (err);
      return 1;
    }
  g_free (err);

  return 0;

}

int
remove_bookmark (char *bookmark)
{
  char *err;

  if (sqlite_exec_printf (db,
			  "delete from bookmarks where bookmark='%s'",
			  NULL, NULL, &err, bookmark))
    {
      g_free (err);
      return 1;
    }

  g_free (err);

  return 0;
}

int
load_db_data (void *tree, int argc, char **argv, char **columnNames)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *location;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  while (argc > 0)
    {
      location = *argv;
#ifdef DEBUG
      printf ("bookmark value is %s\n", location);
#endif
      gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, location, -1);
      argv++;
      argc--;
    }
  return 0;

}

int
refresh_or_load_db (GtkWidget * tree)
{
  char *err;

  if (sqlite_exec (db, "select * from bookmarks", load_db_data, tree, &err))
    {
      g_free (err);
      return 1;
    }

  g_free (err);

  return 0;
}
