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
item_list_callback (void *arg, int argc, char **argv, char **names)
{
  GSList **list = (GSList **) arg;
  GString *data = g_string_new ("");
  if (argc == 1)
    {
      *list = g_slist_append (*list, g_strdup (argv[0]));
    }
  if (argc == 2)
    {
      g_string_printf (data, "%s:%s", argv[0], argv[1]);
      *list = g_slist_append (*list, g_strdup (data->str));
    }
  g_string_free (data, TRUE);
  return 0;
}

static int
item_string_callback (void *arg, int argc, char **argv, char **names)
{
  gchar **string = (gchar **) arg;
  
  if (argc == 1)
    {
      *string = g_strdup (argv[0]);
    }
  return 0;
}
static int
tag_list_callback (void *arg, int argc, char **argv, char **names)
{
  GSList **tags = (GSList **) arg;
  gpe_tag_pair *p = g_malloc0 (sizeof (gpe_tag_pair));
  if (argc == 2)
    {
      p->tag = g_strdup (argv[0]);
      p->value = g_strdup (argv[1]);
      *tags = g_slist_append (*tags, p);
    }
  return 0;
}

gchar *
get_contact (gpesyncd_context * ctx, guint uid, GError ** error)
{
  GSList *tags = NULL;
  gchar *data = NULL;
  char *err;
  if (sqlite_exec_printf
      (ctx->contact_db, "select tag,value from contacts where urn='%d'",
       tag_list_callback, &tags, 0, uid))
    {
      g_set_error (error, 0, 101, err);
      return NULL;
    }

  if (!tags)
    return NULL;

  MIMEDirVCard *vcard = vcard_from_tags (tags);

  data = mimedir_vcard_write_to_string (vcard);

  g_object_unref (vcard);
  return data;
}

gchar *
get_event (gpesyncd_context * ctx, guint uid, GError ** error)
{
  GSList *tags = NULL;
  char *err;
  if (sqlite_exec_printf
      (ctx->event_db, "select tag,value from calendar where uid='%d'",
       tag_list_callback, &tags, 0, uid))
    {
      g_set_error (error, 0, 102, err);
      return NULL;
    }

  if (!tags)
    return NULL;


  MIMEDirVCal *vcal = mimedir_vcal_new ();
  MIMEDirVEvent *vevent = vevent_from_tags (tags);

  mimedir_vcal_add_component (vcal, (MIMEDirVComponent *) vevent);

  gchar *data = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);
  g_object_unref (vevent);

  return data;
}

gchar *
get_todo (gpesyncd_context * ctx, guint uid, GError ** error)
{
  GSList *tags = NULL;
  char *err;
  if (sqlite_exec_printf
      (ctx->todo_db, "select tag,value from todo where uid='%d'",
       tag_list_callback, &tags, 0, uid))
    {
      g_set_error (error, 0, 103, err);
      return NULL;
    }

  if (!tags)
    return NULL;

  MIMEDirVCal *vcal = mimedir_vcal_new ();
  MIMEDirVTodo *vtodo = vtodo_from_tags (tags);

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
  char *err;

  /* At first we need to get the uids: */
  if (sqlite_exec
      (ctx->contact_db,
       "select distinct urn from contacts",
       item_list_callback, &uid_list, 0))
    {
      g_set_error (error, 0, 111, err);
      return NULL;
    }
  GString *uid_modified = g_string_new ("");

  /* Now we got to get their modified tags. */
  for (iter = uid_list; iter; iter = g_slist_next (iter))
    {
      gchar *modified;  
     
      modified = NULL;

      if (sqlite_exec_printf
	  (ctx->contact_db,
	   "select value from contacts where upper(tag)='MODIFIED' and urn=%s",
	   item_string_callback, &modified, 0, (gchar *) iter->data))
	{
	  g_set_error (error, 0, 111, err);
	  return NULL;
	}
     
      /* if we didn't get one, report it as 0 */
      if (modified)
	g_string_printf (uid_modified, "%s:%s", (gchar *) iter->data, modified);
      else
	g_string_printf (uid_modified, "%s:0", (gchar *) iter->data);

      g_free (modified);
      g_free (iter->data);
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
  GSList *uid_list = NULL, *iter;
  char *err;

  /* At first we need to get the uids: */
  if (sqlite_exec
      (ctx->event_db,
       "select distinct uid from calendar",
       item_list_callback, &uid_list, 0))
    {
      g_set_error (error, 0, 111, err);
      return NULL;
    }
  GString *uid_modified = g_string_new ("");

  /* Now we got to get their modified tags. */
  for (iter = uid_list; iter; iter = g_slist_next (iter))
    {
      gchar *modified;  
     
      modified = NULL;

      if (sqlite_exec_printf
	  (ctx->event_db,
	   "select value from calendar where upper(tag)='MODIFIED' and uid=%s",
	   item_string_callback, &modified, 0, (gchar *) iter->data))
	{
	  g_set_error (error, 0, 111, err);
	  return NULL;
	}
     
      /* if we didn't get one, report it as 0 */
      if (modified)
	g_string_printf (uid_modified, "%s:%s", (gchar *) iter->data, modified);
      else
	g_string_printf (uid_modified, "%s:0", (gchar *) iter->data);

      g_free (modified);
      g_free (iter->data);
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
get_todo_uid_list (gpesyncd_context * ctx, GError ** error)
{
  GSList *uid_list = NULL, *iter;
  char *err;

  /* At first we need to get the uids: */
  if (sqlite_exec
      (ctx->todo_db,
       "select distinct uid from todo",
       item_list_callback, &uid_list, 0))
    {
      g_set_error (error, 0, 111, err);
      return NULL;
    }
  GString *uid_modified = g_string_new ("");

  /* Now we got to get their modified tags. */
  for (iter = uid_list; iter; iter = g_slist_next (iter))
    {
      gchar *modified;  
     
      modified = NULL;

      if (sqlite_exec_printf
	  (ctx->todo_db,
	   "select value from todo where upper(tag)='MODIFIED' and uid=%s",
	   item_string_callback, &modified, 0, (gchar *) iter->data))
	{
	  g_set_error (error, 0, 111, err);
	  return NULL;
	}
     
      /* if we didn't get one, report it as 0 */
      if (modified)
	g_string_printf (uid_modified, "%s:%s", (gchar *) iter->data, modified);
      else
	g_string_printf (uid_modified, "%s:0", (gchar *) iter->data);

      g_free (modified);
      g_free (iter->data);
      iter->data = g_strdup (uid_modified->str);

    }
  g_string_free (uid_modified, TRUE);

  return uid_list;
}
