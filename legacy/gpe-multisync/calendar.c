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
calendar_get_changes (struct db *db, int newdb)
{
  GList *data = NULL;
  GSList *list, *i;
  
  if (newdb)
    list = fetch_uid_list (db->db, "select distinct uid from calendar_urn");
  else
    list = fetch_uid_list (db->db, "select uid from calendar where (tag='modified' or tag='MODIFIED') and value>%d",
			   db->last_timestamp);

  for (i = list; i; i = i->next)
    {
      GSList *tags;
      MIMEDirVCal *vcal;
      MIMEDirVEvent *vevent;
      gchar *string;
      changed_object *obj;
      int urn = (int)i->data;
      
      tags = fetch_tag_data (db->db, "select tag,value from calendar where uid=%d", urn);
      vevent = vevent_from_tags (tags);
      gpe_tag_list_free (tags);
      vcal = mimedir_vcal_new ();
      mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vevent));
      string = mimedir_vcal_write_to_string (vcal);
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
calendar_push_object (struct db *db, const char *obj, const char *uid, 
		      char *returnuid, int *returnuidlen, GError **err)
{
  GSList *list, *tags;
  MIMEDirVEvent *vevent;
  MIMEDirVCal *vcal = NULL;
  MIMEDirProfile *profile;
  int id;

  profile = mimedir_profile_new(NULL);
  mimedir_profile_parse(profile, obj, err);
  if (*err)
    vcal = mimedir_vcal_new_from_profile (profile, err);
  
  if (vcal == NULL)
    return FALSE;

  list = mimedir_vcal_get_event_list (vcal);
  if (list == NULL)
    {
      g_object_unref (vcal);
      return FALSE;
    }
  
  vevent = MIMEDIR_VEVENT (list->data);

  tags = vevent_to_tags (vevent);

  if (uid)
    id = atoi (uid);
  else
    {
      char *errmsg;

      if (nsqlc_exec (db->db, "insert into calendar_urn values (NULL)",
		      NULL, NULL, &errmsg))
	return FALSE;
      
      id = nsqlc_last_insert_rowid (db->db);
    }
   
  mimedir_vcal_free_component_list (list);
   
  g_object_unref (vcal);
  g_object_unref (profile);

  store_tag_data (db->db, "calendar", id, tags, TRUE);

  sprintf (returnuid, "%d", id);
  *returnuidlen = strlen (returnuid);

  return TRUE;
}

gboolean
calendar_delete_object (struct db *db, const char *uid, gboolean soft)
{
  nsqlc_exec_printf (db->db, "delete from calendar where uid='%q'", NULL, NULL, NULL, uid);
  nsqlc_exec_printf (db->db, "delete from calendar_urn where uid='%q'", NULL, NULL, NULL, uid);

  return TRUE;
}

struct db calendar_db = 
{
  .type = SYNC_OBJECT_TYPE_CALENDAR,
  .name = "calendar",

  .get_changes = calendar_get_changes,
  .push_object = calendar_push_object,
  .delete_object = calendar_delete_object,
};

void
calendar_init (void)
{
  db_list = g_slist_append (db_list, &calendar_db);
}
