#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <sqlite.h>

#include "errorbox.h"
#include "db.h"

#define DB_NAME "/.gpe/contacts"

sqlite *db;

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
