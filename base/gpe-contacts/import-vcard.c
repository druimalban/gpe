/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Adapted from gpe-bluetooth to gpe-contacts. 
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/contacts-db.h>

#include <mimedir/mimedir-vcard.h>

#include <gpe/vcard.h>

#include <sqlite.h>

#include "import-vcard.h"

#define _(x)  (x)

#define DB_NAME "/.gpe/contacts"

GQuark
gpecontact_impexport_error_quark (void)
{
	static gchar qs[] = "gpecontact-impexport-error-quark";

	return g_quark_from_static_string (qs);
}

static int
do_import_vcard (MIMEDirVCard *card, GError **error)
{
  GSList *tags;

  tags = vcard_to_tags (card);

  /* Set up the person object */
  struct contacts_person *p = contacts_new_person();
  p->data = tags;

  /* Note that contacts_commit_person automatically deletes any
     existing entries with this UID, assigns a new UID, creates
     the MODIFIED tag and fills in the name, etc. from the tags */

  if (!contacts_commit_person(p)) {
    g_set_error(error, GPECONTACT_IMPEXPORT_ERROR, GPECONTACT_IMPEXPORT_ERROR_COMPER, GPECONTACT_IMPEXPORT_ERROR_COMPER_STR);
    contacts_discard_person(p);
    return -2;
  }

  contacts_discard_person(p);

  return 0;
}

int
import_vcard (const gchar *filename, GError **error)
{
  GError *err = NULL;
  GList *cardlist, *l;
  MIMEDirVCard *card = NULL;
  int result = 0;

  if (contacts_db_open(FALSE) < 0) {
    g_set_error(error, GPECONTACT_IMPEXPORT_ERROR, GPECONTACT_IMPEXPORT_ERROR_DBOPEN, GPECONTACT_IMPEXPORT_ERROR_DBOPEN_STR);
    return -3;
  }

  cardlist = mimedir_vcard_read_file (filename, &err);

  if (err) {
    g_propagate_error(error, err);
    return -1;
  }

  for (l = cardlist; l != NULL && result == 0; l = g_list_next (l)) {

    g_assert(l->data != NULL);
    g_assert(MIMEDIR_IS_VCARD (l->data));

    card = l->data;
    result = do_import_vcard (card, error);
  }

  /* Cleanup */

  mimedir_vcard_free_list (cardlist);
  
  return result;

}
