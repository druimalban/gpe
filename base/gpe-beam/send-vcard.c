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
  GError *err = NULL;
  GIOChannel *channel;
  FILE *fp;
  char line[256];

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

  fp = tmpfile ();
  channel = g_io_channel_unix_new (fileno (fp));
  if (mimedir_vcard_write_to_channel (vcard, channel, &err) == FALSE)
    {
      fprintf (stderr, "%s\n", err->message);
      g_clear_error (&err);
    }

  rewind (fp);
  while (fgets (line, sizeof (line), fp))
    {
      puts (line);
    }
  fclose (fp);

  exit (0);
}
