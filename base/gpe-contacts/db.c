#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sqlite.h>

#include "errorbox.h"
#include "db.h"
#include "structure.h"

#define DB_NAME "/.gpe/contacts"
#define LAYOUT_NAME "/.gpe/contacts-layout.xml"
#define DEFAULT_STRUCTURE "/usr/share/gpe-contacts/default-layout.xml"

static sqlite *db;

static const char *schema_str = 
"create table person (
	id		int NOT NULL,
	name		text NOT NULL,
	PRIMARY KEY(id)
);

create table person_attr (
	person		int NOT NULL,
	attr		int NOT NULL,
	value		text
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
new_tag_value (guint tag, gchar *value)
{
  struct tag_value *t = g_malloc (sizeof (struct tag_value));
  t->tag = tag;
  t->value = value;
  t->oid = 0;
  return t;
}

void
free_tag_values (GSList *list)
{
  GSList *i;
  for (i = list; i; i = i->next)
    {
      struct tag_value *t = i->data;
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

guint
new_person_id ()
{
  return 1;
}

void
commit_person (struct person *p)
{
  GSList *iter;
  int r;
  gchar *err;

  if (p->id)
    {
      r = sqlite_exec_printf (db,
			      "update person set name='%q' where id=%d",
			      NULL,
			      NULL,
			      &err,
			      p->name, p->id);
    }
  else
    {
      p->id = new_person_id ();
      r = sqlite_exec_printf (db,
			      "insert into person values (%d,'%q')",
			      NULL,
			      NULL,
			      &err,
			      p->id, p->name);
    }

  for (iter = p->data; iter; iter = iter->next)
    {
      struct tag_value *v = iter->data;
      if (v->oid)
	{
	  r = sqlite_exec_printf (db,
				  "update person_attr set value='%q' where oid=%d", 
				  NULL, NULL, &err,
				  v->value, v->oid);
	}
      else
	{
	  r = sqlite_exec_printf (db,
				  "insert into person_attr values(%d,%d,'%q')",
				  NULL, NULL, &err,
				  p->id, v->tag, v->value);
	}
    }

  discard_person (p);
}

gboolean
db_insert_category (gchar *name, guint *id)
{
  *id = 1;
  return TRUE;
}

gboolean
db_insert_attribute (guint id)
{
  return TRUE;
}

