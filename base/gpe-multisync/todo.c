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
todo_get_changes (struct db *db, GList *data, int newdb)
{
  GSList *list, *i;
  
  list = fetch_uid_list (db->db, "select distinct uid from todo_urn");
  
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
      g_object_unref (vtodo);
      g_object_unref (vcal);

      obj = g_malloc0 (sizeof (*obj));
      obj->comp = string;
      obj->uid = g_strdup_printf ("%d", urn);
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
  return FALSE;
}

gboolean
todo_delete_object (struct db *db, const char *uid, gboolean soft)
{
  return FALSE;
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
todo_init (gpe_conn *conn)
{
  conn->db_list = g_slist_append (conn->db_list, &todo_db);
}
