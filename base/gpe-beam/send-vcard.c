#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libintl.h>

#include <mimedir/mimedir-vcard.h>

#include <sqlite.h>

#include "vcard.h"

#define _(x) gettext (x)

#define DB_NAME "/.gpe/contacts"

sqlite *db;

int 
db_open(void) 
{
  /* open persistent connection */
  char *errmsg;
  char *buf;
  size_t len;
  const char *home = g_get_home_dir ();
  
  len = strlen (home) + strlen (DB_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, DB_NAME);
  
  db = sqlite_open (buf, 0, &errmsg);
  g_free (buf);
  
  if (db == NULL) 
    {
      fprintf (stderr, "%s", errmsg);
      free (errmsg);
      return -1;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  guint uid;
  MIMEDirVCard *vcard;
  gchar *str;

  g_type_init ();

  if (argc < 2)
    {
      fprintf (stderr, "usage: %s uid\n", argv[0]);
      exit (1);
    }

  uid = atoi (argv[1]);

  if (db_open ())
    exit (1);

  vcard = gpe_export_vcard (db, uid);

  str = mimedir_vcard_get_as_string (vcard);
  printf ("%s\n", str);

  exit (0);
}
