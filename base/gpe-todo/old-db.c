/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include <sqlite.h>

#include <gpe/errorbox.h>

#include "todo.h"
#include "todo-sql.h"

GSList *lists;

static sqlite *sqliteh_here;

struct todo_list 
{
  const char *title;
  int id;
  GList *items;
};

struct todo_list *
new_list (int id, const char *title)
{
  struct todo_list *t = g_malloc (sizeof (struct todo_list));
  t->items = NULL;
  t->title = title;
  t->id = id;
  	    
  lists = g_slist_append (lists, t);
  return t;
}

static int
list_callback0 (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      new_list (atoi (argv[0]), g_strdup (argv[1]));

      converted_category (g_strdup (argv[1]));
    }

  return 0;
}

static int
item_callback0 (void *arg, int argc, char **argv, char **names)
{
  if (argc == 6)
    {
      int list = atoi (argv[1]);
      char *summary = argv[2];
      char *description = argv[3];
      int state = atoi (argv[4]);
      char *due = argv[5];
      struct todo_item *i = g_malloc (sizeof (struct todo_item));
      
      GSList *iter;
      time_t t = (time_t)0;

      memset (i, 0, sizeof (*i));
      
      i->id = sqlite_last_insert_rowid (sqliteh_here);

      i->what = g_strdup (description);
      i->state = state;
      i->summary = g_strdup (summary);
  
      if (due[0])
	{
	  struct tm tm;
	  memset (&tm, 0, sizeof (tm));
	  strptime (due, "%F", &tm);
	  t = mktime (&tm);
	}
	
      i->time = t;
      
      for (iter = lists; iter; iter = iter->next)
	{
	  struct todo_category *t = g_malloc (sizeof (struct todo_category));
          struct todo_list *l = iter->data;
	  if (l->id == list)
	  {
	    t->id = l->id;
	    t->title = l->title;
	    i->categories = g_slist_append (i->categories, (gpointer)(t->id));
	    break;
          }
	}
	
      converted_item (i);
    }
  
  return 0;
}

gboolean
convert_old_db (int oldversion, sqlite *sqliteh)
{
  char *err;
  
  sqliteh_here = sqliteh;
  
  if (oldversion == 0) 
    {
      sqlite_exec (sqliteh, "select uid,description from todo_lists",
		   list_callback0, NULL, NULL);

      sqlite_exec (sqliteh, 
		       "select uid,list,summary,description,state,due_by from todo_items",
		   item_callback0, NULL, NULL);
    }
  
  oldversion = 1; /* set equal to new version */
  
  if (sqlite_exec_printf (sqliteh, 
			  "insert into todo_dbinfo (version) values (%d)", 
			  NULL, NULL, &err, oldversion))
    {
      printf ("sqlite: %s\n", err);
      free (err);
      return FALSE;
    }
    
  return TRUE;
}
