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

GList *
sync_todo (GList *data, gpe_conn *conn, int newdb)
{
  GSList *list, *i;
  
  list = fetch_uid_list (conn->todo, "select distinct uid from todo_urn");
  
  for (i = list; i; i = i->next)
    {
      GSList *tags;
      MIMEDirVTodo *vtodo;
      gchar *string;
      changed_object *obj;
      int urn = (int)i->data;
      
      tags = fetch_tag_data (conn->todo, "select tag,value from todo where uid=%d", urn);
      vtodo = vtodo_from_tags (tags);
      gpe_tag_list_free (tags);
      string = mimedir_vtodo_write_to_string (vtodo);
      g_object_unref (vtodo);

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
push_todo (gpe_conn *conn, const char *obj, const char *uid, 
	   char *returnuid, int *returnuidlen, GError **err)
{
  return FALSE;
}

gboolean
delete_todo (gpe_conn *conn, const char *uid, gboolean soft)
{
  return FALSE;
}
