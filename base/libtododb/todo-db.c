/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 * Documentation: 2005 Florian Boor <florian@kernelconcepts.de> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

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
#include <gpe/todo-db.h>

static sqlite *sqliteh;
static GSList *todo_db_items;

static unsigned long dbversion;

static const char *fname = "/.gpe/todo";

extern gboolean convert_old_db (int oldversion, sqlite *);
extern void migrate_old_categories (sqlite * db);

/**
 * todo_db_make_todoid:
 *
 * Makes a globally unique identifier, with which an todo item
 * can be referenced in a vcal.
 *
 * Returns: An id of the format <time>.<pid>@<hostname>
 */
char *
todo_db_make_todoid (void)
{
  static char *hostname;
  static char buffer[512];

  /* Here we create an id for the todo, so that we can
   * later reference it in vcals, etc. */
  if ((gethostname (buffer, sizeof (buffer) - 1) == 0) && (buffer[0] != 0))
    hostname = buffer;
  else
    hostname = "localhost";

  return g_strdup_printf ("%lu.%lu@%s",
			  (unsigned long) time (NULL),
			  (unsigned long) getpid (), hostname);
}

/**
 * todo_db_get_items_list:
 *
 * Get a list of all todo items. The returned list points to the internal 
 * list used by libtododb and doesn't need to be freed. It is a good idea 
 * to consider it to be read only.
 *
 * Returns: List of todo items.
 */
GSList *
todo_db_get_items_list (void)
{
  return todo_db_items;
}

/* ------ */

static int
item_data_callback (void *arg, int argc, char **argv, char **names)
{
  struct todo_item *i = (struct todo_item *) arg;

  if (argc == 2 && argv[0] && argv[1])
    {
      if (!strcasecmp (argv[0], "SUMMARY"))
	i->summary = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "DESCRIPTION"))
	i->what = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "STATE"))
	i->state = atoi (argv[1]);
      else if (!strcasecmp (argv[0], "PRIORITY"))
	i->priority = atoi (argv[1]);
      else if (!strcasecmp (argv[0], "TODOID"))
	i->todoid = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "CATEGORY"))
	i->categories =
	  g_slist_append (i->categories, (gpointer) atoi (argv[1]));
      else if (!strcasecmp (argv[0], "DUE"))
	{
	  struct tm tm;
	  memset (&tm, 0, sizeof (tm));
	  if (strptime (argv[1], "%F", &tm))
	    i->time = mktime (&tm);
	}
    }

  return 0;
}

static int
item_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1 && argv[0])
    {
      char *err;
      int id = atoi (argv[0]);
      struct todo_item *i = g_malloc0 (sizeof (struct todo_item));
      i->id = id;
      i->priority = PRIORITY_STANDARD;
      if (sqlite_exec_printf
	  (sqliteh, "select tag,value from todo where uid=%d",
	   item_data_callback, i, &err, id))
	{
	  gpe_error_box (err);
	  free (err);
	  return 0;
	}
      if (i->state == COMPLETED)
	i->was_complete = TRUE;
      todo_db_items = g_slist_append (todo_db_items, i);
    }
  return 0;
}

static int
dbinfo_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1)
    {
      dbversion = atoi (argv[0]);
    }

  return 0;
}

/* --- */
/**
 * todo_db_start:
 * 
 * Initialises libtododb for use.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
todo_db_start (void)
{
  static const char *schema2_str =
    "create table todo (uid INTEGER NOT NULL, tag TEXT NOT NULL, value TEXT)";
  static const char *schema3_str =
    "create table todo_urn (uid INTEGER PRIMARY KEY)";
  static const char *schema_info =
    "create table todo_dbinfo (version integer NOT NULL)";

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

  sqlite_exec (sqliteh, schema_info, NULL, NULL, &err);

  if (sqlite_exec
      (sqliteh, "select version from todo_dbinfo", dbinfo_callback, NULL,
       &err))
    {
      dbversion = 0;
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  if (sqlite_exec (sqliteh, schema2_str, NULL, NULL, &err))
    free (err);
  if (sqlite_exec (sqliteh, schema3_str, NULL, NULL, &err))
    free (err);

  if (dbversion == 1)
    {
      if (sqlite_exec
	  (sqliteh, "select uid from todo_urn", item_callback, NULL, &err))
	{
	  gpe_error_box (err);
	  free (err);
	  return -1;
	}
    }
  else if (dbversion == 0)
    {
      if (sqlite_exec
	  (sqliteh, "select uid from todo_urn", item_callback, NULL, &err))
	{
	  gpe_error_box (err);
	  free (err);
	  return -1;
	}

      convert_old_db (dbversion, sqliteh);
      dbversion = 1;
    }

  migrate_old_categories (sqliteh);

  return 0;
}

/**
 * todo_db_refresh:
 * 
 * Update list of todo items from database.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
todo_db_refresh (void)
{
  char *err;
  GSList *iter;

  for (iter = todo_db_items; iter; iter = g_slist_next (iter))
    todo_db_destroy_item (iter->data);
  g_slist_free (todo_db_items);
  todo_db_items = NULL;

  if (sqlite_exec
      (sqliteh, "select uid from todo_urn", item_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      return -1;
    }

  return 0;
}

/**
 * todo_db_stop:
 * 
 * Deinitialises libtododb and frees all its allocated data.
 */
void
todo_db_stop (void)
{
  GSList *iter;

  for (iter = todo_db_items; iter; iter = g_slist_next (iter))
    todo_db_destroy_item (iter->data);
  g_slist_free (todo_db_items);
  todo_db_items = NULL;

  sqlite_close (sqliteh);
}

gint
list_sort_func (gconstpointer a, gconstpointer b)
{
  const struct todo_item *ia = a, *ib = b;

  if (ia->time == ib->time)
    return 0;

  if (ia->time == 0)
    return 1;
  if (ib->time == 0)
    return -1;

  return ia->time - ib->time;
}

#define insert_values(db, id, key, format, value)	\
	sqlite_exec_printf (db, "insert into todo values (%d, '%q', '" format "')", \
			    NULL, NULL, &err, id, key, value)

gboolean
todo_db_push_item (struct todo_item * i)
{
  char *err;
  gboolean rollback = FALSE;
  GSList *iter;
  time_t modified;

  modified = time (NULL);

  if (sqlite_exec (sqliteh, "begin transaction", NULL, NULL, &err))
    goto error;

  rollback = TRUE;

  if (!i->todoid)
    {
      i->todoid = todo_db_make_todoid ();
    }

  if (sqlite_exec_printf (sqliteh, "delete from todo where uid=%d",
			  NULL, NULL, &err, i->id)
      || (i->summary
	  && insert_values (sqliteh, i->id, "SUMMARY", "%q", i->summary))
      || (i->what
	  && insert_values (sqliteh, i->id, "DESCRIPTION", "%q", i->what))
      || (i->todoid
	  && insert_values (sqliteh, i->id, "TODOID", "%q", i->todoid))
      || insert_values (sqliteh, i->id, "STATE", "%d", i->state))
    goto error;

  if (i->time)
    {
      char d_buf[32];
      struct tm tm;
      localtime_r (&i->time, &tm);
      strftime (d_buf, sizeof (d_buf), "%F", &tm);
      if (insert_values (sqliteh, i->id, "DUE", "%q", d_buf))
	goto error;
    }

  for (iter = i->categories; iter; iter = iter->next)
    {
      if (insert_values (sqliteh, i->id, "CATEGORY", "%d", iter->data))
	goto error;
    }

  if (insert_values (sqliteh, i->id, "PRIORITY", "%d", i->priority))
    goto error;

  if (insert_values (sqliteh, i->id, "MODIFIED", "%d", (int) modified))
    goto error;

  if (sqlite_exec (sqliteh, "commit transaction", NULL, NULL, &err))
    goto error;

  return TRUE;

error:
  if (rollback)
    sqlite_exec (sqliteh, "rollback transaction", NULL, NULL, NULL);
  gpe_error_box (err);
  free (err);
  return FALSE;
}

gboolean
converted_item (struct todo_item * i)
{
  char *err;
  if (sqlite_exec (sqliteh, "insert into todo_urn values (NULL)",
		   NULL, NULL, &err))
    return FALSE;

  i->id = sqlite_last_insert_rowid (sqliteh);

  todo_db_items = g_slist_append (todo_db_items, i);
  return todo_db_push_item (i);
}

/**
 * todo_db_new_item:
 * 
 * Create a new todo item and add it to list and database.
 *
 * Returns: New todo item.
 */
struct todo_item *
todo_db_new_item (void)
{
  char *err;
  struct todo_item *i;

  if (sqlite_exec (sqliteh, "insert into todo_urn values (NULL)",
		   NULL, NULL, &err))
    return NULL;

  i = g_malloc0 (sizeof (struct todo_item));

  i->id = sqlite_last_insert_rowid (sqliteh);

  todo_db_items = g_slist_append (todo_db_items, i);

  return i;
}

/**
 * todo_db_destroy_item:
 * @i: Todo item to destroy.
 *
 * Frees a todo list item struct.
 */
void
todo_db_destroy_item (struct todo_item *i)
{
  g_free ((gpointer) i->what);
  g_free ((gpointer) i->summary);
  g_free ((gpointer) i->todoid);
  g_slist_free (i->categories);
  g_free (i);
}

/**
 * todo_db_delete_item:
 * @i: Todo item to delete.
 * 
 * Deletes an item from the list and database.
 */
void
todo_db_delete_item (struct todo_item *i)
{
  sqlite_exec_printf (sqliteh, "delete from todo where uid=%d",
		      NULL, NULL, NULL, i->id);
  sqlite_exec_printf (sqliteh, "delete from todo_urn where uid=%d",
		      NULL, NULL, NULL, i->id);

  todo_db_items = g_slist_remove (todo_db_items, i);

  todo_db_destroy_item (i);
}

struct todo_item *
todo_db_find_item_by_id (guint uid)
{
  GSList *todo_list = todo_db_get_items_list ();
  GSList *i;

  for (i = todo_list; i; i = g_slist_next (i))
    {
      struct todo_item *t = i->data;
      if (t->id == uid)
	return t;
    }

  return NULL;
}
