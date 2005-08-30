/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *               2005 Martin Felis <martin@silef.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdarg.h>

#include "gpesync_client.h"

int verbose = 0;

struct gpesync_client
{
  int infd;
  int outfd;
  int seq;

  int busy;

  const gchar *hostname;
  const gchar *username;
};

struct gpesync_client_query_context
{
  struct gpesync_client *ctx;

  int type;

  gpesync_client_callback callback;
  void *cb_data;

  int result;
  gchar *error;
  gboolean aborting;
};


static void
write_command (gpesync_client * ctx, const char *cmd, ...)
{
  char *buf;
  va_list va;

  va_start (va, cmd);
  g_vasprintf (&buf, cmd, va);
  va_end (va);

  if (verbose)
    fprintf (stderr, "-> <%s>", buf);

  write (ctx->outfd, buf, strlen (buf));

  free (buf);
}

char *
get_next_line (const char *data)
{
  if (!strlen (data))
    {
      return NULL;
    }

  GString *string = g_string_new ("");
  int i = 0;
  do
    {
      if (data[i] != '\n')
	g_string_append_c (string, data[i]);
      i++;
    }
  while ((data[i] != '\0') && (data[i] != '\n'));

  if (data[i] == '\n')
    g_string_append_c (string, data[i]);

  gchar *token = NULL;
  if (string->len > 0)
    token = g_strdup (string->str);
  g_string_free (string, TRUE);

  return token;
}

static void
send_lines (struct gpesync_client_query_context *q, char *p)
{
  if (!q->aborting)
    {
      if (verbose)
	fprintf (stderr, "gpe: send <%s>\n", p);
      GSList *lines = NULL, *iter;

      int argc, i;
      char **argv;
      char *line, *line_iter;

      line_iter = p;
      line = get_next_line (line_iter);
      while (line)
	{
	  lines = g_slist_append (lines, g_strdup (line));
	  line_iter += strlen (line);
	  line = get_next_line (line_iter);
	}

      argc = g_slist_length (lines);
      argv = g_malloc0 (sizeof (char *) * argc);
      iter = lines;
      for (i = 0; i < argc; i++)
	{
	  argv[i] = iter->data;
	  iter = g_slist_next (iter);
	}

      if ( (q->callback != NULL) && (q->callback (q->cb_data, argc, argv)))
	{
	  fprintf (stderr, "aborting query\n");
	  q->result = GPESYNC_CLIENT_ABORT;
	  q->aborting = TRUE;
	}
      g_slist_free (lines);
      g_free (argv);
    }
}

static void
read_response (struct gpesync_client_query_context *q)
{
  char buf[1024];
  int p = 0;
  int len = 0;
  gboolean have_len = FALSE;
  gpesync_client *ctx;

  ctx = q->ctx;

  for (;;)
    {
      int rc;
      char c;

      rc = read (ctx->infd, &c, 1);
      if (rc < 0)
	{
	  perror ("read");
	  ctx->busy = 0;
	  break;
	}

      if (have_len == FALSE)
	{
	  if (c == ':')
	    {
	      buf[p] = '\0';
	      len = atoi (buf);
	      p = 0;
	      c = ' ';
	      have_len = TRUE;
	    }
	}
      else
	{
	  if (p == len)
	    {
	      buf[p] = c;
	      buf[p + 1] = '\0';
	      break;
	    }
	}

      buf[p] = c;
      if (p < sizeof (buf) - 1)
	p++;
    }

  if (ctx->busy)
    {
      char *p;

      p = buf;
      send_lines (q, p + 1);
      ctx->busy = 0;
    }
}

gpesync_client *
gpesync_client_open_ssh (const char *database, int mode, char **errmsg)
{
  gpesync_client *ctx;
  gchar *filename = NULL;
  gchar *hostname = NULL;
  const gchar *username = NULL;
  gchar *str;
  gchar *p;
  int in_fds[2], out_fds[2];
  pid_t pid;

  str = g_strdup (database);

  p = strchr (str, ':');
  if (p)
    {
      *p = 0;
      filename = p + 1;
      p = strchr (str, '@');
      if (p)
	{
	  *p = 0;
	  hostname = p + 1;
	  username = str;
	}
      else
	hostname = str;
    }

  if (hostname == NULL)
    hostname = "localhost";

  if (username == NULL)
    username = g_get_user_name ();

  if (filename == NULL)
    filename = "gpesyncd";
  ctx = g_malloc0 (sizeof (struct gpesync_client));

  pipe (in_fds);
  pipe (out_fds);

  pid = fork ();
  if (pid == 0)
    {
      dup2 (out_fds[0], 0);
      dup2 (in_fds[1], 1);
      close (out_fds[1]);
      close (in_fds[0]);
      fprintf (stderr, "connecting as %s to %s filename: %s\n", username, hostname, filename);
      execlp ("ssh", "ssh", "-l", username, hostname, "gpesyncd", "--remote",
	      NULL);
      perror ("exec");
    }

  close (out_fds[0]);
  close (in_fds[1]);

  ctx->outfd = out_fds[1];
  ctx->infd = in_fds[0];

  ctx->hostname = g_strdup (hostname);
  ctx->username = g_strdup (username);

  return ctx;
}

void
gpesync_client_close (gpesync_client * ctx)
{
  close (ctx->infd);
  if (ctx->infd != ctx->outfd)
    close (ctx->outfd);

  g_free (ctx);
  ctx = NULL;
}

int
gpesync_client_exec (gpesync_client * ctx, const char *command,
		 gpesync_client_callback cb, void *cb_data, char **err)
{
  struct gpesync_client_query_context query;
  GString *cmd = g_string_new ("");
  g_string_append_printf (cmd, "%d:%s", strlen (command), command);

  memset (&query, 0, sizeof (query));
  if (verbose)
    fprintf (stderr, "gpe:querying <%s>\n", command);
  query.ctx = ctx;
  query.callback = cb;
  query.cb_data = cb_data;
  query.aborting = FALSE;
  query.result = 0;
  query.error = NULL;

  ctx->busy = 1;

  write_command (ctx, cmd->str);

  while (ctx->busy)
    read_response (&query);

  if (err)
    *err = query.error;

  g_string_free (cmd, TRUE);

  return query.result;
}

int client_callback_print (void *arg, int argc, char **argv)
{
  int i;
  for (i = 0; i < argc; i++)
	{
	  printf ("%s", argv[i]);
	}
	
  return 0;
}

int client_callback_list (void *arg, int argc, char **argv)
{
  int i;
  GSList  **data_list = (GSList **) arg;
 
  for (i = 0; i < argc; i++)
	{
	  *data_list = g_slist_append (*data_list, argv[i]);
	}
	
  return 0;
}

int client_callback_string (void *arg, int argc, char **argv)
{
  int i;
  gchar  **data_str = (gchar **) arg;
 
  for (i = 0; i < argc; i++)
	{
	  if (*data_str == NULL)
	    *data_str = g_malloc0 (sizeof (gchar) * strlen (argv[i]) + 1);
	  else
	  {
	    *data_str = g_realloc (*data_str, strlen (*data_str) + strlen (argv[i]) + 1); 
            fprintf (stderr, "Havelen: %d Adding: %d ", strlen (*data_str), strlen (argv[i]));
	  }

	  *data_str = strcat (*data_str, argv[i]);
	  *data_str = strcat (*data_str, "\0");
	  fprintf (stderr, "= %d \n", strlen (*data_str));
	}
	
  return 0;
}

int client_callback_gstring (void *arg, int argc, char **argv)
{
  int i;
  GString  **data_str = (GString **) arg;
 
  for (i = 0; i < argc; i++)
	{
	  g_string_append (*data_str, argv[i]);
	}
	
  return 0;
}
/*
** The following four routines implement the varargs versions of the
** gpesync_client_exec() and gpesync_client_get_table() interfaces.  See the gpesync_client.h
** header files for a more detailed description of how these interfaces
** work.
**
** These routines are all just simple wrappers.
*/
int gpesync_client_exec_printf(
  gpesync_client *db,                   /* An open database */
  const char *query_format,        /* printf-style format string for the SQL */
  gpesync_client_callback xCallback,    /* Callback function */
  void *pArg,                   /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  ...                           /* Arguments to the format string. */
){
  va_list ap;
  char *buf = NULL;
  int rc;

  va_start(ap, errmsg);
  g_vasprintf (&buf, query_format, ap);
  va_end(ap);

  rc = gpesync_client_exec (db, buf, xCallback, pArg, errmsg);

  free (buf);
  return rc;
}

