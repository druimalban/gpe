/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
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
static GSList *todo_db_categories, *todo_db_items;

static unsigned long dbversion;

static const char *fname = "/.gpe/todo";

extern gboolean convert_old_db (int oldversion, sqlite *);

static struct todo_category *
new_category_internal (int id, const char *title)
{
  struct todo_category *t = g_malloc (sizeof (struct todo_category));

  t->title = title;
  t->id = id;

  todo_db_categories = g_slist_append (todo_db_categories, t);

  return t;
}

GSList *
todo_db_get_items_list(void)
{
  return todo_db_items;
}

GSList *
todo_db_get_categories_list(void)
{
  return todo_db_categories;
}
	
int 
converted_category (const char *title)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "insert into todo_categories values (NULL, '%q')",
			  NULL, NULL, &err, title))
    {
      gpe_error_box (err);
      free (err);
      return -1;
    }

  new_category_internal (sqlite_last_insert_rowid (sqliteh), title);
  return sqlite_last_insert_rowid (sqliteh);
}

struct todo_category *
todo_db_new_category (const char *title)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "insert into todo_categories values (NULL, '%q')",
			  NULL, NULL, &err, title))
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return new_category_internal (sqlite_last_insert_rowid (sqliteh), title);
}

struct todo_category *
todo_db_category_find_by_id (int i)
{
  GSList *iter;

  for (iter = todo_db_categories; iter; iter = iter->next)
    {
      struct todo_category *c = (struct todo_category *)iter->data;
      if (c->id == i)
	return c;
    }

  return NULL;
}

void
todo_db_del_category (struct todo_category *c)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "delete from todo_categories where uid=%d",
			  NULL, NULL, &err,
			  c->id))
    {
      gpe_error_box (err);
      free (err);
    }

  todo_db_categories = g_slist_remove (todo_db_categories, c);
}

/* ------ */

static int
item_data_callback (void *arg, int argc, char **argv, char **names)
{
  struct todo_item *i = (struct todo_item *)arg;

  if (argc == 2 && argv[0] && argv[1])
    {
      if (!strcmp (argv[0], "SUMMARY"))
	i->summary = g_strdup (argv[1]);
      else if (!strcmp (argv[0], "DESCRIPTION"))
	i->what = g_strdup (argv[1]);
      else if (!strcmp (argv[0], "STATE"))
	i->state = atoi (argv[1]);
      else if (!strcmp (argv[0], "CATEGORY"))
	{
	  struct todo_category *c = todo_db_category_find_by_id (atoi (argv[1]));
	  if (c)
	    i->categories = g_slist_append (i->categories, c);;
	}
      else if (!strcmp (argv[0], "DUE"))
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
      struct todo_item *i = g_malloc (sizeof (struct todo_item));
      memset (i, 0, sizeof (*i));
      i->id = id;
      if (sqlite_exec_printf (sqliteh, "select tag,value from todo where uid=%d",
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

static int
category_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2 && argv[0] && argv[1])
    new_category_internal (atoi (argv[0]), g_strdup (argv[1]));
  return 0;
}

/* --- */

int
todo_db_start (void)
{
  static const char *schema1_str = 
    "create table todo_categories (uid INTEGER PRIMARY KEY, description text)";
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
  
  if (sqlite_exec (sqliteh, "select version from todo_dbinfo", dbinfo_callback, NULL, &err))
    {
      dbversion=0;
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema1_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema2_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema3_str, NULL, NULL, &err);
     
  if (dbversion==1) 
    {
      
      if (sqlite_exec (sqliteh, "select uid,description from todo_categories",
		   category_callback, NULL, &err))
      {
        gpe_error_box (err);
        free (err);
        return -1;
      }
      
      if (sqlite_exec (sqliteh, "select uid from todo_urn", item_callback, NULL, &err))
      {
        gpe_error_box (err);
        free (err);
        return -1;
      }
      
    }
    
  else if (dbversion==0) 
    {
      if (sqlite_exec (sqliteh, "select uid,description from todo_categories",
		   category_callback, NULL, &err))
      {
        gpe_error_box (err);
        free (err);
        return -1;
      }
      
      if (sqlite_exec (sqliteh, "select uid from todo_urn", item_callback, NULL, &err))
      {
        gpe_error_box (err);
        free (err);
        return -1;
      }
      
      convert_old_db (dbversion, sqliteh);
      dbversion=1;
    }

  return 0;
}

void
todo_db_stop (void)
{
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
	sqlite_exec_printf (db, "insert into todo values (%d, '%q', '" ## format ## "')", \
			    NULL, NULL, &err, id, key, value)

gboolean
todo_db_push_item (struct todo_item *i)
{
  char *err;
  gboolean rollback = FALSE;
  GSList *iter;

  if (sqlite_exec (sqliteh, "begin transaction", NULL, NULL, &err))
    goto error;

  rollback = TRUE;
  
  if (sqlite_exec_printf (sqliteh, "delete from todo where uid=%d",
			  NULL, NULL, &err, i->id)
      || (i->summary && insert_values (sqliteh, i->id, "SUMMARY", "%q", i->summary))
      || (i->what && insert_values (sqliteh, i->id, "DESCRIPTION", "%q", i->what))
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
      if (insert_values (sqliteh, i->id, "CATEGORY", "%d", ((struct todo_category *)iter->data)->id))
	goto error;
    }

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
converted_item (struct todo_item *i)
{
  char *err;
  if (sqlite_exec (sqliteh, "insert into todo_urn values (NULL)",
		   NULL, NULL, &err))
    return FALSE;
  
  i->id = sqlite_last_insert_rowid (sqliteh);

  todo_db_items = g_slist_append (todo_db_items, i);
  return todo_db_push_item (i);
}

struct todo_item *
todo_db_new_item (void)
{
  char *err;
  struct todo_item *i;
  if (sqlite_exec (sqliteh, "insert into todo_urn values (NULL)",
		   NULL, NULL, &err))
    return NULL;
  
  i = g_malloc (sizeof (struct todo_item));
  memset (i, 0, sizeof (*i));

  i->id = sqlite_last_insert_rowid (sqliteh);

  todo_db_items = g_slist_append (todo_db_items, i);

  return i;
}

void 
todo_db_delete_item (struct todo_item *i)
{
  sqlite_exec_printf (sqliteh, "delete from todo where uid=%d",
		      NULL, NULL, NULL,
		      i->id);
  sqlite_exec_printf (sqliteh, "delete from todo_urn where uid=%d",
		      NULL, NULL, NULL,
		      i->id);

  todo_db_items = g_slist_remove (todo_db_items, i);

  g_free ((gpointer)i->what);
  g_free ((gpointer)i->summary);
  g_free (i);
}
