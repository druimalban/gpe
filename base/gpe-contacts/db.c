/*
 * Copyright (C) 2001, 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#include <gpe/errorbox.h>

#include "db.h"
#include "structure.h"
#include "support.h"
#include "main.h"

#define DB_NAME "/.gpe/contacts"
#define LAYOUT_NAME "/.gpe/contacts-layout.xml"
#define DEFAULT_STRUCTURE PREFIX "/share/gpe-contacts/default-layout.xml"
#define LARGE_STRUCTURE PREFIX "/share/gpe-contacts/contacts-layout-bigscreen.xml"

#include <sqlite.h>

static sqlite *db;

extern void migrate_old_categories (sqlite *db);

static const char *schema_str = "create table contacts (urn INTEGER NOT NULL, tag TEXT NOT NULL, value TEXT NOT NULL);";

static const char *schema2_str = "create table contacts_urn (urn INTEGER PRIMARY KEY);";

/* this one is for config data */
static const char *schema4_str = "create table contacts_config (id INTEGER PRIMARY KEY,	cgroup INTEGER NOT NULL, cidentifier TEXT NOT NULL, cvalue TEXT);";

int 
db_open (void)
{
  /* open persistent connection */
  char *errmsg;
  
  char *buf;
  size_t len;
  char *home = getenv ("HOME");
  if (home == NULL)
    home = "";
  
  len = strlen (home) + strlen (DB_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, DB_NAME);
  
  db = sqlite_open (buf, 0, &errmsg);
  g_free (buf);
	
  if (db == NULL) 
    {
      gpe_error_box (errmsg);
      free (errmsg);
      return -1;
    }

  sqlite_exec (db, schema_str, NULL, NULL, NULL);
  sqlite_exec (db, schema2_str, NULL, NULL, NULL);
  /* if we can create this table, we should add some defaults */
  if (sqlite_exec (db, schema4_str, NULL, NULL, NULL) == SQLITE_OK)
    {
      db_add_config_values (CONFIG_PANEL, _("Name"), "NAME");
      db_add_config_values (CONFIG_PANEL, _("Phone"), "HOME.TELEPHONE");
      db_add_config_values (CONFIG_PANEL, _("EMail"), "HOME.EMAIL");
    }

  gpe_pim_categories_init ();

  migrate_old_categories (db);

  return 0;
}

gboolean
load_structure (void)
{
  gchar *buf;
  size_t len;
  char *home = getenv ("HOME");
  gboolean rc = TRUE;
  if (home == NULL)
    home = "";
  
  len = strlen (home) + strlen (LAYOUT_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, LAYOUT_NAME);
  
  if (access (buf, F_OK))
    {
      if (mode_large_screen)
        rc = read_structure (LARGE_STRUCTURE);
      else
        rc = read_structure (DEFAULT_STRUCTURE);
    }
  else
    rc = read_structure (buf);
  
  g_free (buf);
  return rc;
}

struct tag_value *
new_tag_value (gchar * tag, gchar * value)
{
  struct tag_value *t = g_malloc (sizeof (struct tag_value));
  t->tag = tag;
  t->value = value;
  return t;
}

void
free_tag_values (GSList * list)
{
  GSList *i;
  for (i = list; i; i = i->next)
    {
      struct tag_value *t = i->data;
      g_free (t->tag);
      g_free (t->value);
      g_free (t);
    }
  g_slist_free (list);
}

void
update_tag_value (struct tag_value *t, gchar * value)
{
  g_free (t->value);
  t->value = value;
}

struct person *
new_person (void)
{
  struct person *p = g_malloc (sizeof (struct person));
  memset (p, 0, sizeof (*p));
  return p;
}

void
discard_person (struct person *p)
{
  free_tag_values (p->data);
  g_free (p->name);
  g_free (p->given_name);
  g_free (p->middle_name);
  g_free (p->family_name);
  g_free (p->company);
  g_free (p);
}

gboolean
new_person_id (guint * id)
{
  char *err;
  int r = sqlite_exec (db, "insert into contacts_urn values (NULL)",
		       NULL, NULL, &err);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  *id = sqlite_last_insert_rowid (db);
  return TRUE;
}

gint 
sort_entries (struct person * a, struct person * b)
{
  return strcoll (a->name, b->name);
}

static int
read_name (void *arg, int argc, char **argv, char **names)
{
  struct person *p = (struct person *)arg;
  p->name = g_strdup (argv[1]);

  return 0;
}

static int
read_given_name (void *arg, int argc, char **argv, char **names)
{
  struct person *p = (struct person *)arg;
  p->given_name = g_strdup (argv[1]);

  return 0;
}

static int
read_family_name (void *arg, int argc, char **argv, char **names)
{
  struct person *p = (struct person *)arg;
  p->family_name = g_strdup (argv[1]);

  return 0;
}

static int
read_middle_name (void *arg, int argc, char **argv, char **names)
{
  struct person *p = (struct person *)arg;
  p->middle_name = g_strdup (argv[1]);

  return 0;
}
static int

read_company (void *arg, int argc, char **argv, char **names)
{
  struct person *p = (struct person *)arg;
  p->company = g_strdup (argv[1]);

  return 0;
}

static int
read_one_entry (void *arg, int argc, char **argv, char **names)
{
  struct person *p = new_person ();
  GSList **list = (GSList **) arg;

  p->id = atoi (argv[0]);
  sqlite_exec_printf (db, "select tag,value from contacts where (urn=%d) and ((tag='NAME') or (tag='name'))",
		      read_name, p, NULL, p->id);
  sqlite_exec_printf (db, "select tag,value from contacts where (urn=%d) and ((tag='GIVEN_NAME') or (tag='given_name'))",
		      read_given_name, p, NULL, p->id);
  sqlite_exec_printf (db, "select tag,value from contacts where (urn=%d) and (tag='MIDDLE_NAME')",
		      read_middle_name, p, NULL, p->id);
  sqlite_exec_printf (db, "select tag,value from contacts where (urn=%d) and ((tag='FAMILY_NAME') or (tag='family_name'))",
		      read_family_name, p, NULL, p->id);
  sqlite_exec_printf (db, "select tag,value from contacts where (urn=%d) and (tag='COMPANY')",
		      read_company, p, NULL, p->id);

  /* we support contacts without name now */
  if (!p->name) 
    p->name = g_strdup("");
  *list = g_slist_prepend (*list, p);

  return 0;
}

GSList *
db_get_entries (void)
{
  GSList *list = NULL;
  char *err;
  int r;

  r = sqlite_exec (db, "select urn from contacts_urn",
		   read_one_entry, &list, &err);

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}

static int
read_entry_data (void *arg, int argc, char **argv, char **names)
{
  struct person *p = arg;
  db_set_multi_data (p, argv[0], g_strdup (argv[1]));
  return 0;
}

struct person *
db_get_by_uid (guint uid)
{
  struct person *p = new_person ();
  char *err;
  int r;

  p->id = uid;
  r = sqlite_exec_printf (db, "select tag,value from contacts where urn=%d",
  read_entry_data, p, &err, uid);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      discard_person (p);
      return NULL;
    }
  
  return p;
}

gboolean
db_delete_by_uid (guint uid)
{
  int r;
  gchar *err;
  gboolean rollback = FALSE;

  r = sqlite_exec (db, "begin transaction", NULL, NULL, &err);
  if (r)
    goto error;

  rollback = TRUE;

  r = sqlite_exec_printf (db, "delete from contacts where urn='%d'",
			  NULL, NULL, &err, uid);
  if (r)
    goto error;
  
  r = sqlite_exec_printf (db, "delete from contacts_urn where urn='%d'",
			  NULL, NULL, &err, uid);
  if (r)
    goto error;

  r = sqlite_exec (db, "commit transaction", NULL, NULL, &err);
  if (r)
    goto error;

  return TRUE;

error:
  if (rollback)
    sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
  gpe_error_box (err);
  free (err);
  return FALSE;
}

struct tag_value *
db_find_tag (struct person *p, gchar * tag)
{
  struct tag_value *t = NULL;
  GSList *iter;
  for (iter = p->data; iter; iter = iter->next)
    {
      struct tag_value *id = (struct tag_value *) iter->data;
      if (!strcasecmp (id->tag, tag))
	{
	  t = id;
	  break;
	}
    }
  return t;
}

void
db_set_multi_data (struct person *p, gchar * tag, gchar * value)
{
  struct tag_value *t = new_tag_value (g_strdup (tag), value);
  p->data = g_slist_prepend (p->data, t);
}

void
db_set_data (struct person *p, gchar * tag, gchar * value)
{
  struct tag_value *t = db_find_tag (p, tag);

  if (t)
    {
      update_tag_value (t, value);
    }
  else
    {
      t = new_tag_value (g_strdup (tag), value);
      p->data = g_slist_prepend (p->data, t);
    }
}

void
db_delete_tag (struct person *p, gchar * tag)
{
  GSList *l;
  
  for (l = p->data; l;)
    {
      GSList *n = l->next;
      struct tag_value *t = l->data;
      if (!strcasecmp (t->tag, tag))
	{
	  g_free (t->tag);
	  g_free (t->value);
	  g_free (t);
	  p->data = g_slist_remove (p->data, t);
	}
      l = n;
    }
}

gboolean
commit_person (struct person *p)
{
  GSList *iter;
  int r;
  gchar *err;
  gboolean rollback = FALSE;
  time_t modified;

  time (&modified);

  r = sqlite_exec (db, "begin transaction", NULL, NULL, &err);
  if (r)
    goto error;

  rollback = TRUE;

  if (p->id)
    {
      r = sqlite_exec_printf (db, "delete from contacts where urn='%d'",
			      NULL, NULL, &err, p->id);
      if (r)
	goto error;
    }
  else
    {
      if (new_person_id (&p->id) == FALSE)
	{
	  sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
	  return FALSE;
	}
    }

  for (iter = p->data; iter; iter = iter->next)
    {
      struct tag_value *v = iter->data;
      if (v->value && v->value[0] && (strcmp(v->tag,"MODIFIED")))
        {
	      r = sqlite_exec_printf (db,
				  "insert into contacts values(%d,'%q','%q')",
				  NULL, NULL, &err, p->id, v->tag, v->value);
          if (r)
	        goto error;
        }
    }

  if (sqlite_exec_printf (db,
			  "insert into contacts values(%d, 'MODIFIED', %d)",
			  NULL, NULL, &err, p->id, modified))
    goto error;

  r = sqlite_exec (db, "commit transaction", NULL, NULL, &err);
  if (r)
    goto error;

  return TRUE;

error:
  if (rollback)
    sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
  gpe_error_box (err);
  free (err);
  return FALSE;
}

gboolean
db_insert_category (gchar * name, guint * id)
{
  char *err;
  int r =
    sqlite_exec_printf (db,
			"insert into contacts_category values (NULL, '%q')",
			      NULL, NULL, &err, name);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  
  *id = sqlite_last_insert_rowid (db);
  return TRUE;
}
 
gboolean
db_delete_category (guint id)
{
  char *err;
  int r = sqlite_exec_printf (db, "delete from contacts_category where id=%d", 
			      NULL, NULL, &err, id);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  return TRUE;
}

static int
read_one_entry_alpha (void *arg, int argc, char **argv, char **names)
{
  struct person *p = new_person ();
  GSList **list = (GSList **) arg;

  p->id = atoi (argv[0]);
  p->name = g_strdup (argv[1]);

  if (p->name)
    *list = g_slist_prepend (*list, p);
  else
    discard_person (p);

  return 0;
}

GSList *
db_get_entries_alpha (const gchar * alphalist)
{
  GSList *list = NULL;

  char *command, *tmp = NULL, *modifier;
  char *err;
  int r, s, t;

  const gchar *cursym;
  gssize temp_char_len;
  gchar *temp_char_low;
  gchar *temp_char_up;

  if (! g_utf8_validate (alphalist, -1, &cursym))
    {
      gpe_error_box ("Not a valid UTF-8 string");
      return NULL;
    }

  s = g_utf8_strlen (alphalist, -1);
  command = g_strdup ("");
	
  if (alphalist[0] == '!')
    {
      t = 1;
      modifier = g_strdup (" NOT ");
    }	  
  else
    {
      t = 0;  
      modifier = g_strdup (" ");
    }

  cursym = alphalist + t;

  for (r = t; r < s; r++)
    {
      tmp = g_utf8_offset_to_pointer (cursym, 1);
      temp_char_len = tmp - cursym;

      temp_char_low = g_utf8_strdown (cursym, temp_char_len);
      temp_char_up  = g_utf8_strup (cursym, temp_char_len);
      
      tmp = g_strdup_printf ("%s(value like '%s%%') or (value like '%s%%')",
			     command, temp_char_up, temp_char_low);
      
      g_free (command);
      g_free (temp_char_low);
      g_free (temp_char_up);

      if (r != s-1)
        command = g_strdup_printf ("%s or", tmp);
      
      cursym = g_utf8_next_char (cursym);
      
    }
  
  command = g_strdup_printf ("select urn, value from contacts "
			     "where ((( tag = 'NAME') or ( tag = 'name')) and%s(%s)) "
			     "order by value desc", modifier, tmp);
  r = sqlite_exec (db, command, read_one_entry_alpha, &list, &err);
  g_free (tmp);

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }
  
  return list;
}

gint
db_get_tag_list (gchar *** list)
{
  gchar *err;
  gint r, c;

  sqlite_get_table (db, "select distinct tag from contacts", list, &r, &c,
		    &err);
  if (err)
    g_free (err);
  return r;
}


gint
db_get_config_values (gint group, gchar *** list)
{
  gchar *err, *statement;
  gint r, c;

  statement = g_strdup_printf ("select cidentifier,cvalue from contacts_config where cgroup=%i",
			       group);

  sqlite_get_table (db, statement, list, &r, &c, &err);
  if (err)
    {
      fprintf (stderr,"e: %s\n", err);
      g_free (err);
    }

  g_free (statement);
  return r;
}

void
db_add_config_values (gint group, gchar * identifier, gchar * value)
{
  gchar *err;
  gint r, c;
  gchar **list;
  sqlite_get_table_printf (db,
			   "insert into contacts_config (cgroup,cidentifier,cvalue) values(%i,'%q','%q')",
			   &list, &r, &c, &err, group, identifier, value);
  if (err)
    {
      fprintf (stderr,"e: %s\n", err);
      g_free (err);
    }
  if (r)
    {
      sqlite_free_table (list);
    }
}


void
db_delete_config_values (gint group, gchar * identifier)
{
  gchar *err;
  gint r = 0;
  gint c = 0;
  gchar **list;
  sqlite_get_table_printf (db,
			   "delete from contacts_config where (cgroup=%i) and (cidentifier='%q')",
			   &list, &r, &c, &err, group, identifier);
  if (err)
    {
      fprintf (stderr,"e: %s\n", err);
      g_free (err);
    }
  if (r)
    {
      sqlite_free_table (list);
    }
}


gchar *
db_get_config_tag (gint group, const gchar * tagname)
{
  gchar *err = NULL;
  gchar **list = NULL;
  gint r, c;

  sqlite_get_table_printf (db,
			   "select cvalue from contacts_config where (cgroup=%i) and (cidentifier='%q')",
			   &list, &r, &c, &err, group, tagname);
  if (err)
    {
      fprintf (stderr,"e: %s\n", err);
      g_free (err);
      return NULL;
    }
  if (list != NULL)
    {
      err = g_strdup (list[1]);
      sqlite_free_table (list);
    }
  return err;
}

void
db_free_result (char **table)
{
  sqlite_free_table (table);
}
