/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2006 Philip Blundell <philb@gnu.org>
 *               2004 - 2006 Florian Boor <florian@kernelconcepts.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#include <gpe/errorbox.h>

#include "gpe/contacts-db.h"

#define _(x) (x)

#define DB_NAME_CONTACTS "/.gpe/contacts"
#define DB_NAME_VCARD "/.gpe/vcard"

#include <sqlite.h>

static sqlite *contacts_db = NULL;

#ifndef IS_HILDON
extern void contacts_db_migrate_old_categories (sqlite * db);
#endif

/* contacts full data in tag/value pairs */
static const char *contacts_schema_str =
  "create table contacts (urn INTEGER NOT NULL, tag TEXT NOT NULL, value TEXT NOT NULL);";

/* entry data for identification and absic information */
static const char *contacts_schema2_str =
  "create table contacts_urn (urn INTEGER PRIMARY KEY, name TEXT, family_name TEXT, company TEXT);";

/* this one is for config data */
static const char *contacts_schema4_str =
  "create table contacts_config (id INTEGER PRIMARY KEY,	cgroup INTEGER NOT NULL, cidentifier TEXT NOT NULL, cvalue TEXT);";


/**
 * contacts_new_person:
 *
 * Allocates and initializes a new #contacts_person data structure.
 * The returend value needs to be destroyed with #contacts_discard_person.
 * 
 * Returns: A new allocated contacts_person structure.
 *
 */
inline struct contacts_person *
contacts_new_person (void)
{
  struct contacts_person *p = g_malloc0 (sizeof (struct contacts_person));
  return p;
}

/* used by contacts_check_table_update to convert database */
static int
contacts_read_name (void *arg, int argc, char **argv, char **names)
{
  struct contacts_person *p = (struct contacts_person *) arg;
  p->name = g_strdup (argv[0]);

  return 0;
}

/* used by contacts_check_table_update to convert database */
static int
contacts_read_family_name (void *arg, int argc, char **argv, char **names)
{
  struct contacts_person *p = (struct contacts_person *) arg;
  p->family_name = g_strdup (argv[0]);

  return 0;
}

/* used by contacts_check_table_update to convert database */
static int
contacts_read_one_entry_old (void *arg, int argc, char **argv, char **names)
{
  struct contacts_person *p = contacts_new_person ();
  GSList **list = (GSList **) arg;

  p->id = atoi (argv[0]);
  sqlite_exec_printf (contacts_db,
		      "select value from contacts where (urn=%d) and (tag='NAME')",
		      contacts_read_name, p, NULL, p->id);
  sqlite_exec_printf (contacts_db,
		      "select value from contacts where (urn=%d) and (tag='FAMILY_NAME')",
		      contacts_read_family_name, p, NULL, p->id);

  /* we support contacts without name now */
  if (!p->name)
    p->name = g_strdup ("");
  *list = g_slist_prepend (*list, p);

  return 0;
}

/* used by contacts_check_table_update to convert database */
GSList *
contacts_db_get_entries_old (void)
{
  GSList *list = NULL;
  char *err;
  int r;

  r = sqlite_exec (contacts_db, "select urn from contacts_urn",
		   contacts_read_one_entry_old, &list, &err);

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}

/* check contacts_urn table and update to new scheme if necessary */
static void
contacts_check_table_update (void)
{
  int r;
  gchar *err = NULL;
  GSList *entries = NULL, *iter = NULL;


  /* check if we have the company field */
  r = sqlite_exec (contacts_db, "select company from contacts_urn",
		   NULL, NULL, &err);

  if (r)			/* r > 0 indicates a failure, need to recreate that table with new schema */
    {
      struct contacts_person *p;

      free (err);
      r = sqlite_exec (contacts_db, "begin transaction", NULL, NULL, &err);
      if (r)
	goto error;
      entries = contacts_db_get_entries_old ();
      r = sqlite_exec (contacts_db, "drop table contacts_urn",
		       NULL, NULL, &err);
      if (r)
	goto error;
      r = sqlite_exec (contacts_db, contacts_schema2_str, NULL, NULL, NULL);
      if (r)
	goto error;
      for (iter = entries; iter; iter = iter->next)
	{
	  p = (struct contacts_person *) iter->data;
	  if (p)
	    {
	      sqlite_exec_printf (contacts_db,
				  "insert into contacts_urn values (%d, '%q', '%q', '%q')",
				  NULL, NULL, &err, p->id, p->name,
				  p->family_name, p->company);
	      contacts_discard_person (p);
	    }
	}
      r = sqlite_exec (contacts_db, "commit transaction", NULL, NULL, &err);
      if (r)
	goto error;
      g_slist_free (entries);

      /* add some additional indexes */
      r =
	sqlite_exec (contacts_db, "create index idxtag on contacts (tag)",
		     NULL, NULL, &err);
      if (r)
	goto error;

      return;

    error:
      {
	gpe_error_box_fmt ("Couldn't convert database data to new format: %s",
			   err);
	g_free (err);
	sqlite_exec (contacts_db, "rollback transaction", NULL, NULL, &err);
      }
    }
}

static int
contacts_check_one_tag (void *arg, int argc, char **argv, char **names)
{
  GSList **list = (GSList **) arg;
  gchar *utag = g_ascii_strup (argv[0], -1);

  if (argc && (argv[0][0] != 0) && strcmp (argv[0], utag))
    *list = g_slist_append (*list, g_strdup (argv[0]));
  g_free (utag);
  return 0;
}

static void
contacts_db_check_tags (void)
{
  char *err;
  int r;
  GSList *list = NULL;

  r = sqlite_exec (contacts_db, "select tag from contacts",
		   contacts_check_one_tag, &list, &err);

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
	  if (strcmp (iter->data, utag))
	    {
	      r =
		sqlite_exec_printf (contacts_db,
				    "update contacts set tag='%q' where (tag='%q')",
				    NULL, NULL, &err, utag, iter->data);
	      if (r)
		{
		  fprintf (stderr, "Err: %s\n", err);
		  free (err);
		}
	    }
	  g_free (iter->data);
	  g_free (utag);
	}
      g_slist_free (list);
    }
}

/**
 * contacts_db_open:
 * @open_vcard: Wether to open the personal vCard database.
 * 
 * Open and initalise the contacts database. This also performs
 * an up-to-date check and if necessary a database update.
 * If you open your personal vCard database you access the databse
 * file that contains the personal vCard(s) used by other tools.
 *
 * This API is likely to change soon.
 *
 * Returns: 0 on success, error code otherwise.
 */
int
contacts_db_open (gboolean open_vcard)
{
  /* open persistent connection */
  char *errmsg;
  char *buf;

  buf = g_strdup_printf ("%s/%s", g_get_home_dir (),
			 open_vcard ? DB_NAME_VCARD : DB_NAME_CONTACTS);
  if (contacts_db)
    sqlite_close (contacts_db);
  contacts_db = sqlite_open (buf, 0, &errmsg);
  g_free (buf);

  if (contacts_db == NULL)
    {
      gpe_error_box (errmsg);
      free (errmsg);
      return -1;
    }

  sqlite_exec (contacts_db, contacts_schema_str, NULL, NULL, NULL);
  sqlite_exec (contacts_db, contacts_schema2_str, NULL, NULL, NULL);
  /* if we can create this table, we should add some defaults */
  if (sqlite_exec (contacts_db, contacts_schema4_str, NULL, NULL, NULL) ==
      SQLITE_OK)
    {
      contacts_db_add_config_values (CONFIG_PANEL, _("Name"), "NAME");
      contacts_db_add_config_values (CONFIG_PANEL, _("Phone"),
				     "HOME.TELEPHONE");
      contacts_db_add_config_values (CONFIG_PANEL, _("EMail"), "HOME.EMAIL");
    }
#ifndef IS_HILDON
  else
    contacts_check_table_update ();

  contacts_db_migrate_old_categories (contacts_db);
#endif
  contacts_db_check_tags ();

  return 0;
}

/**
 * contacts_db_close:
 * 
 * Close the contacts database.
 * 
 * Returns: 0 on success
 */
int
contacts_db_close (void)
{
  if (contacts_db)
    sqlite_close (contacts_db);
  contacts_db = NULL;
  return 0;
}

struct contacts_tag_value *
contacts_new_tag_value (gchar * tag, gchar * value)
{
  struct contacts_tag_value *t =
    g_malloc (sizeof (struct contacts_tag_value));
  t->tag = tag;
  t->value = value;
  return t;
}

inline void
contacts_free_tag_values (GSList * list)
{
  GSList *i;
  for (i = list; i; i = i->next)
    {
      struct contacts_tag_value *t = i->data;
      g_free (t->tag);
      g_free (t->value);
      g_free (t);
    }
  g_slist_free (list);
}

void
contacts_update_tag_value (struct contacts_tag_value *t, gchar * value)
{
  g_free (t->value);
  t->value = value;
}

void
contacts_discard_person (struct contacts_person *p)
{
  contacts_free_tag_values (p->data);
  g_free (p->name);
  g_free (p->family_name);
  g_free (p->company);
  g_free (p);
}

gboolean
contacts_new_person_id (guint * id)
{
  char *err;
  int r =
    sqlite_exec (contacts_db,
		 "insert into contacts_urn values (NULL, NULL, NULL, NULL)",
		 NULL, NULL, &err);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  *id = sqlite_last_insert_rowid (contacts_db);
  return TRUE;
}

gint
contacts_sort_entries (struct contacts_person * a, struct contacts_person * b)
{
  gint s = 0;
  if (a->family_name && b->family_name)
    s = strcoll (a->family_name, b->family_name);
  return s ? s : strcoll (a->name, b->name);
}

static int
contacts_read_one_entry (void *arg, int argc, char **argv, char **names)
{
  struct contacts_person *p = contacts_new_person ();
  GSList **list = (GSList **) arg;

  p->id = atoi (argv[0]);

  if (argv[1])
    p->name = g_strdup (argv[1]);
  if (argv[2])
    p->family_name = g_strdup (argv[2]);
  if (argv[3])
    p->company = g_strdup (argv[3]);

  /* we support contacts without name now */
  if (!p->name)
    p->name = g_strdup ("");
  *list = g_slist_prepend (*list, p);

  return 0;
}

GSList *
contacts_db_get_entries (void)
{
  GSList *list = NULL;
  char *err;
  int r;

  r = sqlite_exec (contacts_db,
             "select urn, name, family_name, company from contacts_urn",
             contacts_read_one_entry, &list, &err);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}

GSList *
contacts_db_get_entries_list (const gchar * name, const gchar * cat)
{
  GSList *list = NULL;
  char *err;
  int r;

  gboolean no_cat = (!cat) || (!cat[0]);
  gboolean no_name = (!name) || (!name[0]);

  if (no_name && no_cat)
    return contacts_db_get_entries ();

  if (no_name && !no_cat)
    {
      r = sqlite_exec_printf (contacts_db,
                 "select contacts_urn.urn, contacts_urn.name, contacts_urn.family_name, contacts_urn.company "
                 "from contacts_urn, contacts where contacts_urn.urn = contacts.urn "
                 "and contacts.tag = 'CATEGORY' and contacts.value like '%q%%'",
                 contacts_read_one_entry, &list, &err, cat);
    }
  else if (no_cat)
    {
      r = sqlite_exec_printf (contacts_db, 
                 "select urn, name, family_name, company from contacts_urn where name like '%q%%' "
	             "or family_name like '%q%%' or company like '%q%%'",
	             contacts_read_one_entry, &list, &err, name, name, name);
    }
  else
    {
      r = sqlite_exec_printf (contacts_db, 
                 "select urn, name, family_name, company from contacts where tag = 'CATEGORY' "
                 "and value = '%q%%' and urn IN "
                 "(select distinct urn from contacts where "
                 "(tag = 'NAME' or tag = 'GIVEN_NAME' "
                 "or tag = 'FAMILY_NAME' or tag = 'COMPANY')"
                 "and value like '%q%%')",
                 contacts_read_one_entry, &list, &err, cat, name);
    }

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}


GSList *
contacts_db_get_entries_finddlg (const gchar * str, const gchar * cat)
{
  GSList *list = NULL;
  char *err;
  int r;
  gboolean has_cat = cat && (cat[0]);
  gboolean has_str = str && (str[0]);

  if (!has_cat && !has_str)
    {
      return contacts_db_get_entries ();
    }

  if (has_cat && has_str)
    {
      r = sqlite_exec_printf (contacts_db,
                "select distinct contacts_urn.urn, contacts_urn.name, contacts_urn.family_name, contacts_urn.company "
                "from contacts_urn, contacts where (contacts_urn.urn = contacts.urn) and (contacts.tag = 'CATEGORY') "
                "and contacts.value = '%q' and contacts.urn in (select distinct urn from contacts where value like '%%%q%%');",
			    contacts_read_one_entry, &list, &err, cat, str);
    }
  else if (has_cat)
    {
      r = sqlite_exec_printf (contacts_db,
                "select distinct contacts_urn.urn, contacts_urn.name, contacts_urn.family_name, contacts_urn.company "
                "from contacts_urn, contacts where (contacts_urn.urn = contacts.urn) and (contacts.tag = 'CATEGORY') "
                "and contacts.value = '%q'",
                contacts_read_one_entry, &list, &err, cat);
    }
  else
    {
      r = sqlite_exec_printf (contacts_db,
                "select distinct contacts_urn.urn, contacts_urn.name, contacts_urn.family_name, contacts_urn.company "
                "from contacts_urn, contacts where (contacts_urn.urn = contacts.urn) "
                "and contacts.urn in (select distinct urn from contacts where value like '%%%q%%');",
                contacts_read_one_entry, &list, &err, str);
    }

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}

GSList *
contacts_db_get_entries_filtered (const gchar * filter)
{
  GSList *list = NULL;
  char *err = NULL;
  int r;

  if (filter && !strncmp (filter, "***", 3))
    return contacts_db_get_entries ();

  if (filter)
    r = sqlite_exec_printf (contacts_db,
              "select urn, name, family_name, company from contacts_urn "
              "where (family_name like '%c%%' or family_name like '%c%%' or family_name like '%c%%')",
              contacts_read_one_entry, &list, &err, filter[0], filter[1], filter[2]);
  else
    r = sqlite_exec (contacts_db,
              "select urn, name, family_name from contacts_urn "
              "where lower(substr(family_name,1,1)) not between 'a' and 'z'",
              contacts_read_one_entry, &list, &err);

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}

GSList *
contacts_db_get_entries_list_filtered (const gchar * str,
				       const gchar * filter,
				       const gchar * cat)
{
  GSList *list = NULL;
  char *err = NULL;
  int r = 0;
  gboolean has_cat = cat && cat[0];
  gboolean has_str = str && str[0];

  if (filter && !strncmp (filter, "***", 3))
    return contacts_db_get_entries_list (str, cat);

  if (!has_cat && !has_str)
    {
      return contacts_db_get_entries_filtered (filter);
    }
  else if (!has_str && has_cat)
    {
      r = sqlite_exec_printf (contacts_db,
                "select contacts_urn.urn, contacts_urn.name, contacts_urn.family_name, contacts_urn.company "
                "from contacts_urn, contacts where contacts_urn.urn = contacts.urn "
                "and contacts.tag = 'CATEGORY' and contacts.value like '%q%%' and "
                "(contacts_urn.family_name like '%c%%') and " 
                "(contacts_urn.family_name like '%c%%' or contacts_urn.family_name like '%c%%')",
                contacts_read_one_entry, &list, &err, cat, filter[0], filter[1], filter[2]);
    }
  else if (has_cat)
    {
      r = sqlite_exec_printf (contacts_db,
                "select contacts_urn.urn, contacts_urn.name, contacts_urn.family_name, contacts_urn.company "
                "from contacts_urn, contacts where contacts_urn.urn = contacts.urn "
                "and contacts.tag = 'CATEGORY' and contacts.value like '%%%q%%' and "
                "(contacts_urn.family_name like '%c%%') and "
                "(contacts_urn.family_name like '%c%%' or contacts_urn.family_name like '%c%%')",
                contacts_read_one_entry, &list, &err, cat, str, filter[0], filter[1], filter[2]);
    }
  else
    {
      r = sqlite_exec_printf (contacts_db,
                     "select distinct contacts_urn.urn, contacts_urn.name, contacts_urn.family_name, contacts_urn.company "
                     "from contacts_urn, contacts where contacts_urn.urn = contacts.urn "
                     "and (contacts_urn.family_name like '%c%%') and (contacts_urn.family_name like '%c%%' or contacts_urn.family_name like '%c%%')",
                     contacts_read_one_entry, &list, &err, str, filter[0], filter[1], filter[2]);
    }

  if (r)
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return list;
}

static int
contacts_read_entry_data (void *arg, int argc, char **argv, char **names)
{
  struct contacts_person *p = arg;
  contacts_db_set_multi_data (p, argv[0], g_strdup (argv[1]));
  return 0;
}

struct contacts_person *
contacts_db_get_by_uid (guint uid)
{
  struct contacts_person *p = contacts_new_person ();
  char *err;
  int r;

  p->id = uid;
  r =
    sqlite_exec_printf (contacts_db,
			"select tag,value from contacts where urn=%d",
			contacts_read_entry_data, p, &err, uid);
  if (r)
    {
      gpe_error_box (err);
      free (err);
      contacts_discard_person (p);
      return NULL;
    }

  return p;
}

gboolean
contacts_db_delete_by_uid (guint uid)
{
  int r;
  gchar *err;
  gboolean rollback = FALSE;

  r = sqlite_exec (contacts_db, "begin transaction", NULL, NULL, &err);
  if (r)
    goto error;

  rollback = TRUE;

  r = sqlite_exec_printf (contacts_db, "delete from contacts where urn='%d'",
			  NULL, NULL, &err, uid);
  if (r)
    goto error;

  r =
    sqlite_exec_printf (contacts_db,
			"delete from contacts_urn where urn='%d'", NULL, NULL, &err, uid);
  if (r)
    goto error;

  r = sqlite_exec (contacts_db, "commit transaction", NULL, NULL, &err);
  if (r)
    goto error;

  return TRUE;

error:
  if (rollback)
    sqlite_exec (contacts_db, "rollback transaction", NULL, NULL, NULL);
  gpe_error_box (err);
  free (err);
  return FALSE;
}

inline struct contacts_tag_value *
contacts_db_find_tag (struct contacts_person *p, gchar * tag)
{
  struct contacts_tag_value *t = NULL;
  GSList *iter;

  for (iter = p->data; iter; iter = iter->next)
    {
      struct contacts_tag_value *id =
	(struct contacts_tag_value *) iter->data;
      if (!strcmp (id->tag, tag))
	{
	  t = id;
	  break;
	}
    }

  return t;
}

void
contacts_db_set_multi_data (struct contacts_person *p, gchar * tag,
			    gchar * value)
{
  struct contacts_tag_value *t =
    contacts_new_tag_value (g_strdup (tag), value);
  p->data = g_slist_prepend (p->data, t);
}

void
contacts_db_set_data (struct contacts_person *p, gchar * tag, gchar * value)
{
  struct contacts_tag_value *t = contacts_db_find_tag (p, tag);

  if (t)
    {
      contacts_update_tag_value (t, value);
    }
  else
    {
      t = contacts_new_tag_value (g_strdup (tag), value);
      p->data = g_slist_prepend (p->data, t);
    }
}

void
contacts_db_delete_tag (struct contacts_person *p, gchar * tag)
{
  GSList *l;

  for (l = p->data; l;)
    {
      GSList *n = l->next;
      struct contacts_tag_value *t = l->data;
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
contacts_commit_person (struct contacts_person *p)
{
  GSList *iter;
  int r;
  gchar *err;
  gboolean rollback = FALSE;
  time_t modified;

  time (&modified);

  r = sqlite_exec (contacts_db, "begin transaction", NULL, NULL, &err);
  if (r)
    goto error;

  rollback = TRUE;

  if (p->id)
    {
      r =
	sqlite_exec_printf (contacts_db,
			    "delete from contacts where urn='%d'", NULL, NULL,
			    &err, p->id);
      if (r)
	goto error;
    }
  else
    {
      if (contacts_new_person_id (&p->id) == FALSE)
	{
	  sqlite_exec (contacts_db, "rollback transaction", NULL, NULL, NULL);
	  return FALSE;
	}
    }

  for (iter = p->data; iter; iter = iter->next)
    {
      struct contacts_tag_value *v = iter->data;
      if (v->value && v->value[0] && (strcasecmp (v->tag, "MODIFIED")))
        {
          r = sqlite_exec_printf (contacts_db,
                      "insert into contacts values(%d,'%q','%q')",
                      NULL, NULL, &err, p->id, v->tag, v->value);
          if (r)
            goto error;
    
          if (!strcasecmp (v->tag, "NAME"))
            p->name = g_strdup (v->value);
          else if (!strcasecmp (v->tag, "FAMILY_NAME"))
            p->family_name = g_strdup (v->value);
          else if (!strcasecmp (v->tag, "COMPANY"))
            p->company = g_strdup (v->value);
        }
    }

  r = sqlite_exec_printf (contacts_db,
			  "update contacts_urn set name='%q', family_name='%q', company='%q' where (urn=%d)",
			  NULL, NULL, &err, p->name, p->family_name,
			  p->company, p->id);
  if (r)
    goto error;

  if (sqlite_exec_printf (contacts_db,
			  "insert into contacts values(%d, 'MODIFIED', %d)",
			  NULL, NULL, &err, p->id, modified))
    goto error;

  r = sqlite_exec (contacts_db, "commit transaction", NULL, NULL, &err);
  if (r)
    goto error;

  return TRUE;

error:
  if (rollback)
    sqlite_exec (contacts_db, "rollback transaction", NULL, NULL, NULL);
  gpe_error_box (err);
  free (err);
  return FALSE;
}


gint
contacts_db_get_tag_list (gchar *** list)
{
  gchar *err;
  gint r, c;

  sqlite_get_table (contacts_db, "select distinct tag from contacts", list,
		    &r, &c, &err);
  if (err)
    g_free (err);
  return r;
}


gint
contacts_db_get_config_values (gint group, gchar *** list)
{
  gchar *err, *statement;
  gint r, c;

  statement =
    g_strdup_printf
    ("select cidentifier,cvalue from contacts_config where cgroup=%i", group);

  sqlite_get_table (contacts_db, statement, list, &r, &c, &err);
  if (err)
    {
      fprintf (stderr, "e: %s\n", err);
      g_free (err);
    }

  g_free (statement);
  return r;
}

void
contacts_db_add_config_values (gint group, gchar * identifier, gchar * value)
{
  gchar *err;
  gint r, c;
  gchar **list;
  sqlite_get_table_printf (contacts_db,
			   "insert into contacts_config "
			   "(cgroup,cidentifier,cvalue) values(%i,'%q','%q')",
			   &list, &r, &c, &err, group, identifier, value);
  if (err)
    {
      fprintf (stderr, "e: %s\n", err);
      g_free (err);
    }
  if (r)
    {
      sqlite_free_table (list);
    }
}


void
contacts_db_delete_config_values (gint group, gchar * identifier)
{
  gchar *err;
  gint r = 0;
  gint c = 0;
  gchar **list;
  sqlite_get_table_printf (contacts_db,
			   "delete from contacts_config "
			   "where (cgroup=%i) and (cidentifier='%q')",
			   &list, &r, &c, &err, group, identifier);
  if (err)
    {
      fprintf (stderr, "e: %s\n", err);
      g_free (err);
    }
  if (r)
    {
      sqlite_free_table (list);
    }
}

void
contacts_db_update_config_values (gint group, gchar * identifier,
				  gchar * value)
{
  contacts_db_delete_config_values (group, identifier);
  contacts_db_add_config_values (group, identifier, value);
}

gchar *
contacts_db_get_config_tag (gint group, const gchar * tagname)
{
  gchar *err = NULL;
  gchar **list = NULL;
  gint r, c;

  sqlite_get_table_printf (contacts_db,
			   "select cvalue from contacts_config "
			   "where (cgroup=%i) and (cidentifier='%q')",
			   &list, &r, &c, &err, group, tagname);
  if (err)
    {
      fprintf (stderr, "e: %s\n", err);
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
contacts_db_free_result (char **table)
{
  sqlite_free_table (table);
}

gchar *
contacts_db_compress (void)
{
  gchar *err = NULL;
  gchar **list = NULL;
  gint r, c;

  sqlite_get_table_printf (contacts_db, "vacuum", &list, &r, &c, &err);
  if (err)
    {
      fprintf (stderr, "e: %s\n", err);
      return err;
    }
  if (list != NULL)
    {
      sqlite_free_table (list);
    }
  return NULL;
}

gint
contacts_db_size (void)
{
  gchar *fn = g_strdup_printf ("%s/" DB_NAME_CONTACTS, g_get_home_dir ());
  int result = 0;
  struct stat dat;

  if (!stat (fn, &dat))
    result = dat.st_size / 1024;
  g_free (fn);
  return result;
}
