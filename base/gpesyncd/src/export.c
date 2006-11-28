/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  Copyright (C) 2006 Graham Cobb <g+gpe@cobb.uk.net>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include "gpesyncd.h"

gchar *
get_contact (gpesyncd_context * ctx, guint uid, GError ** error)
{
  struct contacts_person *p = contacts_db_get_by_uid (uid);
  
  if ((!p) || (!p->data)) {
      g_set_error (error, 0, 111, "No contact data returned");
      return NULL;    
  }

  MIMEDirVCard *vcard = vcard_from_tags (p->data);

  gchar *data = mimedir_vcard_write_to_string (vcard);

  g_object_unref (vcard);
  return data;
}

gchar *
get_event (gpesyncd_context * ctx, guint uid, GError ** error)
{
  Event *ev = event_db_find_by_uid(ctx->event_db, uid);

  if (!ev) {
      g_set_error (error, 0, 102, "No event data returned");
      return NULL;    
  }

  /* Workround bug in event_db_find_by_uid which returns empty events
     when the UID does not exist */
  if (!event_get_start(ev)) {
      g_set_error (error, 0, 102, "No event data returned");
      g_object_unref(ev);
      return NULL;    
  }


  MIMEDirVCal *vcal = mimedir_vcal_new ();

  MIMEDirVEvent *vevent = export_event_as_vevent(ev);
  g_object_unref(ev);

  mimedir_vcal_add_component (vcal, (MIMEDirVComponent *) vevent);

  gchar *data = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);
  g_object_unref (vevent);

  return data;
}

GSList *todo_db_item_to_tags (struct todo_item *t) {
  GSList *tags = NULL, *iter;

  if (t->summary) tags = gpe_tag_list_prepend(tags, "SUMMARY", g_strdup(t->summary));
  if (t->what) tags = gpe_tag_list_prepend(tags, "DESCRIPTION", g_strdup(t->what));
  if (t->todoid) tags = gpe_tag_list_prepend(tags, "TODOID", g_strdup(t->todoid));
  tags = gpe_tag_list_prepend(tags, "STATE", g_strdup_printf("%d", t->state));
  if (t->time)
    {
      char d_buf[32];
      struct tm tm;
      localtime_r (&t->time, &tm);
      strftime (d_buf, sizeof (d_buf), "%F", &tm);
      tags = gpe_tag_list_prepend(tags, "DUE", g_strdup(d_buf));
    }
  for (iter = t->categories; iter; iter = iter->next)
    {
      tags = gpe_tag_list_prepend(tags, "CATEGORY", g_strdup_printf("%d", iter->data));
    }
  tags = gpe_tag_list_prepend(tags, "PRIORITY", g_strdup_printf("%d", t->priority));

  return tags;
}

struct todo_item *todo_db_find_item_by_id(guint uid) {
  GSList *todo_list = todo_db_get_items_list(), *i;

  for (i=todo_list; i; i=g_slist_next(i)) {
    struct todo_item *t = i->data;
    if (t->id == uid) return t;
  }

  return NULL;
}

gchar *
get_todo (gpesyncd_context * ctx, guint uid, GError ** error)
{
  struct todo_item *t = todo_db_find_item_by_id(uid);

  if (!t) {
      g_set_error (error, 0, 103, "No task data returned");
      return NULL;    
  }

  GSList *tags = todo_db_item_to_tags(t);

  MIMEDirVCal *vcal = mimedir_vcal_new ();
  MIMEDirVTodo *vtodo = vtodo_from_tags (tags);

  gpe_tag_list_free(tags);

  mimedir_vcal_add_component (vcal, (MIMEDirVComponent *) vtodo);

  gchar *data = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);
  g_object_unref (vtodo);

  return data;
}

/*! \brief Returns a list of the uids and their modified tags
 *
 * \param ctx	The current context
 * \param error	For error handling
 *
 */
GSList *
get_contact_uid_list (gpesyncd_context * ctx, GError ** error)
{
  GSList *uid_list = NULL, *iter;

  /* At first we need to get the entries: */
  uid_list = contacts_db_get_entries();

  GString *uid_modified = g_string_new ("");

  /* Now we got to get their modified tags. */
  for (iter = uid_list; iter; iter = g_slist_next (iter))
    {
      struct contacts_person *p=iter->data, *fullp;
      struct contacts_tag_value *modified;  

      fullp = contacts_db_get_by_uid(p->id);
     
      modified = contacts_db_find_tag(fullp, "MODIFIED");

      /* if we didn't get one, report it as 0 */
      if (modified)
	g_string_printf (uid_modified, "%d:%s", p->id, modified->value);
      else
	g_string_printf (uid_modified, "%d:0", p->id);

      contacts_discard_person (fullp);
      contacts_discard_person (p);
      iter->data = g_strdup (uid_modified->str);

    }
  g_string_free (uid_modified, TRUE);

  return uid_list;
}

/*! \brief Returns a list of the uids and their modified tags
 *
 * \param ctx	The current context
 * \param error	For error handling
 *
 */
GSList *
get_event_uid_list (gpesyncd_context * ctx, GError ** error)
{
  GSList *uid_list = NULL, *return_list = NULL, *i;
  GString *uid_modified = g_string_new ("");

  /* Load the events from each calendar */
  for (i = ctx->event_calendars; i; i = g_slist_next(i)) {
    GSList *iter, *event_list = event_calendar_list_events(i->data);

    for (iter = event_list; iter; iter = g_slist_next (iter))
      {
	Event *e=iter->data;
	time_t modtime;
	unsigned long uid;

	uid = event_get_uid(e);

	/* Ignore duplicate UIDs */
	if (!g_slist_find(uid_list, GUINT_TO_POINTER(uid))) {

	  /* Remember UID */
	  uid_list = g_slist_append(uid_list, GUINT_TO_POINTER(uid));

	  modtime = event_get_last_modification(e);
	  g_string_printf (uid_modified, "%d:%d", uid, modtime);

	  /* Add to return list */
	  return_list = g_slist_append(return_list, g_strdup (uid_modified->str));
	}

	g_object_unref(e);
      }
    g_slist_free (event_list);
  }
  g_slist_free (uid_list);
  g_string_free (uid_modified, TRUE);

  return return_list;
}

/*! \brief Returns a list of the uids and their modified tags
 *
 * \param ctx	The current context
 * \param error	For error handling
 *
 */
GSList *
get_todo_uid_list (gpesyncd_context * ctx, GError ** error)
{
  GSList *todo_list, *uid_list = NULL, *iter;

  /* Note: the todo list returned by todo_db_get_items_list
     is owned by libtododb and must not be modified.  The items should
     not be freed. */
  todo_list = todo_db_get_items_list();

  GString *uid_modified = g_string_new ("");

  for (iter = todo_list; iter; iter = g_slist_next (iter))
    {
      struct todo_item *t=iter->data;

      /* Todo-db does not expose the MODIFIED value */

      g_string_printf (uid_modified, "%d:0", t->id);

      uid_list = g_slist_append(uid_list, g_strdup (uid_modified->str));
    }

  g_string_free (uid_modified, TRUE);

  return uid_list;
}
