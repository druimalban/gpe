/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  Copyright (C) 2006, 2007 Graham Cobb <g+gpe@cobb.uk.net>
 *  Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

/* We need to define __USE_XOPEN to get strptime defined.
   Maybe there is a better way to do this from configure? */
#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <time.h>
#include "gpesyncd.h"

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
add_contact (gpesyncd_context * ctx, guint *uid, gchar * data,
	  guint * modified, GError ** error)
{
  GSList *tags = NULL;
  GError *convert_error = NULL;

  /* Parse the VCARD */
  MIMEDirProfile *profile = mimedir_profile_new (NULL);
  mimedir_profile_parse (profile, data, &convert_error);
  if (convert_error)
    {
      *error = convert_error;
      return FALSE;
    }

  /* Get the VCARD */
  MIMEDirVCard *vcard = mimedir_vcard_new_from_profile (profile, &convert_error);
  g_object_unref (profile);
  if (convert_error)
    {
      *error = convert_error;
      return FALSE;
    }

  /* Convert the VCARD to tags */
  tags = vcard_to_tags (vcard);
  g_object_unref (vcard);

  /* Set up the person object */
  struct contacts_person *p = contacts_new_person();
  p->id = *uid;
  p->data = tags;

  /* Note that contacts_commit_person automatically deletes any
     existing entries with this UID, assigns a new UID, creates
     the MODIFIED tag and fills in the name, etc. from the tags */

  if (!contacts_commit_person(p)) {
    g_set_error(error, 0, 211, "Could not commit person");
    contacts_discard_person(p);
    return FALSE;
  }

  *uid = p->id;

  /* return the new modified value */

  *modified = 0;

  struct contacts_person *newp = contacts_db_get_by_uid(p->id);
  struct contacts_tag_value *mod_tag = contacts_db_find_tag(newp, "MODIFIED");
  if (mod_tag) *modified = atoi(mod_tag->value);
  contacts_discard_person (newp);

  contacts_discard_person(p);

  return TRUE;
}

gboolean
add_event (gpesyncd_context * ctx, guint *uid, gchar * data,
	  guint * modified, GError ** error)
{
  GError *convert_error = NULL;
  Event *ev;

  /* Parse the VCALENDAR */
  MIMEDirProfile *profile = mimedir_profile_new (NULL);
  mimedir_profile_parse (profile, data, &convert_error);
  if (convert_error)
    {
      g_propagate_error (error, convert_error);
      return FALSE;
    }

  /* Get the VCAL */
  MIMEDirVCal *vcal = mimedir_vcal_new_from_profile (profile, &convert_error);
  g_object_unref (profile);
  if (convert_error)
    {
      g_propagate_error (error, convert_error);
      return FALSE;
    }

  /* Get the list of events in the calendar
     Note: we only process the first one -- any others are ignored */
  GSList *events = mimedir_vcal_get_event_list (vcal);
  MIMEDirVEvent *vevent = events->data;

  /* If this is a modify, rather than an add,
     we need to set the VEVENT UID so it will overwrite the existing event */
  if ( (*uid != 0)
      && (ev = event_db_find_by_uid(ctx->event_db, *uid, NULL)) ) {
    char *evuid = event_get_eventid (ev, NULL);
    g_object_set (vevent, "uid", evuid, NULL);
    g_free (evuid);
    g_object_unref(ev);
  }

  int res = event_import_from_vevent (ctx->import_calendar, vevent, &ev, error);

  g_object_unref (vevent);
  g_slist_free (events);
  g_object_unref (vcal);

  if (! res)
    return FALSE;

  /* Get the UID and the modification time */
  *modified = event_get_last_modification(ev);
  *uid = event_get_uid(ev);

  g_object_unref (ev);

  return TRUE;
}

gboolean todo_db_update_item_from_tags (struct todo_item *t, GSList *tags) {
  /* This is mostly taken from item_callback and item_data_callback in todo-db.c */

  t->priority = PRIORITY_STANDARD;

  /* Clear the category list */
  g_slist_free(t->categories);
  t->categories = NULL;

  while (tags) {
    gpe_tag_pair *p = tags->data;

    if (!strcasecmp(p->tag, "SUMMARY"))
      t->summary = g_strdup(p->value);
    else if (!strcasecmp(p->tag, "DESCRIPTION"))
      t->what = g_strdup(p->value);
    else if (!strcasecmp(p->tag, "TODOID"))
      t->todoid = g_strdup(p->value);
    else if (!strcasecmp(p->tag, "STATE"))
      t->state = atoi(p->value);
    else if (!strcasecmp(p->tag, "PRIORITY"))
      t->priority = atoi(p->value);
    else if (!strcasecmp(p->tag, "CATEGORY"))
      t->categories = g_slist_prepend(t->categories, (gpointer) atoi(p->value));
    else if (!strcasecmp (p->tag, "DUE"))
      {
	struct tm tm;
	memset (&tm, 0, sizeof (tm));
	if (strptime (p->value, "%F", &tm))
	  t->time = mktime (&tm);
      }

    tags = tags->next;
  }

  if (t->state == COMPLETED)
    t->was_complete = TRUE;

  return TRUE;

}

gboolean
add_todo (gpesyncd_context * ctx, guint *uid, gchar * data,
	  guint * modified, GError ** error)
{
  GSList *tags = NULL;
  GError *convert_error = NULL;
  gboolean created;

  /* Parse the VCAL */
  MIMEDirProfile *profile = mimedir_profile_new (NULL);
  mimedir_profile_parse (profile, data, &convert_error);
  if (convert_error)
    {
      *error = convert_error;
      return FALSE;
    }

  /* Get the VCAL */
  MIMEDirVCal *vcal = mimedir_vcal_new_from_profile (profile, &convert_error);
  g_object_unref (profile);
  if (convert_error)
    {
      *error = convert_error;
      return FALSE;
    }

  /* Get the list of todos in the calendar
     Note: we only process the first one -- any others are ignored */
  GSList *todos = mimedir_vcal_get_todo_list (vcal);
  MIMEDirVTodo *vtodo = todos->data;

  /* Convert the VTODO to tags */
  tags = vtodo_to_tags (vtodo);
  g_object_unref (vtodo);
  g_slist_free (todos);
  g_object_unref (vcal);

  /* Does the item already exist? */
  struct todo_item *t = todo_db_find_item_by_id(*uid);

  if (!t) {
    /* Create new item */
    created = TRUE;
    t = todo_db_new_item();
  } else created = FALSE;

  if (!t) {
    g_set_error(error, 0, 213, "Could not create todo item");
    gpe_tag_list_free(tags);
    return FALSE;
  }

  gboolean status = todo_db_update_item_from_tags (t, tags);
  gpe_tag_list_free(tags);
  if (!status) {
    g_set_error(error, 0, 213, "Could not complete todo item");
    if (created) todo_db_delete_item(t);
    return FALSE;
  }

  /* Send to database */
  if (!todo_db_push_item(t)) {
    g_set_error(error, 0, 213, "Could not write todo item");
    /* Re-store data from disk */
    todo_db_refresh();
    return FALSE;
  }

  *uid = t->id;

  *modified = time(NULL);

  return TRUE;
}

gboolean
del_contact (gpesyncd_context * ctx, guint uid, GError ** error)
{
  if (!contacts_db_delete_by_uid(uid)) {
    g_set_error (error, 0, 231, "No item found\n");
    return FALSE;
  }
  return TRUE;
}

gboolean
del_event (gpesyncd_context * ctx, guint uid, GError ** error)
{
  Event *ev = event_db_find_by_uid(ctx->event_db, uid, error);
  if (!ev) {
    return FALSE;
  }
  if (!event_remove(ev, error)) {
    return FALSE;
  }

  g_object_unref(ev);

  return TRUE;
}

gboolean
del_todo (gpesyncd_context * ctx, guint uid, GError ** error)
{
  struct todo_item *t = todo_db_find_item_by_id(uid);

  if (!t) {
    g_set_error (error, 0, 233, "No item found\n");
    return FALSE;
  }

  todo_db_delete_item(t);

  return TRUE;
}
