/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libintl.h>

#include <mimedir/mimedir-vcard.h>

#include <sqlite.h>

#include "vcard.h"

#define _(x) gettext (x)

struct tag_map
{
  gchar *tag;
  gchar *vc;
};

static struct tag_map map[] =
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

static void
set_type (GObject *o, gchar *type)
{
  g_object_set (o, "work", FALSE, "home", FALSE, NULL);
  if (!strcasecmp (type, "work"))
    g_object_set (o, "work", TRUE, NULL);
  else if (!strcasecmp (type, "home"))
    g_object_set (o, "home", TRUE, NULL);
}

static void
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

static int
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

MIMEDirVCard *
gpe_export_vcard (sqlite *db, guint uid)
{
  MIMEDirVCard *vcard = mimedir_vcard_new ();
  char *err;

  if (sqlite_exec_printf (db, "select tag,value from contacts where urn=%d",
			  read_data, vcard, &err, uid))
    {
      fprintf (stderr, "%s\n", err);
      free (err);
      g_object_unref (vcard);
      return NULL;
    }

  return vcard;
}

#define insert(a,b) \
	  if (sqlite_exec_printf (db, "insert into contacts values (%d,'%q','%q')", \
				  NULL, NULL, &err, \
				  id, a, b)) \
	    goto error;


gboolean
gpe_import_vcard (sqlite *db, MIMEDirVCard *vcard)
{
  char *err;
  guint id;
  struct tag_map *t = &map[0];
  GSList *l;

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

  fprintf (stderr, "uid is %d\n", id);

  while (t->tag)
    {
      gchar *value;
      g_object_get (G_OBJECT (vcard), t->vc ? t->vc : t->tag, &value, NULL);
      if (value)
	{
	  if (sqlite_exec_printf (db, "insert into contacts values (%d,'%q','%q')",
				  NULL, NULL, &err,
				  id, t->tag, value))
	    goto error;
	}
      t++;
    }

  l = NULL;
  g_object_get (G_OBJECT (vcard), "address-list", &l, NULL);
  while (l)
    {
      MIMEDirVCardAddress *address;
      gchar *s = NULL;
      gboolean home = FALSE, work = FALSE;
      address = l->data;
      g_object_get (G_OBJECT (address), "full", &s, NULL);
      if (s)
	{
	  g_object_get (G_OBJECT (address), "home", &home, NULL);
	  g_object_get (G_OBJECT (address), "work", &work, NULL);
	  if (home)
	    insert ("home.address", s);
	  if (work)
	    insert ("work.address", s);
	}
      l = g_slist_next (l);
    }

  g_object_get (G_OBJECT (vcard), "email-list", &l, NULL);
  while (l)
    {
      MIMEDirVCardEMail *email;
      l = g_slist_next (l);
    }
  g_object_get (G_OBJECT (vcard), "phone-list", &l, NULL);
  while (l)
    {
      MIMEDirVCardPhone *phone;
      l = g_slist_next (l);
    }

  if (sqlite_exec (db, "commit transaction", NULL, NULL, &err))
    {
      fprintf (stderr, "%s\n", err);
      free (err);
      return FALSE;
    }

  return TRUE;

 error:
  sqlite_exec (db, "rollback transaction", NULL, NULL, NULL);
  fprintf (stderr, "%s\n", err);
  free (err);
  return FALSE;
}
