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

#include <gpe/vevent.h>
#include <gpe/tag-db.h>

#include <mimedir/mimedir-vcal.h>

GList *
sync_calendar (GList *data, gpe_conn *conn, int newdb)
{
  GSList *list, *i;
  
  list = fetch_uid_list (conn->calendar, "select distinct uid from calendar_urn");

  for (i = list; i; i = i->next)
    {
      GSList *tags;
      MIMEDirVCal *vcal;
      MIMEDirVEvent *vevent;
      gchar *string;
      changed_object *obj;
      int urn = (int)i->data;
      
      tags = fetch_tag_data (conn->calendar, "select tag,value from calendar where uid=%d", urn);
      vevent = vevent_from_tags (tags);
      gpe_tag_list_free (tags);
      vcal = mimedir_vcal_new ();
      mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vevent));
      string = mimedir_vcal_write_to_string (vcal);
      g_object_unref (vevent);
      g_object_unref (vcal);

      obj = g_malloc0 (sizeof (*obj));
      obj->comp = string;
      obj->uid = g_strdup_printf ("%d", urn);
      obj->object_type = SYNC_OBJECT_TYPE_CALENDAR;
      obj->change_type = SYNC_OBJ_MODIFIED; 

      data = g_list_append (data, obj);
    }

  g_slist_free (list);

  return data;
}

gboolean
push_calendar (gpe_conn *conn, const char *obj, const char *uid, 
	       char *returnuid, int *returnuidlen, GError **err)
{
  return FALSE;
}

gboolean
delete_calendar (gpe_conn *conn, const char *uid, gboolean soft)
{
  return FALSE;
}
