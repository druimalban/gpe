/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <openobex/obex.h>

#include <mimedir/mimedir-vcard.h>

#include <gpe/vcard.h>

#include <sqlite.h>

#define _(x)  (x)

#define DB_NAME "/.gpe/contacts"

static void
do_import_vcard (MIMEDirVCard *card)
{
  sqlite *db;
  GSList *tags, *i;
  char *buf;
  const gchar *home;
  char *err = NULL;
  int id;

  home = g_get_home_dir ();
  
  buf = g_strdup_printf ("%s%s", home, DB_NAME);

  db = sqlite_open (buf, 0, &err);
  g_free (buf);

  if (db == NULL)
    {
      gpe_error_box (err);
      free (err);
      return;
    }
 
  if (sqlite_exec (db, "insert into contacts_urn values (NULL)", NULL, NULL, &err) != SQLITE_OK)
    {
      gpe_error_box (err);
      free (err);
      sqlite_close (db);
      return;
    }

  id = sqlite_last_insert_rowid (db);

  tags = vcard_to_tags (card);

  for (i = tags; i; i = i->next)
    {
      gpe_tag_pair *t = i->data;

      sqlite_exec_printf (db, "insert into contacts values ('%d', '%q', '%q')", NULL, NULL, NULL,
			  id, t->tag, t->value);
    }
  
  gpe_tag_list_free (tags);

  sqlite_close (db);
}

void
import_vcard (const gchar *data, size_t len)
{
  MIMEDirVCard *card;
  gchar *str;
  GError *error = NULL;

  str = g_malloc (len + 1);
  memcpy (str, data, len);
  str[len] = 0;

  card = mimedir_vcard_new_from_string (str, &error);
 
  g_free (str);

  if (card)
    {
      gchar *name;
      gchar *query;

      g_object_get (G_OBJECT (card), "name", &name, NULL);

      query = g_strdup_printf (_("Received a business card for %s.  Import it?"), name);

      if (gpe_question_ask (query, NULL, "bt-logo", "!gtk-cancel", NULL, "!gtk-ok", NULL, NULL))
	do_import_vcard (card);

      g_object_unref (card);
    }
}

