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
  GSList *list, *first;
  
  list = fetch_uid_list (conn->contacts, "select distinct urn from contacts_urn");

  first = list;

  while (list)
    {
      int urn = (int)list->data;
      GSList *tags, *first;
      MIMEDirVCard *vcard;
      gchar *string;
      changed_object *obj;
      
      tags = fetch_tag_data (conn->contacts, "select tag,value from contacts where urn=%d", urn);

      vcard = vcard_from_tags (tags);
      string = mimedir_vcard_write_to_string (vcard);

      obj = g_malloc0 (sizeof (*obj));
      obj->comp = string;
      obj->uid = g_strdup_printf ("%d", urn);
      obj->object_type = SYNC_OBJECT_TYPE_PHONEBOOK;

      data = g_list_append (data, obj);
 
      first = tags;
      while (tags)
	{
	  gpe_tag_pair *p = tags->data;

	  g_free ((void *)p->tag);
	  g_free ((void *)p->value);
	  g_free (p);

	  tags = tags->next;
	}

      g_slist_free (first);

      list = list->next;
    }

  g_slist_free (first);

  return data;
}
