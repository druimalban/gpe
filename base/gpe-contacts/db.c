#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#include <gpe/errorbox.h>

#include "db.h"
#include "structure.h"

#define DB_NAME "/.gpe/contacts"
#define LAYOUT_NAME "/.gpe/contacts-layout.xml"
#define DEFAULT_STRUCTURE PREFIX "/share/gpe-contacts/default-layout.xml"

#undef USE_USQLD

#ifdef USE_USQLD
#include <usql.h>
static usqld_conn *db;
#else
#include <sqlite.h>
static sqlite *db;
#endif

extern GtkWidget *clist;

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
  int ret;
  
#ifdef USE_USQLD
  db = usqld_connect ("localhost", "contacts", &errmsg);
  
  if (db == NULL) 
    {
      gpe_error_box (errmsg);
      free (errmsg);
      return -1;
    }
  
  ret = usqld_exec (db, schema_str, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK)
    {
      printf("err: %s\n",errmsg);
      free(errmsg);
    }
  ret = usqld_exec (db, schema2_str, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK)
    {
      printf("err: %s\n",errmsg);
      free(errmsg);
    }
  ret = usqld_exec (db, schema3_str, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK){
    printf("err: %s\n",errmsg);
    free(errmsg);
  }
#else
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
#endif
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
#ifdef USE_USQLD	
  int r = usqld_exec (db, "insert into contacts_urn values (NULL)",
		       NULL, NULL, &err);
#else	
  int r = sqlite_exec (db, "insert into contacts_urn values (NULL)",
		       NULL, NULL, &err);
#endif
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

#ifdef USE_USQLD	
  *id = usqld_last_insert_rowid (db);
#else
  *id = sqlite_last_insert_rowid (db);
#endif	
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

#ifdef USE_USQLD	
  r = usqld_exec (db, "select urn,value from contacts where tag='NAME'",
		   read_one_entry, &list, &err);
#else
  r = sqlite_exec (db, "select urn,value from contacts where tag='NAME'",
		   read_one_entry, &list, &err);
#endif
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
#ifdef USE_USQLD	
  r = usqld_exec_printf (db, "select tag,value from contacts where urn=%d",
#else
  r = sqlite_exec_printf (db, "select tag,value from contacts where urn=%d",
#endif  
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

#ifdef USE_USQLD	
  r = usqld_exec (db, "begin transaction", NULL, NULL, &err);
#else
  r = sqlite_exec (db, "begin transaction", NULL, NULL, &err);
#endif
  if (r)
    goto error;

  rollback = TRUE;

#ifdef USE_USQLD	
  r = usqld_exec_printf (db, "delete from contacts where urn='%d'",
			  NULL, NULL, &err,
			  uid);
#else
  r = sqlite_exec_printf (db, "delete from contacts where urn='%d'",
			  NULL, NULL, &err,
			  uid);
#endif
  if (r)
    goto error;
  
#ifdef USE_USQLD	
  r = usqld_exec_printf (db, "delete from contacts_urn where urn='%d'",
			  NULL, NULL, &err,
			  uid);
#else
  r = sqlite_exec_printf (db, "delete from contacts_urn where urn='%d'",
			  NULL, NULL, &err,
			  uid);
#endif
  if (r)
    goto error;

#ifdef USE_USQLD	
  r = usqld_exec (db, "commit transaction", NULL, NULL, &err);
#else
  r = sqlite_exec (db, "commit transaction", NULL, NULL, &err);
#endif
  if (r)
    goto error;

  return TRUE;

 error:
  if (rollback)
#ifdef USE_USQLD	
    usqld_exec (db, "rollback transaction", NULL, NULL, NULL);
#else
    sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
#endif  
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
db_set_multi_data (struct person *p, gchar *tag, gchar *value)
{
  struct tag_value *t = new_tag_value (g_strdup (tag), value);
  p->data = g_slist_append (p->data, t);
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

void
db_delete_tag (struct person *p, gchar *tag)
{
  GSList *l;
  
  for (l = p->data; l; )
    {
      GSList *n = l->next;
      struct tag_value *t = l->data;
      if (!strcmp (t->tag, tag))
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

#ifdef USE_USQLD	
  r = usqld_exec (db, "begin transaction", NULL, NULL, &err);
#else
  r = sqlite_exec (db, "begin transaction", NULL, NULL, &err);
#endif	
  if (r)
    goto error;

  rollback = TRUE;

  if (p->id)
    {
#ifdef USE_USQLD		
      r = usqld_exec_printf (db, "delete from contacts where urn='%d'",
			      NULL, NULL, &err,
			      p->id);
#else
      r = sqlite_exec_printf (db, "delete from contacts where urn='%d'",
			      NULL, NULL, &err,
			      p->id);
#endif		
      if (r)
	goto error;
    }
  else
    {
      if (new_person_id (&p->id) == FALSE)
	{
#ifdef USE_USQLD		
	  usqld_exec (db, "rollback transaction", NULL, NULL, NULL);
#else
	  sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
#endif		
	  return FALSE;
	}
    }

  for (iter = p->data; iter; iter = iter->next)
    {
      struct tag_value *v = iter->data;
      if (v->value && v->value[0])
	{
#ifdef USE_USQLD		
	  r = usqld_exec_printf (db,
				  "insert into contacts values(%d,'%q','%q')",
				  NULL, NULL, &err,
				  p->id, v->tag, v->value);
#else
	  r = sqlite_exec_printf (db,
				  "insert into contacts values(%d,'%q','%q')",
				  NULL, NULL, &err,
				  p->id, v->tag, v->value);
#endif		
	}
      if (r)
	goto error;
    }

#ifdef USE_USQLD		
  r = usqld_exec (db, "commit transaction", NULL, NULL, &err);
#else
  r = sqlite_exec (db, "commit transaction", NULL, NULL, &err);
#endif		
  if (r)
    goto error;

  return TRUE;

 error:
  if (rollback)
#ifdef USE_USQLD		
    usqld_exec (db, "rollback transaction", NULL, NULL, NULL);
#else
    sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
#endif		
  gpe_error_box (err);
  free (err);
  return FALSE;
}

gboolean
db_insert_category (gchar *name, guint *id)
{
  char *err;
#ifdef USE_USQLD		
  int r = usqld_exec_printf (db, "insert into contacts_category values (NULL, '%q')", 
			      NULL, NULL, &err, name);
#else
  int r = sqlite_exec_printf (db, "insert into contacts_category values (NULL, '%q')", 
			      NULL, NULL, &err, name);
#endif		
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }
  
#ifdef USE_USQLD 
  *id = usqld_last_insert_rowid (db);
#else
  *id = sqlite_last_insert_rowid (db);
#endif
  return TRUE;
}
 
gboolean
db_delete_category (guint id)
{
  char *err;
#ifdef USE_USQLD		
  int r = usqld_exec_printf (db, "delete from contacts_category where id=%d", 
			      NULL, NULL, &err, id);
#else
  int r = sqlite_exec_printf (db, "delete from contacts_category where id=%d", 
			      NULL, NULL, &err, id);
#endif		
  if (r)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

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
  if (
#ifdef USE_USQLD		
      usqld_exec
#else
      sqlite_exec
#endif		
      (db, "select id,description from contacts_category",
       load_one_attribute, &list, &err))
    {
      fprintf (stderr, "sqlite: %s\n", err);
      free (err);
      return NULL;
    }
  
  return list;
}

GSList *
db_get_entries_alpha (const gchar* alphalist)
{
  GSList *list = NULL;
  char *err;
  char *statement;
  int r;

  statement=malloc(sizeof(gchar)*100);
  for (r=0;r<strlen(alphalist);r++)
    {
      sprintf(statement,"select urn,value from contacts where (tag='NAME') and ((value like \'%c%%\') or (value like \'%c%%\'))",alphalist[r],tolower(alphalist[r]));
#ifdef USE_USQLD		
      usqld_exec (db, statement, read_one_entry, &list, &err);
#else
      sqlite_exec (db, statement, read_one_entry, &list, &err);
#endif		
    }
  free(statement);
  return list;
}


