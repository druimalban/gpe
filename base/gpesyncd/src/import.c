/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include "gpesyncd.h"

static int
fetch_int_callback (void *arg, int argc, char **argv, char **names)
{
  int *value = (int *) arg;
  if (argc == 1)
    {
      *value = atoi (argv[0]);
      return 0;
    }
  return -1;
}

guint
get_new_uid (sqlite * db, gchar * type)
{
  int uid = 0;
  char uid_name[4] = "uid\0";
  if (!strcasecmp (type, "contacts"))
    sprintf (uid_name, "urn");

  sqlite_exec_printf (db, "select max(%q) from %q", fetch_int_callback, &uid,
		      0, uid_name, type);

  return uid + 1;
}

gchar *
get_tag_value (GSList * tags, gchar * tag)
{
  GSList *iter;
  gpe_tag_pair *p;

  for (iter = tags; iter; iter = g_slist_next (iter))
    {
      p = (gpe_tag_pair *) iter->data;
      if (!strcasecmp (p->tag, tag))
	return g_strdup (p->value);
    }
  return NULL;
}

void
set_tag_value (GSList * tags, gchar * tag, gchar * value)
{
  GSList *iter;
  gpe_tag_pair *p;

  for (iter = tags; iter; iter = g_slist_next (iter))
    {
      p = (gpe_tag_pair *) iter->data;
      if (!strcasecmp (p->tag, tag))
	{
	  g_free ((void *) p->value);
	  p->value = g_strdup (value);
	  return;
	}
    }

  /* If we are here, there was nothing modified, so
   * we add the tag */
  p = g_malloc0 (sizeof (gpe_tag_pair));
  p->tag = g_strdup (tag);
  p->value = g_strdup (value);
  tags = g_slist_append (tags, p);
}

gboolean
add_item (gpesyncd_context * ctx, guint uid, gchar * type, gchar * data,
	  GError ** error)
{
  GString *query = g_string_new ("");
  GSList *tags = NULL, *tag_iter;
  GError *convert_error = NULL;
  MIMEDirProfile *profile;
  gchar *errmsg = NULL;
  sqlite *db = NULL;
  gint errorcode = 0;

  profile = mimedir_profile_new (NULL);
  mimedir_profile_parse (profile, data, NULL);

  if (convert_error)
    {
      error = &convert_error;
      return FALSE;
    }

  if (!strcasecmp (type, "contacts"))
    {
      MIMEDirVCard *vcard = NULL;
      vcard = mimedir_vcard_new_from_profile (profile, &convert_error);

      tags = vcard_to_tags (vcard);

      g_object_unref (vcard);

      db = ctx->contact_db;
      errorcode = 211;
    }
  else if (!strcasecmp (type, "calendar"))
    {
      MIMEDirVCal *vcal = NULL;
      GSList *events;

      vcal = mimedir_vcal_new_from_profile (profile, &convert_error);
      if (convert_error)
	{
	  error = &convert_error;
	  return FALSE;
	}

      events = mimedir_vcal_get_event_list (vcal);

      /* we get a whole list of events, but we will process only
       * one. In the future we might support more. (Help is welcome!)
       */
      tags = vevent_to_tags (events->data);
      g_object_unref (events->data);
      g_slist_free (events);

      db = ctx->event_db;
      errorcode = 212;
    }
  else if (!strcasecmp (type, "todo"))
    {
      MIMEDirVCal *vcal = NULL;
      GSList *todos;

      vcal = mimedir_vcal_new_from_profile (profile, &convert_error);
      if (convert_error)
	{
	  error = &convert_error;
	  return FALSE;
	}

      todos = mimedir_vcal_get_todo_list (vcal);

      /* we get a whole list of events, but we will process only
       * one. In the future we might support more. (Help is welcome!)
       */
      tags = vtodo_to_tags ((MIMEDirVTodo *) todos->data);

      g_object_unref (todos->data);
      g_slist_free (todos);
      db = ctx->todo_db;
      errorcode = 213;
    }

  g_object_unref (profile);

  if (uid == 0)
    uid = get_new_uid (db, type);

  GString *modified = g_string_new ("");
  g_string_printf (modified, "%d", (int) time (NULL));
  set_tag_value (tags, "MODIFIED", modified->str);

  for (tag_iter = tags; tag_iter; tag_iter = g_slist_next (tag_iter))
    {
      gpe_tag_pair *p = tag_iter->data;
      sqlite_exec_printf (db, "insert into %q values (%d, upper('%q'), '%q')",
			  NULL, NULL, &errmsg, type, uid, p->tag, p->value);
    }
  if (errmsg)
    {
      g_set_error (error, 0, errorcode, "adding %s with uid %d: %s\n", type,
		   uid, errmsg);
      return FALSE;
    }

  if (!strcasecmp (type, "contacts"))
    {
      sqlite_exec_printf (db,
			  "insert into contacts_urn values ('%d', '%s', '%s', '%s')",
			  NULL, NULL, &errmsg, uid, get_tag_value (tags,
								   "name"),
			  get_tag_value (tags, "family_name"),
			  get_tag_value (tags, "company"));
    }

  else if (!strcasecmp (type, "calendar"))
    {
      sqlite_exec_printf (db, "insert into calendar_urn values (%d)", NULL,
			  NULL, &errmsg, uid);
    }
  else if (!strcasecmp (type, "todo"))
    {
      sqlite_exec_printf (db,
			  "insert into todo_urn values (%d)",
			  NULL, NULL, &errmsg, uid);
    }

  errorcode += 10;
  if (errmsg)
    {
      g_set_error (error, 0, errorcode,
		   "inserting in %s_urn with uid %d: %s\n", type, uid,
		   errmsg);
      return FALSE;
    }

  g_string_free (query, TRUE);
  if (tags)
    gpe_tag_list_free (tags);

  return TRUE;

}


gboolean
del_item (gpesyncd_context * ctx, guint uid, gchar * type, GError ** error)
{
  gchar *errmsg = NULL;
  GString *query = g_string_new ("");
  sqlite *db = NULL;
  gint errorcode = 0;
  if (!strcasecmp (type, "contacts"))
    {
      g_string_printf (query, "delete from contacts where urn='%d'", uid);
      db = ctx->contact_db;
      errorcode = 231;
    }
  else if (!strcasecmp (type, "calendar"))
    {
      g_string_printf (query, "delete from calendar where uid='%d'", uid);
      db = ctx->event_db;
      errorcode = 232;
    }
  else if (!strcasecmp (type, "todo"))
    {
      g_string_printf (query, "delete from todo where uid='%d'", uid);
      db = ctx->todo_db;
      errorcode = 233;
    }
  sqlite_exec_printf (db, query->str, NULL, NULL, &errmsg, uid);
  if (errmsg)
    {
      g_set_error (error, 0, errorcode, "deleting %s with uid %d: %s\n", type,
		   uid, errmsg);
      return FALSE;
    }

  if (!strcasecmp (type, "contacts"))
    g_string_printf (query, "delete from contacts_urn where urn='%d'", uid);
  if (!strcasecmp (type, "calendar"))
    g_string_printf (query, "delete from calendar_urn where uid='%d'", uid);
  if (!strcasecmp (type, "todo"))
    g_string_printf (query, "delete from todo_urn where uid='%d'", uid);

  sqlite_exec_printf (db, query->str, NULL, NULL, &errmsg, uid);

  errorcode += 10;
  if (errmsg)
    {
      g_set_error (error, 0, errorcode, "deleting %s_urn with uid %d: %s\n",
		   type, uid, errmsg);
      return FALSE;
    }

  g_string_free (query, TRUE);

  return TRUE;

}
