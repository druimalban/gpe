/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <libintl.h>
#include <assert.h>

#include <mimedir/mimedir-vcard.h>
#include <gpe/vcard.h>

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
    { "comment", "note" },
	{ "work_organization", "organization"},
    { NULL, NULL }
  };

static void
set_type (GObject *o, const gchar *type)
{
  assert (!strcasecmp (type, "work") || !strcasecmp (type, "home"));

  g_object_set (o, "work", FALSE, "home", FALSE, NULL);
  g_object_set (o, type, TRUE, NULL);
}

static gboolean
tag_with_type (MIMEDirVCard *card, const char *tag, const char *value, const char *type)
{
  if (!strcasecmp (tag, "address"))
    {
      MIMEDirVCardAddress *a = mimedir_vcard_address_new ();
      set_type (G_OBJECT (a), type);
      g_object_set (G_OBJECT (a), "full", value, NULL);
      mimedir_vcard_append_address (card, a);
      return TRUE;
    }
  else if (!strcasecmp (tag, "telephone"))
    {
      MIMEDirVCardPhone *a = mimedir_vcard_phone_new ();
      set_type (G_OBJECT (a), type);
      g_object_set (G_OBJECT (a), "number", value, NULL);
      mimedir_vcard_append_phone (card, a);
      return TRUE;
    }
  else if (!strcasecmp (tag, "email"))
    {
      MIMEDirVCardEMail *a = mimedir_vcard_email_new ();
      g_object_set (G_OBJECT (a), "address", value, NULL);
      mimedir_vcard_append_email (card, a);
      return TRUE;
    }

  return FALSE;
}

static gboolean
vcard_interpret_tag (MIMEDirVCard *card, const char *tag, const char *value)
{
  struct tag_map *t = &map[0];
  while (t->tag)
    {
      if (!strcasecmp (t->tag, tag))
	{
	  g_object_set (G_OBJECT (card), t->vc ? t->vc : t->tag, value, NULL);
	  return TRUE;
	}
      t++;
    }

  if (!strncasecmp (tag, "home.", 5))
    return tag_with_type (card, tag + 5, value, "home");
  else if (!strncasecmp (tag, "work.", 5))
    return tag_with_type (card, tag + 5, value, "work");

  return FALSE;
}

MIMEDirVCard *
vcard_from_tags (GSList *tags)
{
  MIMEDirVCard *vcard = mimedir_vcard_new ();

  while (tags)
    {
      gpe_tag_pair *p = tags->data;

      vcard_interpret_tag (vcard, p->tag, p->value);

      tags = tags->next;
    }

  return vcard;
}
