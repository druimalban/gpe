#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libintl.h>

#include <mimedir/mimedir-vcard.h>

#include <sqlite.h>

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
  char *home = g_get_home_dir ();
  
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

struct tag_map
{
  gchar *tag;
  gchar *vc;
};

struct tag_map map[] =
  {
    { "name", NULL },
    { "given_name", "givenname" },
    { "family_name", "familyname" },
    { "title", "prefix" },
    { "honorific_suffix", "suffix" },
    { "nickname", NULL },
    { "birthday", NULL },
    { "comment", "note" },
    { NULL, NULL }
  };

void
set_type (GObject *o, gchar *type)
{
  g_object_set (o, "work", FALSE, "home", FALSE, NULL);
  if (!strcasecmp (type, "work"))
    g_object_set (o, "work", TRUE, NULL);
  else if (!strcasecmp (type, "home"))
    g_object_set (o, "home", TRUE, NULL);
}

void
tag_with_type (void *arg, char *tag, char *value, char *type)
{
  if (!strcasecmp (tag, "address"))
    {
      MIMEDirVCardAddress *a = mimedir_vcard_address_new ();
      set_type (G_OBJECT (a), type);
      g_object_set (G_OBJECT (a), "full", value, NULL);
      mimedir_vcard_append_address ((MIMEDirVCard *)arg, a);
    }
  else if (!strcasecmp (tag, "telephone"))
    {
      MIMEDirVCardPhone *a = mimedir_vcard_phone_new ();
      set_type (G_OBJECT (a), type);
      g_object_set (G_OBJECT (a), "number", value, NULL);
      mimedir_vcard_append_phone ((MIMEDirVCard *)arg, a);
    }
  else if (!strcasecmp (tag, "email"))
    {
      MIMEDirVCardEMail *a = mimedir_vcard_email_new ();
      g_object_set (G_OBJECT (a), "address", value, NULL);
      mimedir_vcard_append_email ((MIMEDirVCard *)arg, a);
    }
}

int
read_data (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      char *tag = argv[0];
      char *value = argv[1];
      struct tag_map *t = &map[0];
      while (t->tag)
	{
	  if (!strcasecmp (t->tag, tag))
	    {
	      g_object_set (G_OBJECT (arg), t->vc ? t->vc : t->tag, value, NULL);
	      return 0;
	    }
	  t++;
	}

      if (!strncasecmp (tag, "home.", 5))
	tag_with_type (arg, tag + 5, value, "home");
      else if (!strncasecmp (tag, "work.", 5))
	tag_with_type (arg, tag + 5, value, "work");
    }
  return 0;
}

int
main (int argc, char *argv[])
{
  guint uid;
  char *err;
  MIMEDirVCard *vcard;
  gchar *str;

  g_type_init ();

  if (argc < 2)
    {
      fprintf (stderr, "usage: %s uid\n", argv[0]);
      exit (1);
    }

  uid = atoi (argv[1]);
  vcard = mimedir_vcard_new ();

  if (db_open ())
    exit (1);

  if (sqlite_exec_printf (db, "select tag,value from contacts where urn=%d",
			  read_data, vcard, &err, uid))
    {
      fprintf (stderr, "%s\n", err);
      free (err);
      exit (1);
    }

  str = mimedir_vcard_get_as_string (vcard);
  printf ("%s\n", str);

  exit (0);
}
