/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  Copyright (C) 2006, 2007 Graham Cobb <g+gpe@cobb.uk.net>
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

/* Free slist containing pointers to objects */
extern void free_object_list(GSList *list) {
  GSList *i;

  /* Empty list ? */
  if (!list) return;

  for (i=list; i; i = g_slist_next(i)) {
    g_object_unref(i->data);
  }
  g_slist_free(list);

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
  if (contacts_db_open(FALSE)) {
    fprintf(stderr, "Could not open contacts database.\n");
  }

  GString *filename = g_string_new(g_get_home_dir());
  g_string_append(filename, "/.gpe/calendar");
  ctx->event_db = event_db_new (filename->str, NULL);
  g_string_free(filename, TRUE);
  if (!ctx->event_db) {
    fprintf(stderr, "Could not open calendar database.\n");
  }

  /* Get the calendars */
  ctx->event_calendars = event_db_list_event_calendars (ctx->event_db, NULL);

  if (todo_db_start()) {
    fprintf(stderr, "Could not open todo database.\n");
  }

}

/*! \brief Checks whether an ip is allowed to connect or not
 * 
 * \param ip	The ip adress
 *
 * The check is done by comparing every line of
 * $HOME/.gpe/gpesyncd.allow with ip. If the ip is found,
 * 1 is returned, otherwise 0.
 */
int
check_ip (const char *ip)
{
  char *home;
  char *path;
  char buffer[16];
  FILE *fp;

  home = getenv ("HOME");
  path =
    calloc (sizeof (char) * strlen (home) + strlen ("/.gpe/gpesyncd.allow") +
	    1, 1);

  if (!path)
    {
      fprintf (stderr,
	       "Couldn't allocate enough memory for the ip-filter!\n");
      return 0;
    }

  sprintf (path, "%s/.gpe/gpesyncd.allow", home);

  fp = fopen (path, "r");
  if (!fp)
    {
      fprintf (stderr, "Error opening %s\n", path);
      return 0;
    }

  while (!feof (fp))
    {
      bzero (buffer, 16);
      fgets (buffer, 16, fp);
      buffer[strlen (buffer) - 1] = '\0';
      if (!strcmp (buffer, ip))
	{
	  fclose (fp);
	  return 1;
	}
    }

  fclose (fp);

  return 0;
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
  ctx->socket = 0;
  ctx->event_calendars = NULL;

  ctx->result = g_string_new ("");

  return ctx;
}

/*! \brief Frees the memory of a context
 */
void
gpesyncd_context_free (gpesyncd_context * ctx)
{
  if (ctx)
    {
      /* Contacts DB */
      contacts_db_close();

      /* Events DB */
      free_object_list(ctx->event_calendars);
      ctx->event_calendars = NULL;
      g_object_unref(ctx->event_db);
      ctx->event_db = 0;

      /* Todo DB */
      todo_db_stop();

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

  if (ctx->remote)
    {
      fprintf (ctx->ofp, "%d:%s", strlen (buf), buf);
      fflush (ctx->ofp);
    }
  else
    puts (buf);

  g_free (buf);
}

gchar *
get_last_token (gchar * str, int *num)
{
  char *last = strchr (str, ' ');
  return last;
}

gchar *
get_next_token (gchar * str, guint * num)
{
  GString *string = g_string_new ("");
  int i = 0;
  do
    {
      string = g_string_append_c (string, str[i]);
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
      c[0] = ' ';
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

  g_string_assign (ctx->result, "");

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
	  g_free (uidstr);
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
	    add_contact (ctx, &uid, data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result =
	    add_event (ctx, &uid, data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result = add_todo (ctx, &uid, data, &modified, &error);
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
	    add_contact (ctx, &uid, data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result =
	    add_event (ctx, &uid, data, &modified, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result =
	    add_todo (ctx, &uid, data, &modified, &error);
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
	  cmd_result = del_contact (ctx, uid, &error);
	  break;
	case GPE_DB_TYPE_VEVENT:
	  cmd_result = del_event (ctx, uid, &error);
	  break;
	case GPE_DB_TYPE_VTODO:
	  cmd_result = del_todo (ctx, uid, &error);
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
      gchar *err;
      gpesyncd_printf (ctx, "Exiting!\n");
      fprintf (stderr, "Compressing contacts database...");
      err=contacts_db_compress();
      if (err) {
	fprintf (stderr, "failed\n");
	g_free(err);
      } else {
	fprintf (stderr, "succeeded\n");
      }
      fprintf (stderr, "Contacts DB size = %d\n",contacts_db_size());
      g_free (cmd);
      return FALSE;
    }
  else
    {
      replace_newline (cmd);
      g_string_printf (ctx->result, "Invalid command: %s\n", cmd);
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

  if (!ctx->socket)
    gpesyncd_printf (ctx, "%s", ctx->result->str);

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

      gchar c = fgetc (ctx->ifp);
      do
	{
	  g_string_append_c (cmd_string, c);
	  c = fgetc (ctx->ifp);
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
		      c = fgetc (ctx->ifp);
		      do
			{
			  g_string_append_c (read_buffer, c);
			  c = fgetc (ctx->ifp);
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
	  if (buf->len == len - 1)
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
daemon_loop (gpesyncd_context * ctx)
{
  gint n, sent_bytes;
  gchar buffer[BUFFER_LEN];
  gchar *result;
  GString *command;

  bzero (buffer, BUFFER_LEN);

  command = g_string_new ("");

  while (n = read (ctx->socket, buffer, BUFFER_LEN - 1))
    {
      if (n < 0)
	perror ("reading from socket");

      g_string_append_len (command, buffer, strlen (buffer));
      if (strlen (buffer) < BUFFER_LEN - 1)
	break;
      bzero (buffer, BUFFER_LEN);
    }

  if (!strncasecmp (command->str, "quit", 4))
    return 1;

  do_command (ctx, command->str);

  sent_bytes = 0;
  while (sent_bytes < ctx->result->len)
    {
      n = send (ctx->socket, ctx->result->str, ctx->result->len, 0);
      if (n < 0)
	{
	  perror ("sending through socket!");
	  exit (1);
	}
      sent_bytes += n;
    }
  g_string_free (command, TRUE);

  return 0;
}

void
sigchld_handler (int s)
{
  while (waitpid (-1, NULL, WNOHANG) > 0);
}

/*! \brief Sets up a port and listens on that for incoming connections
 *
 * \param ctx	The context of the gpesyncd
 *
 * This is just a standard function, taken from beej's socket tutorial.
 */
int
setup_daemon (gpesyncd_context * ctx, int port)
{
  int sockfd, new_fd, n, doquit;
  struct sockaddr_in daemon_addr;
  struct sockaddr_in client_addr;
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;

  if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      exit (1);
    }

  if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1)
    {
      perror ("setsockopt");
      exit (1);
    }

  daemon_addr.sin_family = AF_INET;
  daemon_addr.sin_port = htons (port);
  daemon_addr.sin_addr.s_addr = INADDR_ANY;
  memset (&(daemon_addr.sin_zero), '\0', 8);

  if (bind
      (sockfd, (struct sockaddr *) &daemon_addr,
       sizeof (struct sockaddr)) == -1)
    {
      perror ("bind");
      exit (1);
    }

  if (listen (sockfd, 10) == -1)
    {
      perror ("listen");
      exit (1);
    }

  sa.sa_handler = sigchld_handler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction (SIGCHLD, &sa, NULL) == -1)
    {
      perror ("sigaction");
      exit (1);
    }

  fprintf (stderr, "Running Daemon mode. Listening on Port %d\n", port);
  while (1)
    {
      sin_size = sizeof (struct sockaddr_in);
      if ((new_fd = accept (sockfd, (struct sockaddr *) &client_addr,
			    &sin_size)) == -1)
	{
	  perror ("accept");
	  continue;
	}

      if (check_ip (inet_ntoa (client_addr.sin_addr)))
	{
	  if (send (new_fd, "OK\n", 3, 0) < 3)
	    {
	      perror ("sending");
	      shutdown (new_fd, SHUT_RDWR);
	    }
	  else
	    {
	      printf ("gpesyncd: allowing connection from %s\n",
		      inet_ntoa (client_addr.sin_addr));


	      if (!fork ())
		{
		  close (sockfd);

		  ctx->socket = new_fd;

		  doquit = 0;
		  while (!doquit)
		    doquit = daemon_loop (ctx);

		  if (shutdown (new_fd, SHUT_RDWR))
		    perror ("closing socket");
		  exit (0);
		}
	    }
	}
      else
	{

	  if (send (new_fd, "Your're not allowed to connect!\n", 33, 0) < 33)
	    {
	      perror ("sending");
	      fprintf (stderr, "Error sending blocking message\n");
	    }

	  fprintf (stderr, "gpesyncd: blocking connection from %s\n",
		   inet_ntoa (client_addr.sin_addr));

	  if (shutdown (new_fd, SHUT_RDWR))
	    perror ("closing socket");
	}

    }

  return 0;
}

int
main (int argc, char **argv)
{
  gchar *err = NULL;

#ifdef USE_VARTMP
  /* Set the temporary file directory to something useful on the 770
     Note that setting TMPDIR, TMP or TEMP will still override this */
  setenv("TEMP", "/var/tmp", 0);
#endif

  g_type_init ();

  gpesyncd_context *ctx = gpesyncd_context_new (err);

  if (!ctx)
    {
      fprintf (stderr, "Error initializing gpesyncd: %s\n", err);
      return -1;
    }

  gpesyncd_setup_databases (ctx);

  ctx->remote = 0;
  if (argc > 1)
    {
      if (!strcasecmp (argv[1], "--help"))
        {
	  printf ("usage: %s [OPTION] \n\n", argv[0]);
	  printf ("gpesyncd is a tool to import and export vcard,vevent and vtodos. When running\n");
	  printf ("enter \"help\" for the available commands.\n\n");
          printf ("Possible options: \n");
	  printf (" -r, --remote        Starts gpesyncd in remote mode, which means that all\n");
	  printf ("                     input must be entered as <nn>:<data> where <nn> is the\n");
	  printf ("                     length of the data <data>.\n");
	  printf ("                     Output follows the same convention.\n");
          printf (" -d, --daemon [PORT] Starts in tcp/ip mode. Listens on port 6446 unless PORT\n");
	  printf ("                     is specified.\n");

	  exit (0);
	}
      else if ( (!strcasecmp (argv[1], "--remote")) 
	|| (!strcasecmp (argv[1], "-r")) )
	{
	  ctx->remote = 1;
	  remote_loop (ctx);
	  exit (0);
	}
      else if ( (!strcasecmp (argv[1], "--daemon")) 
	  || (!strcasecmp (argv[1], "-d")))
	{
	  int port = 6446;

	  if (argc == 3)
	    port = atoi (argv[2]);

	  setup_daemon (ctx, port);
	  exit (0);
	}
    }

  ctx->ifp = stdin;
  ctx->ofp = stdout;

  printf ("gpesyncd local mode\n");
  command_loop (ctx);

  return 0;
}
