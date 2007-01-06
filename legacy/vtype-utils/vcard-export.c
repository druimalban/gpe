#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libintl.h>

#include <mimedir/mimedir-vcard.h>

#include <sqlite.h>

#include <gpe/vcard.h>

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
read_tags (void *data, int argc, char *argv[], char *ignore[])
{
  GSList **tags = (GSList **)data;

  *tags = gpe_tag_list_prepend (*tags, argv[0], g_strdup (argv[1]));

  return 0;
}

void
export_one_vcard (int uid)
{
  MIMEDirVCard *vcard;
  gchar *str;
  GSList *tags = NULL;

  sqlite_exec_printf (db, "select tag,value from contacts where urn=%d", read_tags, &tags, NULL,
		      uid);  

  vcard = vcard_from_tags (tags);

  if (vcard)
    {
      str = mimedir_vcard_write_to_string (vcard);

      fprintf (stdout, "%s\n", str);
    }
  else
    fprintf (stderr, "Cannot generate vCard\n");
}

int
read_one (void *ignore, int argc, char *argv[], void *data)
{
  int uid = atoi (argv[0]);
  export_one_vcard (uid);
  return 0;
}

int
main (int argc, char *argv[])
{
  g_type_init ();

  if (db_open ())
    exit (1);

  if (argc > 1)
    {
      guint uid;
      uid = atoi (argv[1]);
      export_one_vcard (uid);
    }
  else
    sqlite_exec (db, "select urn from contacts_urn", (sqlite_callback)read_one, NULL, NULL);

  exit (0);
}
