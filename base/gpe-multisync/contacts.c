/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include "gpe_sync.h"

#include <gpe/vcard.h>
#include <gpe/tag-db.h>

GList *
sync_contacts (GList *data, gpe_conn *conn, int newdb)
{
  GSList *list, *i;
  
  list = fetch_uid_list (conn->contacts, "select distinct urn from contacts_urn");
  
  for (i = list; i; i = i->next)
    {
      GSList *tags;
      MIMEDirVCard *vcard;
      gchar *string;
      changed_object *obj;
      int urn = (int)i->data;
      
      tags = fetch_tag_data (conn->contacts, "select tag,value from contacts where urn=%d", urn);
      vcard = vcard_from_tags (tags);
      gpe_tag_list_free (tags);
      string = mimedir_vcard_write_to_string (vcard);
      g_object_unref (vcard);

      obj = g_malloc0 (sizeof (*obj));
      obj->comp = string;
      obj->uid = g_strdup_printf ("%d", urn);
      obj->object_type = SYNC_OBJECT_TYPE_PHONEBOOK;
      obj->change_type = SYNC_OBJ_MODIFIED; 

      data = g_list_append (data, obj);
    }

  g_slist_free (list);

  return data;
}

gboolean
push_contact (gpe_conn *conn, const char *obj, const char *uid, 
	      char *returnuid, int *returnuidlen)
{
  return FALSE;
}

gboolean
delete_contact (gpe_conn *conn, const char *uid, gboolean soft)
{
  return FALSE;
}
