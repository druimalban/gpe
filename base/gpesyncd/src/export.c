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
  char *err;
  if (sqlite_exec_printf
      (ctx->contact_db, "select tag,value from contacts where urn='%d'",
       tag_list_callback, &tags, 0, uid))
    {
      g_set_error (error, 0, 101, err);
      return NULL;
    }

  MIMEDirVCard *vcard = vcard_from_tags (tags);

  gchar *data = mimedir_vcard_write_to_string (vcard);

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

  MIMEDirVCal *vcal = mimedir_vcal_new ();
  MIMEDirVTodo *vtodo = vtodo_from_tags (tags);

  mimedir_vcal_add_component (vcal, (MIMEDirVComponent *) vtodo);

  gchar *data = mimedir_vcal_write_to_string (vcal);
  g_object_unref (vcal);
  g_object_unref (vtodo);

  return data;
}

GSList *
get_contact_uid_list (gpesyncd_context * ctx, GError ** error)
{
  GSList *list = NULL;
  char *err;
  if (sqlite_exec
      (ctx->contact_db,
       "select urn,value from contacts where upper(tag)='MODIFIED' order by urn",
       item_list_callback, &list, 0))
    {
      g_set_error (error, 0, 111, err);
      return NULL;
    }
  return list;
}

GSList *
get_event_uid_list (gpesyncd_context * ctx, GError ** error)
{
  GSList *list = NULL;
  char *err;
  if (sqlite_exec
      (ctx->event_db,
       "select uid,value from calendar where upper(tag)='MODIFIED' order by uid",
       item_list_callback, &list, 0))
    {
      g_set_error (error, 0, 112, err);
      return NULL;
    }
  return list;
}

GSList *
get_todo_uid_list (gpesyncd_context * ctx, GError ** error)
{
  GSList *list = NULL;
  char *err;
  fprintf (stderr, "Getting todo_list\n");
  if (sqlite_exec
      (ctx->todo_db,
       "select uid,value from todo where upper(tag)='MODIFIED' order by uid",
       item_list_callback, &list, 0))
    {
      g_set_error (error, 0, 113, err);
      return NULL;
    }
  fprintf (stderr, "Getting todo_list\n");
  return list;
}
