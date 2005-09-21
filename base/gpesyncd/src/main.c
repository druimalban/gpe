/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Parts of this file are derieved from the nsqld from Phil Blundell
 *  See http://handhelds.org/cgi-bin/cvsweb.cgi/gpe/base/nsqld/
 *  for information about it.
 */

#include "gpesyncd.h"

int verbose = 0;

int
callback_list (void *arg, int argc, char **argv, char **names)
{
  GSList **tables = (GSList **) arg;
  int i;
  for (i = 0; i < argc; i++)
    *tables = g_slist_append (*tables, argv[i]);

  return 0;
}

/*! \brief Initializes the databases
 *
 * \param ctx	The current context
 *
 * Opens the contacts, todo, calendar databases and initializes them
 * with the default schemes if the databases didn't exist.
 */
void
gpesyncd_setup_databases (gpesyncd_context * ctx)
{
  GSList *tables = NULL, *iter;
  gchar *err;
  if (ctx->contact_db)
    {
      gchar *err = NULL;
      sqlite_exec (ctx->contact_db, "select max(urn) from contacts",
		   callback_list, &tables, &err);
      if ((err) && (!strcasecmp (err, "no such table: contacts")))
	{
	  fprintf (stderr, "No contacts tables found. Creating...");
	  sqlite_exec (ctx->contact_db,
		       "create table contacts (urn INTEGER NOT NULL, tag TEXT NOT NULL, value TEXT NOT NULL)",
		       0, 0, 0);
	  sqlite_exec (ctx->contact_db,
		       "create table contacts_config (id INTEGER PRIMARY KEY,   cgroup INTEGER NOT NULL, cidentifier TEXT NOT NULL, cvalue TEXT)",
		       0, 0, 0);
	  sqlite_exec (ctx->contact_db,
		       "INSERT INTO contacts_config VALUES(1,0,'Name','NAME')",
		       0, 0, 0);
	  sqlite_exec (ctx->contact_db,
		       "INSERT INTO contacts_config VALUES(2,0,'Phone','HOME.TELEPHONE')",
		       0, 0, 0);
	  sqlite_exec (ctx->contact_db,
		       "INSERT INTO contacts_config VALUES(3,0,'EMail','HOME.EMAIL')",
		       0, 0, 0);
	  sqlite_exec (ctx->contact_db,
		       "create table contacts_urn (urn INTEGER PRIMARY KEY, name TEXT, family_name TEXT, company TEXT)",
		       0, 0, 0);
	  fprintf (stderr, " done.\n");
	  g_free (err);
	  err = NULL;
	}
    }
  if (ctx->event_db)
    {
      gchar *err = NULL;
      sqlite_exec (ctx->event_db, "select max(uid) from calendar",
		   callback_list, &tables, &err);
      if ((err) && (!strcasecmp (err, "no such table: calendar")))
	{
	  fprintf (stderr, "No calendar tables found. Creating...");
	  sqlite_exec (ctx->event_db,
		       "create table calendar (uid INTEGER NOT NULL, tag TEXT NOT NULL, value TEXT)",
		       0, 0, 0);
	  sqlite_exec (ctx->event_db,
		       "create table calendar_dbinfo (version integer NOT NULL)",
		       0, 0, 0);
	  sqlite_exec (ctx->event_db, "INSERT INTO calendar_dbinfo VALUES(1)",
		       0, 0, 0);
	  sqlite_exec (ctx->event_db,
		       "create table calendar_urn (uid INTEGER PRIMARY KEY)",
		       0, 0, 0);
	  fprintf (stderr, " done.\n");
	  g_free (err);
	  err = NULL;
	}
    }
  if (ctx->todo_db)
    {
      gchar *err = NULL;
      sqlite_exec (ctx->todo_db, "select max(uid) from todo", callback_list,
		   &tables, &err);
      if ((err) && (!strcasecmp (err, "no such table: todo")))
	{
	  fprintf (stderr, "No todo tables found. Creating...");
	  sqlite_exec (ctx->todo_db,
		       "create table todo (uid INTEGER NOT NULL, tag TEXT NOT NULL, value TEXT)",
		       0, 0, 0);
	  sqlite_exec (ctx->todo_db,
		       "create table todo_dbinfo (version integer NOT NULL)",
		       0, 0, 0);
	  sqlite_exec (ctx->todo_db, "INSERT INTO todo_dbinfo VALUES(1)", 0,
		       0, 0);
	  sqlite_exec (ctx->todo_db,
		       "create table todo_urn (uid INTEGER PRIMARY KEY)", 0,
		       0, 0);
	  fprintf (stderr, " done.\n");
	  g_free (err);
	  err = NULL;
	}
    }
}

/*! \brief Creates a new context
 * 
 */
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
  if (verbose)
    fprintf (stderr, "Opening: %s\n", buf);
  if (!ctx->event_db)
    {
      g_free (buf);
      return NULL;
    }
  sprintf (buf, "%s%s", home, "/.gpe/contacts");
  if (verbose)
    fprintf (stderr, "Opening: %s\n", buf);
  ctx->contact_db = sqlite_open (buf, 0, &err);
  if (!ctx->contact_db)
    {
      g_free (buf);
      return NULL;
    }
  sprintf (buf, "%s%s", home, "/.gpe/todo");
  if (verbose)
    fprintf (stderr, "Opening: %s\n", buf);
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

/*! \brief Frees the memory of a context
 */
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

/*! \brief Prints all the output
 *
 * \param ctx		the context
 * \param *format	The string
 *
 * When using in remote mode, we need the length of the output
 * at the beginning of it. (e.g.: "11:get vcard 3") so we can
 * process the command / data correct.
 */
void
gpesyncd_printf (gpesyncd_context * ctx, char *format, ...)
{
  char *buf;
  va_list ap;
  va_start (ap, format);

  g_vasprintf (&buf, format, ap);
  va_end (ap);

  if (verbose)
    fprintf (stderr, "[gpesyncd out]: %s\n", buf);

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
get_next_token (gchar * str, guint *num)
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

  gchar *data = string->str;
  g_string_free (string, FALSE);
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

/*! \brief executes a command
 *
 * \param ctx		The current context
 * \param command	The command which should be executed
 *
 * \returns	FALSE, if we want to exit, otherwise TRUE
 */
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
  guint modified = 0;
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
	  guint uidlen = 0;
	  uidstr = get_next_token (cmd_string->str + pos, &uidlen);
	  uid = atoi (uidstr);
	  if (uid > 0)
	    pos += uidlen + 1;
	}

      g_free (buf);
    }

  if (pos < cmd_string->len)
    {
      data = (gchar *) (cmd_string->str + pos - 1);
      /* We remove any leading spaces */
      while (data[0] == ' ')
	*data++;
    }
  else
    data = NULL;

  GError *error = NULL;

  if (verbose)
    {
      if (data)
	fprintf (stderr, "cmd: %s type: %d uid: %d data[0]: %c\n",
		 cmd, type, uid, data[0]);
      else
	fprintf (stderr, "cmd: %s type: %d uid: %d\n", cmd, type, uid);
    }

  if ((!strcasecmp (cmd, "GET")) && (type != GPE_DB_TYPE_UNKNOWN)
      && (uid > 0))
    {
      gchar *get_result;
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  get_result = get_contact (ctx, uid, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  get_result = get_event (ctx, uid, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  get_result = get_todo (ctx, uid, &error);
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
      if (get_result)
	{
	  g_string_append (ctx->result, get_result);
	  g_free (get_result);
	}
      else
	g_string_append (ctx->result, "Error: No item found\n");
    }
  else if ((!strcasecmp (cmd, "ADD")) && (type != GPE_DB_TYPE_UNKNOWN)
	   && (data))
    {
      uid = 0;
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  cmd_result =
	    add_item (ctx, &uid, "contacts", data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result =
	    add_item (ctx, &uid, "calendar", data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result = add_item (ctx, &uid, "todo", data, &modified, &error);
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
      if (cmd_result) 
        {
	  /* We need to return the modified and the uid value so
	   * that we can report changes to opensync */
	  g_string_printf (ctx->result, "OK:%d:%d\n", modified, uid);
	}
      
    }
  else if ((!strcasecmp (cmd, "MODIFY")) && (type != GPE_DB_TYPE_UNKNOWN)
	   && (data) && (uid > 0))
    {
      switch (type)
	{
	case GPE_DB_TYPE_VCARD:
	  cmd_result =
	    modify_item (ctx, uid, "contacts", data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result =
	    modify_item (ctx, uid, "calendar", data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result =
	    modify_item (ctx, uid, "todo", data, &modified, &error);
	  break;
	default:
	  g_string_append (ctx->result, "Error: wrong type\n");
	}
      if (error)
	fprintf (stderr, "Error found in modify\n");

      if (cmd_result)
	g_string_printf (ctx->result, "OK:%d\n", modified);
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
      if (cmd_result)
	g_string_printf (ctx->result, "OK:0\n");
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
      else if (!error)
	g_string_append (ctx->result, "Error: No item found\n");

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
      return FALSE;
    }
  else
    {
      replace_newline (cmd);
      g_string_append_printf (ctx->result, "Invalid command: %s\n", cmd);
    }
  
  g_free (cmd);

  if (verbose)
    fprintf (stderr, "error: %d, data: %d, cmd_result: %d\n", (int) error,
	     (int) data, (int) cmd_result);

  /* Either we have an error, return data, or just a 
   * small "OK" */
  if (error)
    {
      g_string_append_printf (ctx->result, "Error: %s\n", error->message);
      g_error_free (error);
      error = NULL;
    }
  else if (!ctx->result->len)
    {
      if (cmd_result)
	g_string_append_printf (ctx->result, "OK\n");
      else
	g_string_append_printf (ctx->result, "UNKNOWN ERROR\n");
    }
  gpesyncd_printf (ctx, ctx->result->str);
  g_string_assign (ctx->result, "");

  return TRUE;
}

/*! \brief parses the commands from stdin
 *
 * \param ctx	The current context
 */
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
	  if (!strncasecmp (tmpstr, "BEGIN", 5))
	    {
	      addtype = strchr (tmpstr, 'V');
	      if (!addtype)
		addtype = strchr (tmpstr, 'v');
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
      if (!do_command (ctx, cmd_string->str))
	break;
      g_string_assign (cmd_string, "");
    }

  gpesyncd_context_free (ctx);
  g_string_free (cmd_string, TRUE);
}

/* \brief Parses the commands from ctx->ifp
 *
 * \param ctx	The current context
 *
 * Reads the commands from ctx->ifp. They must have the length
 * of the full command in the beginning of it.
 */
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

  int len = 0;
  gboolean have_len = FALSE;
  char c;

  GString *buf;
  buf = g_string_new ("");

  while ((!ctx->quit) && (!feof (ctx->ifp)))
    {
      c = fgetc (ctx->ifp);

      if (have_len == FALSE)
	{
	  if (c == ':')
	    {
	      len = atoi (buf->str);
	      have_len = TRUE;
	      g_string_assign (buf, "");
	      continue;
	    }
	  else if ((c > '9') || (c < '0'))
	    {
	      g_string_assign (buf, "");
	      len = 0;
	      have_len = FALSE;
	      continue;
	    }
	}
      else
	{
	  if (buf->len == len-1)
	    {
	      g_string_append_c (buf, c);

	      if (verbose)
		fprintf (stderr, "Executing command: %s", buf->str);

	      /* If we want to exit, we have to break out
	       * of this loop. */
	      if (!do_command (ctx, buf->str))
		  break;

	      g_string_assign (buf, "");
	      have_len = FALSE;
	      len = 0;
	      continue;
	    }
	}

      g_string_append_c (buf, c);
    }

  g_string_free (buf, TRUE);
  gpesyncd_context_free (ctx);

  return 0;
}

int
main (int argc, char **argv)
{
  gchar *err = NULL;
  gpesyncd_context *ctx = gpesyncd_context_new (err);

  gpesyncd_setup_databases (ctx);

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
	exit (0);
      }

  command_loop (ctx);

  return 0;
}
