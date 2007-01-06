#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libintl.h>

#include <mimedir/mimedir-vcard.h>

#include <sqlite.h>

#include <gpe/tag-db.h>
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

gboolean
gpe_import_vcard (sqlite *db, MIMEDirVCard *card)
{
  GSList *tags, *iter;
  gchar *err = NULL;
  guint id;

  if (sqlite_exec (db, "begin transaction", NULL, NULL, &err))
    {
      fprintf (stderr, "%s\n", err);
      free (err);
      return FALSE;
    }

  if (sqlite_exec (db, "insert into contacts_urn values (NULL)",
		   NULL, NULL, &err))
    goto error;

  id = sqlite_last_insert_rowid (db);

  tags = vcard_to_tags (card);

  for (iter = tags; iter; iter = iter->next)
    {
      gpe_tag_pair *t = iter->data;
      if (sqlite_exec_printf (db, "insert into contacts values ('%d', '%q', '%q')", NULL, NULL, &err, id, t->tag, t->value))
        {
          gpe_tag_list_free (tags);
          goto error;
        }
    }

  if (sqlite_exec (db, "commit transaction", NULL, NULL, &err))
    {
      fprintf (stderr, "%s\n", err);
      free (err);
      gpe_tag_list_free (tags);
      return FALSE;
    }
	
  gpe_tag_list_free (tags);

  return TRUE;

 error:
  sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
  fprintf (stderr, "%s\n", err);
  free (err);
  return FALSE;
}

int
main (int argc, char *argv[])
{
  MIMEDirVCard *vcard;
  gchar *str;
  GError *err = NULL;
  GIOChannel *channel;

  g_type_init ();

  if (db_open ())
    exit (1);

  channel = g_io_channel_unix_new (0);

  vcard = mimedir_vcard_new_from_channel (channel, &err);
  if (vcard == NULL)
    {
      fprintf (stderr, "%s\n", err->message);
      g_clear_error (&err);
      exit (1);
    }

  str = mimedir_vcard_get_as_string (vcard);
  puts (str);

  if (gpe_import_vcard (db, vcard) == FALSE)
    exit (1);

  exit (0);
}
