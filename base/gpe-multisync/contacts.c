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
contacts_get_changes (struct db *db, int newdb)
{
  GList *data = NULL;
  GSList *list, *i;
  
  if (newdb)
    list = fetch_uid_list (db->db, "select distinct urn from contacts_urn");
  else
    list = fetch_uid_list (db->db, "select urn from contacts where (tag='modified' or tag='MODIFIED') and value>%d",
			   db->last_timestamp);
  
  for (i = list; i; i = i->next)
    {
      GSList *tags;
      MIMEDirVCard *vcard;
      gchar *string;
      changed_object *obj;
      int urn = (int)i->data;
      
      tags = fetch_tag_data (db->db, "select tag,value from contacts where urn=%d", urn);
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
contacts_push_object (struct db *db, const char *obj, const char *uid, 
		      char *returnuid, int *returnuidlen, GError **err)
{
  GSList *tags;
  MIMEDirVCard *vcard;
  int id;

  vcard = mimedir_vcard_new_from_string (obj, err);
  if (vcard == NULL)
    return FALSE;

  tags = vcard_to_tags (vcard);

  if (uid)
    id = atoi (uid);
  else
    {
      char *errmsg;

      if (nsqlc_exec (db->db, "insert into contacts_urn values (NULL)",
		      NULL, NULL, &errmsg))
	return FALSE;
      
      id = nsqlc_last_insert_rowid (db->db);
    }
    
  store_tag_data (db->db, "contacts", id, tags, TRUE);

  sprintf (returnuid, "%d", id);
  *returnuidlen = strlen (returnuid);

  return TRUE;
}

gboolean
contacts_delete_object (struct db *db, const char *uid, gboolean soft)
{
  nsqlc_exec_printf (db->db, "delete from contacts where urn='%q'", NULL, NULL, NULL, uid);
  nsqlc_exec_printf (db->db, "delete from contacts_urn where urn='%q'", NULL, NULL, NULL, uid);

  return TRUE;
}

struct db contacts_db = 
{
  .type = SYNC_OBJECT_TYPE_PHONEBOOK,
  .name = "contacts",

  .get_changes = contacts_get_changes,
  .push_object = contacts_push_object,
  .delete_object = contacts_delete_object,
};

void
contacts_init (void)
{
  db_list = g_slist_append (db_list, &contacts_db);
}
