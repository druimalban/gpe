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

static sqlite *db;

static const char *schema_str = 
"create table person (
	id		int NOT NULL,
	name		text NOT NULL,
	summary		text,
	birthday	text,
	categories	text,
	notes		text,
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
    initial_structure ();
  else
    rc = read_structure (buf);
  
  g_free (buf);
  return rc;
}
