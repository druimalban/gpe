#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include <sqlite.h>

#include "errorbox.h"
#include "db.h"

#define DB_NAME "/.gpe/contacts"

static sqlite *db;

static const char *schema = 
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
	
create table attr_types (
	id		int NOT NULL,
	type		int NOT NULL,
	name		text NOT NULL,
	PRIMARY KEY(id)
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

    return 0;
}

void
load_structure (void)
{
  initial_structure ();
}
