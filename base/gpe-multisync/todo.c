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

#include <gpe/vtodo.h>
#include <gpe/tag-db.h>

#include <mimedir/mimedir-vcal.h>

GList *
todo_get_changes (struct db *db, int newdb)
{
  GList *data = NULL;
  GSList *list, *i;
  
  if (newdb)
    list = fetch_uid_list (db->db, "select distinct uid from todo_urn");
  else
    list = fetch_uid_list (db->db, "select uid from todo where (tag='modified' or tag='MODIFIED') and value>%d",
			   db->last_timestamp);
  
  for (i = list; i; i = i->next)
    {
      GSList *tags;
      MIMEDirVCal *vcal;
      MIMEDirVTodo *vtodo;
      gchar *string;
      changed_object *obj;
      int urn = (int)i->data;
      
      tags = fetch_tag_data (db->db, "select tag,value from todo where uid=%d", urn);
      vtodo = vtodo_from_tags (tags);
      gpe_tag_list_free (tags);
      vcal = mimedir_vcal_new ();
      mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vtodo));
      string = mimedir_vcal_write_to_string (vcal);
      g_object_unref (vcal);

      obj = g_malloc0 (sizeof (*obj));
      obj->comp = string;
      obj->uid = g_strdup_printf ("todo-%d", urn);
      obj->object_type = SYNC_OBJECT_TYPE_TODO;
      obj->change_type = SYNC_OBJ_MODIFIED; 

      data = g_list_append (data, obj);
    }

  g_slist_free (list);

  return data;
}

gboolean
todo_push_object (struct db *db, const char *obj, const char *uid, 
		  char *returnuid, int *returnuidlen, GError **err)
{
  GSList *list, *tags;
  MIMEDirVTodo *vtodo;
  MIMEDirVCal *vcal;
  int id;

  vcal = mimedir_vcal_new_from_string (obj, err);
  if (vcal == NULL)
    return FALSE;

  list = mimedir_vcal_get_todo_list (vcal);
  if (list == NULL)
    {
      g_object_unref (vcal);
      return FALSE;
    }

  vtodo = MIMEDIR_VTODO (list->data);

  tags = vtodo_to_tags (vtodo);

  if (uid)
    sscanf (uid, "todo-%d", &id);
  else
    {
      char *errmsg;

      if (nsqlc_exec (db->db, "insert into todo_urn values (NULL)",
		      NULL, NULL, &errmsg))
	return FALSE;
      
      id = nsqlc_last_insert_rowid (db->db);
    }
   
  mimedir_vcal_free_component_list (list);
   
  g_object_unref (vcal);

  nsqlc_exec_printf (db->db, "delete from todo where uid='%q'", NULL, NULL, NULL, uid);
  store_tag_data (db->db, "todo", id, tags, FALSE);

  sprintf (returnuid, "%d", id);
  *returnuidlen = strlen (returnuid);

  return TRUE;
}

gboolean
todo_delete_object (struct db *db, const char *uid, gboolean soft)
{
  nsqlc_exec_printf (db->db, "delete from todo where uid='%q'", NULL, NULL, NULL, uid);
  nsqlc_exec_printf (db->db, "delete from todo_urn where uid='%q'", NULL, NULL, NULL, uid);

  return TRUE;
}

struct db todo_db = 
{
  .type = SYNC_OBJECT_TYPE_TODO,
  .name = "todo",

  .get_changes = todo_get_changes,
  .push_object = todo_push_object,
  .delete_object = todo_delete_object,
};

void
todo_init (void)
{
  db_list = g_slist_append (db_list, &todo_db);
}
