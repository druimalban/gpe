#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sqlite.h>

#include <gpe/errorbox.h>

#include "db.h"
#include "structure.h"

#define DB_NAME "/.gpe/contacts"
#define LAYOUT_NAME "/.gpe/contacts-layout.xml"
#define DEFAULT_STRUCTURE PREFIX "/share/gpe-contacts/default-layout.xml"

static sqlite *db;

static const char *schema_str = 
"create table contacts (
	urn		INTEGER NOT NULL,
	tag		TEXT NOT NULL,
	value		TEXT NOT NULL
);
";

static const char *schema2_str = 
"create table contacts_urn (
        urn             INTEGER PRIMARY KEY
);
";

static const char *schema3_str =
"create table contacts_category (
	id		INTEGER PRIMARY KEY,
	description	TEXT
);
";

int 
db_open(void) 
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
  sqlite_exec (db, schema3_str, NULL, NULL, NULL);

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
    rc = read_structure (DEFAULT_STRUCTURE);
  else
    rc = read_structure (buf);
  
  g_free (buf);
  return rc;
}

struct tag_value *
new_tag_value (gchar *tag, gchar *value)
{
  struct tag_value *t = g_malloc (sizeof (struct tag_value));
  t->tag = tag;
  t->value = value;
  return t;
}

void
free_tag_values (GSList *list)
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
update_tag_value (struct tag_value *t, gchar *value)
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
  g_free (p);
}

gboolean
new_person_id (guint *id)
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
sort_entries (struct person *a, struct person *b)
{
  return strcoll (a->name, b->name);
}

static int
read_one_entry (void *arg, int argc, char **argv, char **names)
{
  struct person *p = new_person ();
  GSList **list = (GSList **)arg;

  p->id = atoi (argv[0]);
  p->name = g_strdup (argv[1]);

  *list = g_slist_insert_sorted (*list, p, (GCompareFunc)sort_entries);

  return 0;
}

GSList *
db_get_entries (void)
{
  GSList *list = NULL;
  char *err;
  int r;

  r = sqlite_exec (db, "select urn,value from contacts where tag='NAME'",
		   read_one_entry, &list, &err);

  return list;
}

static int
read_entry_data (void *arg, int argc, char **argv, char **names)
{
  struct person *p = arg;
  db_set_data (p, argv[0], g_strdup (argv[1]));
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
			  NULL, NULL, &err,
			  uid);
  if (r)
    goto error;
  
  r = sqlite_exec_printf (db, "delete from contacts_urn where urn='%d'",
			  NULL, NULL, &err,
			  uid);
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
db_find_tag (struct person *p, gchar *tag)
{
  struct tag_value *t = NULL;
  GSList *iter;
  for (iter = p->data; iter; iter = iter->next)
    {
      struct tag_value *id = (struct tag_value *)iter->data;
      if (!strcmp (id->tag, tag))
	{
	  t = id;
	  break;
	}
    }
  return t;
}

void
db_set_data (struct person *p, gchar *tag, gchar *value)
{
  struct tag_value *t = db_find_tag (p, tag);

  if (t)
    {
      update_tag_value (t, value);
    }
  else
    {
      t = new_tag_value (g_strdup (tag), value);
      p->data = g_slist_append (p->data, t);
    }
}

gboolean
commit_person (struct person *p)
{
  GSList *iter;
  int r;
  gchar *err;
  gboolean rollback = FALSE;

  r = sqlite_exec (db, "begin transaction", NULL, NULL, &err);
  if (r)
    goto error;

  rollback = TRUE;

  if (p->id)
    {
      r = sqlite_exec_printf (db, "delete from contacts where urn='%d'",
			      NULL, NULL, &err,
			      p->id);
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
      if (v->value && v->value[0])
	{
	  r = sqlite_exec_printf (db,
				  "insert into contacts values(%d,'%q','%q')",
				  NULL, NULL, &err,
				  p->id, v->tag, v->value);
	}
      if (r)
	goto error;
    }

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
db_insert_category (gchar *name, guint *id)
{
  char *err;
  int r = sqlite_exec_printf (db, "insert into contacts_category values (NULL, '%q')", 
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

static int
load_one_attribute (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      GSList **list = (GSList **)arg;
      struct category *c = g_malloc (sizeof (struct category));

      c->id = atoi (argv[0]);
      c->name = g_strdup (argv[1]);

      *list = g_slist_append (*list, c);
    }

  return 0;
}

GSList *
db_get_categories (void)
{
  GSList *list = NULL;
  char *err;
  if (sqlite_exec (db,
		   "select id,description from contacts_category",
		   load_one_attribute, &list, &err))
  {
    fprintf (stderr, "sqlite: %s\n", err);
    free (err);
    return NULL;
  }
		
  return list;
}
