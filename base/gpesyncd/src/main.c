/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include "gpesyncd.h"

int verbose = 0;

gpesyncd_context *
gpesyncd_context_new (char *err)
{
  if (err)
    {
      return NULL;
    }
  gpesyncd_context *ctx = g_malloc0 (sizeof (gpesyncd_context));
  const char *home = getenv ("HOME");
  char *buf;
  size_t len;

  ctx->ifp = NULL;
  ctx->ofp = NULL;

  len = strlen (home) + strlen ("/./gpe/calendar") + 1;
  buf = g_malloc0 (len);

  sprintf (buf, "%s%s", home, "/.gpe/calendar");
  ctx->event_db = sqlite_open (buf, 0, &err);
  if (!ctx->event_db)
    {
      g_free (buf);
      return NULL;
    }
  sprintf (buf, "%s%s", home, "/.gpe/contacts");
  ctx->contact_db = sqlite_open (buf, 0, &err);
  if (!ctx->contact_db)
    {
      g_free (buf);
      return NULL;
    }
  sprintf (buf, "%s%s", home, "/.gpe/todo");
  ctx->todo_db = sqlite_open (buf, 0, &err);
  if (!ctx->todo_db)
    {
      g_free (buf);
      return NULL;
    }

  ctx->result = g_string_new ("");

  g_free (buf);

  return ctx;
}

void
gpesyncd_context_free (gpesyncd_context * ctx)
{
  if (ctx)
    {
      if (ctx->contact_db)
	sqlite_close (ctx->contact_db);
      if (ctx->event_db)
	sqlite_close (ctx->event_db);
      if (ctx->todo_db)
	sqlite_close (ctx->todo_db);

      if (ctx->ifp)
	fclose (ctx->ifp);
      if (ctx->ofp)
	fclose (ctx->ofp);

      if (ctx->result)
	g_string_free (ctx->result, TRUE);

      g_free (ctx);
    }
}

void
gpesyncd_printf (gpesyncd_context * ctx, char *format, ...)
{
  char *buf;
  va_list ap;
  va_start (ap, format);

  g_vasprintf (&buf, format, ap);
  va_end (ap);

  if (ctx->ofp)
    {
      fprintf (ctx->ofp, "%d:%s", strlen (buf), buf);
      fflush (ctx->ofp);
    }
  else
    printf (buf);

  g_free (buf);
}

gchar *
get_last_token (gchar * str, int *num)
{
  char *last = strchr (str, ' ');
  return last;
}

gchar *
get_next_token (gchar * str, int *num)
{
  GString *string = g_string_new ("");
  int i = 0;
  do
    {
      g_string_append_c (string, str[i]);
      i++;
    }
  while ((str[i] != ' ') && (str[i] != '\0'));

  if (num)
    *num += i + 1;

  gchar *data = g_strdup (string->str);
  g_string_free (string, TRUE);
  return data;
}

void
replace_newline (gchar * str)
{
  gchar *c;
  c = strchr (str, '\n');
  while (c)
    {
      c[0] = '%';
      c = strchr (str, '\n');
    }
}

gboolean
do_command (gpesyncd_context * ctx, gchar * command)
{
  GString *cmd_string = g_string_new (command);

  gchar *buf = NULL;
  gchar *cmd = NULL;
  gchar *data = NULL;
  gchar *uidstr = NULL;
  guint pos = 0;
  gpe_db_type type = GPE_DB_TYPE_UNKNOWN;
  guint uid = 0;
  gboolean cmd_result = FALSE;

  //printf ("Processing command %s\n", command);

  cmd = get_next_token (cmd_string->str, &pos);

  if (strlen (cmd) < cmd_string->len)
    {
      buf = get_next_token (cmd_string->str + pos, &pos);

      type = GPE_DB_TYPE_UNKNOWN;

      if (!strncasecmp (buf, "VCARD", 5))
	type = GPE_DB_TYPE_VCARD;
      else if (!strncasecmp (buf, "VEVENT", 6))
	type = GPE_DB_TYPE_VEVENT;
      else if (!strncasecmp (buf, "VTODO", 5))
	type = GPE_DB_TYPE_VTODO;

      if (pos < cmd_string->len)
	{
	  int uidlen = 0;
	  uidstr = get_next_token (cmd_string->str + pos, &uidlen);
	  uid = atoi (uidstr);
	  if (uid > 0)
	    pos += uidlen + 1;
	}
    }

  data = (gchar *) (cmd_string->str + pos);

  GError *error = NULL;

  //fprintf (stderr, "cmd: %s, typestr: %s, type: %d, uid: %d data[0]: %c\n", cmd, buf, type, uid, data[0]);
  if ((!strcasecmp (cmd, "GET")) && (type != GPE_DB_TYPE_UNKNOWN)
      && (uid > 0))
    {
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  g_string_append (ctx->result, get_contact (ctx, uid, &error));
	  break;
	case GPE_DB_TYPE_VEVENT:
	  g_string_append (ctx->result, get_event (ctx, uid, &error));
	  break;
	case GPE_DB_TYPE_VTODO:
	  g_string_append (ctx->result, get_todo (ctx, uid, &error));
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
    }
  else if ((!strcasecmp (cmd, "ADD")) && (type != GPE_DB_TYPE_UNKNOWN))
    {
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  cmd_result = add_item (ctx, uid, "contacts", data, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result = add_item (ctx, uid, "calendar", data, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result = add_item (ctx, uid, "todo", data, &error);
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
    }
  else if ((!strcasecmp (cmd, "MODIFY")) && (type != GPE_DB_TYPE_UNKNOWN)
	   && (uid > 0))
    {
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  cmd_result = del_item (ctx, uid, "contacts", &error);
	  if (cmd_result)
	    break;
	  else
	    cmd_result = add_item (ctx, uid, "contacts", data, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result = del_item (ctx, uid, "calendar", &error);
	  if (cmd_result)
	    break;
	  else
	    cmd_result = add_item (ctx, uid, "calendar", data, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result = del_item (ctx, uid, "todo", &error);
	  if (cmd_result)
	    break;
	  else
	    cmd_result = add_item (ctx, uid, "todo", data, &error);
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
    }
  else if ((!strcasecmp (cmd, "DEL")) && (type != GPE_DB_TYPE_UNKNOWN)
	   && (uid > 0))
    {
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  cmd_result = del_item (ctx, uid, "contacts", &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result = del_item (ctx, uid, "calendar", &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result = del_item (ctx, uid, "todo", &error);
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
    }
  else if ((!strcasecmp (cmd, "UIDLIST")) && (type != GPE_DB_TYPE_UNKNOWN))
    {
      GError *error = NULL;
      GSList *listdata = NULL;
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  listdata = get_contact_uid_list (ctx, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  listdata = get_event_uid_list (ctx, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  listdata = get_todo_uid_list (ctx, &error);
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
      if (listdata)
	{
	  GSList *iter;
	  for (iter = listdata; iter; iter = g_slist_next (iter))
	    {
	      g_string_append_printf (ctx->result, "%s\n",
				      (char *) iter->data);
	      g_free (iter->data);
	    }
	  g_slist_free (listdata);
	  listdata = NULL;
	}
    }

  else if (!strncasecmp (cmd, "HELP", 4))
    {
      g_string_append (ctx->result, "GET (VCARD|VEVENT|VTODO) UID\n");
      g_string_append (ctx->result, "ADD (VCARD|VEVENT|VTODO) DATA\n");
      g_string_append (ctx->result, "MODIFY (VCARD|VEVENT|VTODO) UID DATA\n");
      g_string_append (ctx->result, "DEL (VCARD|VEVENT|VTODO) UID\n");
      g_string_append (ctx->result, "UIDLIST (VCARD|VEVENT|VTODO)\n");
      g_string_append (ctx->result, "QUIT\n");
    }
  else if (!strncasecmp (cmd, "QUIT", 4))
    {
      gpesyncd_printf (ctx, "Exiting!\n");
      exit (0);
    }
  else
    {
      replace_newline (cmd);
      g_string_append_printf (ctx->result, "Invalid command: %s\n", cmd);
    }
  if (cmd)
    g_free (cmd);

  if (buf)
    g_free (buf);

//  fprintf (stderr, "error: %d, data: %d, cmd_result: %d\n", (int) error, (int) data, (int) cmd_result);
  /* Either we have an error, return data, or just a 
   * small "OK" */
  if (error)
    {
      g_string_append_printf (ctx->result, "Error: %s", error->message);
      g_error_free (error);
      error = NULL;
    }
  else if (!ctx->result->len)
    {
      if (cmd_result)
	g_string_append_printf (ctx->result, "OK");
      else
	g_string_append_printf (ctx->result, "UNKNOWN ERROR");
    }
  gpesyncd_printf (ctx, ctx->result->str);
  g_string_assign (ctx->result, "");

  return TRUE;
}



void
command_loop (gpesyncd_context * ctx)
{
  GString *cmd_string = g_string_new ("");
  gchar *tmpstr = NULL;
  gchar *addtype = NULL;
  while (strcasecmp (cmd_string->str, "QUIT"))
    {
      g_string_assign (cmd_string, "");

      gchar c = fgetc (stdin);
      do
	{
	  g_string_append_c (cmd_string, c);
	  c = fgetc (stdin);
	}
      while (c != '\n');
      g_string_append_c (cmd_string, c);

      /* Here we check if we are receiving data like a VCARD.
       * We look at the last word and if it is a
       * BEGIN:VCARD or a BEGIN:VCALENDAR.
       * If so, then we read until we get a END:VCARD or
       * END:VCALENDAR */
      tmpstr = strrchr (cmd_string->str, ' ');
      if (tmpstr)
	{
	  *tmpstr++;
	  if (!strncasecmp (tmpstr, "BEGINs", 5))
	    {
	      addtype = strchr (tmpstr, 'V');
	      if ((!strcasecmp (addtype, "VCALENDAR\n"))
		  || (!strcasecmp (addtype, "VCARD\n")))
		{
		  GString *read_buffer = g_string_new ("");
		  GString *end_string = g_string_new ("");
		  g_string_printf (end_string, "END:%s", addtype);
		  do
		    {
		      g_string_assign (read_buffer, "");
		      c = fgetc (stdin);
		      do
			{
			  g_string_append_c (read_buffer, c);
			  c = fgetc (stdin);
			}
		      while (c != '\n');
		      g_string_append_c (read_buffer, '\n');
		      g_string_append_printf (cmd_string, "%s",
					      read_buffer->str);

		      replace_newline (read_buffer->str);
		      replace_newline (end_string->str);
		    }
		  while (strcasecmp (read_buffer->str, end_string->str));

		  g_string_free (read_buffer, TRUE);
		  g_string_free (end_string, TRUE);
		}
	    }
	}
      do_command (ctx, cmd_string->str);
    }
  g_string_free (cmd_string, TRUE);
}

int
remote_loop (gpesyncd_context * ctx)
{
  if (!ctx)
    {
      fprintf (stderr, "[remote_loop] Critical error: No context provided\n");
      exit (-1);
    }
  ctx->ofp = stdout;
  ctx->ifp = stdin;

  int p = 0;
  int len = 0;
  gboolean have_len = FALSE;
  char c;

  char data[BUFFER_LENGTH];

  while ((!ctx->quit) && (!feof (ctx->ifp)))
    {
      c = fgetc (ctx->ifp);

      if (!feof (ctx->ifp))
	break;

      if (have_len == FALSE)
	{
	  if (c == ':')
	    {
	      data[p] = '\0';
	      len = atoi (data);
	      p = 0;
	      c = ' ';
	      have_len = TRUE;
	    }
	  else if ((c > '9') || (c < '0'))
	    {
	      p = 0;
	      c = ' ';
	    }
	}
      else
	{
	  if (p == len)
	    {
	      data[p] = c;
	      data[p + 1] = '\0';
	      do_command (ctx, data + 1);
	      have_len = FALSE;
	      len = 0;
	      p = 0;
	      c = ' ';
	    }
	}

      data[p] = c;
      if (p < sizeof (data) - 1)
	p++;

    }

  g_free (ctx);

  return 0;
}

int
main (int argc, char **argv)
{
  gchar *err = NULL;
  gpesyncd_context *ctx = gpesyncd_context_new (err);

  g_type_init ();

  if (!ctx)
    {
      fprintf (stderr, "Error initializing gpesyncd: %s\n", err);
      return -1;
    }

  int remote = 0;
  if (argc == 2)
    if (!strcasecmp (argv[1], "--REMOTE"))
      {
	remote = 1;
	remote_loop (ctx);
      }

  command_loop (ctx);

  return 0;
}
