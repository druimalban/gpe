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

#define DB_NAME_CONTACTS "/.gpe/contacts"
#define DB_NAME_VCARD "/.gpe/vcard"
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

static int
check_one_tag (void *arg, int argc, char **argv, char **names)
{
  GSList **list = (GSList **) arg;
  gchar *utag = g_ascii_strup (argv[0], -1);

  if (argc && (argv[0][0] != 0) && strcmp(argv[0], utag))
    *list = g_slist_append(*list, g_strdup(argv[0]));
  g_free(utag);
  return 0;
}

static void 
db_check_tags(void)
{
  char *err;
  int r;
  GSList *list = NULL;

  r = sqlite_exec (db, "select tag from contacts",
                   check_one_tag, &list, &err);

  if (r)
    {
      gpe_error_box (err);
      free (err);
    }
  
  if (list)
    {
      GSList *iter;

      for (iter = list; iter; iter = iter->next)
        {
           gchar *utag = g_ascii_strup (iter->data, -1);
           if (strcmp(iter->data, utag))
             {
               r = sqlite_exec_printf (db, "update contacts set tag='%q' where (tag='%q')", 
                                   NULL, NULL, &err, utag, iter->data);
               if (r) 
                 {
                   fprintf(stderr, "Err: %s\n", err);
                   free (err);
                 }
             }
           g_free(iter->data);
           g_free(utag);
        }
      g_slist_free(list);
    }
}

int 
db_open (gboolean open_vcard)
{
  /* open persistent connection */
  char *errmsg;
  char *buf;

  buf = g_strdup_printf("%s/%s", g_get_home_dir(), 
                                 open_vcard ? DB_NAME_VCARD : DB_NAME_CONTACTS);
  
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

  db_check_tags();
  
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
sort_entries(struct person * a, struct person * b)
{
  gint s = strcoll (a->family_name, b->family_name);
  
  return s ? s : strcoll (a->name, b->name);
}

static int
read_name (void *arg, int argc, char **argv, char **names)
{
  struct person *p = (struct person *)arg;
  p->name = g_strdup (argv[1]);

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
read_one_entry (void *arg, int argc, char **argv, char **names)
{
  struct person *p = new_person ();
  GSList **list = (GSList **) arg;

  p->id = atoi (argv[0]);
  sqlite_exec_printf (db, "select tag,value from contacts where (urn=%d) and ((tag='NAME') or (tag='name'))",
		      read_name, p, NULL, p->id);
  sqlite_exec_printf (db, "select tag,value from contacts where (urn=%d) and ((tag='FAMILY_NAME') or (tag='family_name'))",
		      read_family_name, p, NULL, p->id);
  
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

GSList *
db_get_entries_list (const gchar *name, const gchar *cat)
{
  GSList *list = NULL;
  char *err;
  int r;
  gchar *strsearch;

  gboolean no_cat = (!cat) || (!strlen(cat));
  gboolean no_name = (!name) && (!strlen(name));

  if (no_name && no_cat) 
    return db_get_entries();
  
  strsearch = g_strdup_printf("%%%s%%", name);

  if (no_name && !no_cat) 
    {
      r = sqlite_exec_printf 
        (db, "select distinct urn from contacts where tag = 'CATEGORY' and value like '%q'",
         read_one_entry, &list, &err, cat);
    }
  else if (no_cat)
    {
      r = sqlite_exec_printf 
        (db, "select distinct urn from contacts where (tag = 'NAME' or tag = 'GIVEN_NAME' or tag = 'FAMILY_NAME' or tag = 'COMPANY' or tag = 'MIDDLE_NAME') and value like '%q'",
         read_one_entry, &list, &err, strsearch);
    } 
  else 
    {
      r = sqlite_exec_printf 
        (db, "select urn from contacts where tag = 'CATEGORY' and value = '%q' and urn IN (select distinct urn from contacts where (tag = 'NAME' or tag = 'GIVEN_NAME' or tag = 'FAMILY_NAME' or tag = 'COMPANY' or tag = 'MIDDLE_NAME') and value like '%q')",
         read_one_entry, &list, &err, cat, strsearch);
    }
 
  g_free(strsearch);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}


GSList *
db_get_entries_finddlg (const gchar *str, const gchar *cat)
{
  GSList *list = NULL;
  char *err;
  int r;
  gchar *strsearch;
  gboolean has_cat = cat && strlen(cat);
  gboolean has_str = str && strlen(str);

  if (!has_cat && !has_str) 
    {
      return db_get_entries();
    } 

  strsearch = g_strdup_printf("%%%s%%", str); 

  if (has_cat && has_str) 
    {
      r = sqlite_exec_printf (db, "select urn from contacts where (tag = 'CATEGORY' or tag = 'category') and value = '%q' and urn in (select distinct urn from contacts where value like '%q');",
                              read_one_entry, &list, &err, cat, strsearch);
    } 
  else if (has_cat)
    {
      r = sqlite_exec_printf (db, "select distinct urn from contacts where category = '%q'",
                              read_one_entry, &list, &err, cat);
    }
  else
    {
      r = sqlite_exec_printf (db, "select distinct urn from contacts where value like '%q'",
                              read_one_entry, &list, &err, strsearch);
    }
  
  g_free(strsearch);
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

void
db_update_config_values (gint group, gchar * identifier, gchar * value)
{
  db_delete_config_values(group, identifier);
  db_add_config_values(group, identifier, value);
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
      if (c == 1 && r >= 1)
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

gchar *
db_compress (void)
{
  gchar *err = NULL;
  gchar **list = NULL;
  gint r, c;

  sqlite_get_table_printf (db, "vacuum", &list, &r, &c, &err);
  if (err)
    {
      fprintf (stderr,"e: %s\n", err);
      return err;
    }
  if (list != NULL)
    {
      sqlite_free_table (list);
    }
  return NULL;
}

gint
db_size (void)
{
  gchar *fn = g_strdup_printf("%s/" DB_NAME_CONTACTS, g_get_home_dir());
  int result = 0;
  struct stat dat;
  
  if (!stat(fn, &dat))
    result = dat.st_size / 1024;
  g_free(fn);
  return result;
}
