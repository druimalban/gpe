/*
 * Copyright (C) 2005 Martin Felis <martin@silef.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Parts of this file are derieved from Phil Blundells libnsqlc.
 * See http://handhelds.org/cgi-bin/cvsweb.cgi/gpe/base/libnsqlc/
 * for more information.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
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
    fprintf (stderr, "[gpsyncclient write_command]: %s\n", buf);

  write (ctx->outfd, buf, strlen (buf));

  free (buf);
}

/*! \brief moves forward to the next line
 *
 * \param data	The string on which we want to move forward.
 *
 * \returns	A pointer to the beginning of the next line
 * 		or on '\0'
 */
char *
get_next_line (const char *data, gsize *len)
{
  GString *string;
 
  if (len)
    *len = 0;

  string = g_string_new (NULL);
  
  while ((data[*len] != '\n') && (data[*len] != '\0'))
  {
    g_string_append_c (string, data[*len]);
    *len += 1;
  }

  if (data[*len] == '\n')
    {
      g_string_append_c (string, data[*len]);
      *len += 1;
    }
  
  if (!string->str || string->str[0] == '\0')
  {
    g_string_free (string, TRUE);
    return FALSE;
  }
  
  return g_string_free (string, FALSE);
}

static void
read_lines (struct gpesync_client_query_context *query_ctx, char *data)
{
  if (!query_ctx->aborting)
    {
      if (verbose)
	fprintf (stderr, "[gpesync_client lines_lines] \n<%s>\n", data);
      GSList *lines = NULL, *iter;

      int argc, i;
      char **argv = NULL;
      char *line, *line_iter;

      /* We need to split the data in the sperate lines, so that
       * we can read it as lists. */
      int count = 0;
      line_iter = data;
      line = get_next_line (line_iter, &count);
      do
        {
	  lines = g_slist_append (lines, line);
	  line_iter += count;
	  line = get_next_line (line_iter, &count);
	}
      while (line);

      argc = g_slist_length (lines);
      argv = g_malloc0 (sizeof (char *) * argc);
      
      iter = lines;
      for (i = 0; i < argc; i++)
	{
	  argv[i] = iter->data;
	  iter = g_slist_next (iter);
	}

      if ( (query_ctx->callback != NULL) && (query_ctx->callback (query_ctx->cb_data, argc, argv)))
	{
	  fprintf (stderr, "aborting query\n");
	  query_ctx->result = GPESYNC_CLIENT_ABORT;
	  query_ctx->aborting = TRUE;
	}

      g_free (argv);
      g_slist_free (lines);
    }
}

static void
read_response (struct gpesync_client_query_context *query_ctx)
{
  int len = 0;
  gboolean have_len = FALSE;
  gpesync_client *ctx;
  GString *buf;

  ctx = query_ctx->ctx;
  buf = g_string_new ("");

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
	      len = atoi (buf->str);
	      have_len = TRUE;
	      g_string_assign (buf, "");
	      continue;
	    }
	}
      else
	{
	  if (buf->len == len - 1)
	    {
	      g_string_append_c (buf, c);
	      break;
	    }
	}

      g_string_append_c (buf, c);

    }

  if (ctx->busy)
    {
      read_lines (query_ctx, buf->str);
      g_string_free (buf, TRUE);
      ctx->busy = 0;
    }
}

gpesync_client *
gpesync_client_open (const char *addr, char **errmsg)
{
  gpesync_client *ctx;
  gchar *hostname = NULL;
  const gchar *username = NULL;
  gchar *str;
  gchar *p;

  int in_fds[2], out_fds[2];
  pid_t pid;

  str = g_strdup (addr);

  p = strchr (str, '@');
  if (p)
    {
      *p = 0;
      hostname = p + 1;
      username = str;
    }
  else
    hostname = str;

  if (hostname == NULL)
    hostname = "localhost";

  if (username == NULL)
    username = g_get_user_name ();

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
      if (verbose)
        fprintf (stderr, "connecting as %s to %s filename: %s\n", username, hostname, "gpesyncd");
      execlp ("ssh", "ssh", "-l", username, hostname, "gpesyncd", "--remote",
	      NULL);
      perror ("exec");
    }

  close (out_fds[0]);
  close (in_fds[1]);

  ctx->outfd = out_fds[1];
  ctx->infd = in_fds[0];

  ctx->hostname = (hostname);
  ctx->username = (username);
 
  g_free (str);
  
  return ctx;
}

void
gpesync_client_close (gpesync_client * ctx)
{
  close (ctx->infd);
  if (ctx->infd != ctx->outfd)
    close (ctx->outfd);

  g_free (ctx);
//  ctx = NULL;
}

int
gpesync_client_exec (gpesync_client * ctx, const char *command,
		 gpesync_client_callback cb, void *cb_data, char **err)
{
  struct gpesync_client_query_context query;
  GString *cmd = g_string_new ("");
  g_string_append_printf (cmd, "%d:%s", strlen (command), command);

  memset (&query, 0, sizeof (query));
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
    *data_list = g_slist_append (*data_list, g_strdup (argv[i]));
    g_free (argv[i]);
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
	  }

	  *data_str = strcat (*data_str, argv[i]);
	  *data_str = strcat (*data_str, "\0");
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

int gpesync_client_exec_printf(
  gpesync_client *db,                   /* A connected client */
  const char *query_format,        /* printf-style format string for query
  	                              to gpesyncd.*/
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

