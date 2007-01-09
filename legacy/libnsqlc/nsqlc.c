/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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
#include <ctype.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <glib.h>
#include <stdarg.h>

#include "nsqlc.h"

struct nsqlc
{
  int infd;
  int outfd;
  int seq;

  int busy;
  
  const gchar *hostname;
  const gchar *username;
  const gchar *filename;
};

#define TYPE_SQL	1
#define TYPE_TIME	2
#define TYPE_LAST_ID	3

struct nsqlc_query_context
{
  struct nsqlc *ctx;

  int type;

  nsqlc_callback callback;
  void *cb_data;
  char **column_names;

  int result;
  gchar *error;
  gboolean aborting;
};

int verbose = 1;

void
write_str_encoded (char *s, int fd)
{
  char buf[3];
  while (*s)
    {
      char c = *s++;

     if (!isprint (c) || isspace (c) || c == '%')
	 {
	   sprintf (buf, "%%%02x", c);
	   write(fd,buf,3);
	 }
      else
	   write (fd, &c, 1);
    }
	sprintf(buf,"\r\n");
	write (fd, buf, 2);
}

static void
write_command (nsqlc *ctx, const char *cmd, ...)
{
  char *buf, *buf2;
  va_list va;

  va_start (va, cmd);
  vasprintf (&buf, cmd, va);
  va_end (va);

  buf2 = g_strdup_printf ("%08x %s", ctx->seq++, buf);

  if (verbose)
    fprintf (stderr, "-> %s\n", buf);

  free (buf);

  write_str_encoded(buf2,ctx->outfd);
  g_free (buf2);
}

static char *
decode_string (unsigned char *c)
{
  unsigned char *r, *p;

  r = g_malloc (strlen (c) + 1);
  p = r;

  while (*c)
    {
      if (*c == '%')
	{
	  char buf[3];
	  unsigned long r;
 
	  c++;
	  buf[0] = *c++;
	  if (buf[0] == 0)
	    break;
	  buf[1] = *c;
	  if (buf[1] == 0)
	    break;
	  buf[2] = 0;
	  
	  r = strtoul (buf, NULL, 16);
	  *p = (unsigned char)r;
	}
      else
	*p = *c;

      c++;
      p++;
    }

  *p = 0;

  return r;
}

static void
time_line (struct nsqlc_query_context *q, char *p)
{
  time_t *t = (time_t *)q->cb_data;
  int i;

  sscanf (p, "%d", &i);
  
  *t = i;
}

static void
data_line (struct nsqlc_query_context *q, char *p)
{
  if (! q->aborting)
    {
      int argc;
      char **argv;
      int i;
      
      while (isspace (*p))
	p++;

      argc = strtoul (p, &p, 10);

      argv = g_malloc0 (argc * sizeof (char *));

      for (i = 0; i < argc; i++)
	{
	  char *s;
	  
	  p++;

	  s = strchr (p, ' ');
	  if (s)
	    *s = 0;

	  argv[i] = decode_string (p);

	  if (s)
	    p = s;
	  else
	    break;
	}

      if (q->callback (q->cb_data, argc, argv, q->column_names))
	{
	  fprintf (stderr, "aborting query\n");
	  q->result = NSQLC_ABORT;
	  q->aborting = TRUE;
	}

      for (i = 0; i < argc; i++)
	g_free (argv[i]);

      g_free (argv);
    }
}

static void
read_response (struct nsqlc_query_context *q)
{
  char buf[1024];
  int p = 0;
  nsqlc *ctx;

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
      
      if (c == 10)
	{
	  buf[p] = 0;
	  break;
	}

      buf[p] = c;
      if (p < (sizeof (buf) - 1))
	p++;
    }

  if (ctx->busy)
    {
      char *p;

      if (verbose)
	fprintf (stderr, "<- %s\n", buf);

      p = strchr (buf, ' ');
      while (isspace (*p))
	p++;

      if (*p == '+')
	ctx->busy = 0;
      else if (*p == '-')
	{
	  if (q->type == TYPE_SQL)
	    data_line (q, p + 1);
	  else if (q->type == TYPE_TIME || q->type == TYPE_LAST_ID)
	    time_line (q, p + 1);
	}
      else if (*p == '!')
	{
	  ctx->busy = 0;
	  q->error = strdup (p);
	  q->result = 1;
	}
      else
	{
	  fprintf (stderr, "unknown response: %s\n", buf);
	}
    }
}

nsqlc *
nsqlc_open (const char *database, int mode, char **errmsg)
{
  nsqlc *ctx;
  gchar *filename;
  gchar *hostname = NULL;
  const gchar *username = NULL;
  gchar *str;
  gchar *p;
  struct addrinfo hints, *ai, *aip;
  int fd = -1;
  int rc;
  struct nsqlc_query_context query;

  str = g_strdup (database);

  p = strchr (str, ':');
  if (p)
    {
      *p = 0;
      filename = p + 1;
      p = strchr (str, '@');
      if (0)
	{
	  *p = 0;
	  hostname = p + 1;
	  username = str;
	}
      else
	hostname = str;
    }
  else
    filename = str;

  if (hostname == NULL)
    hostname = "localhost";

  if (username == NULL)
    username = g_get_user_name ();

  memset (&hints, 0, sizeof (hints));
  hints.ai_socktype = SOCK_STREAM;

  rc = getaddrinfo (hostname, "6666", &hints, &ai);
  g_free (str);
  if (rc)
    {
      *errmsg = strdup (gai_strerror (rc));
      return NULL;
    }

  for (aip = ai; aip; aip = aip->ai_next)
    {
      fd = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
      if (fd < 0)
	continue;

      if (connect (fd, ai->ai_addr, ai->ai_addrlen) == 0)
	break;

      close (fd);
      fd = -1;
    }

  freeaddrinfo (ai);

  if (fd == -1)
    {
      *errmsg = strdup ("Unable to connect");
      return NULL;
    }

  ctx = g_malloc0 (sizeof (struct nsqlc));

  ctx->infd = ctx->outfd = fd;
  ctx->hostname = g_strdup (hostname);
  ctx->username = g_strdup (username);
  ctx->filename = g_strdup (filename);

  ctx->busy = 1;

  write_command (ctx, ".open %s", filename);

  memset (&query, 0, sizeof (query));
  query.ctx = ctx;

  while (ctx->busy)
    read_response (&query);

  if (query.result)
    {
      nsqlc_close (ctx);

      if (*errmsg)
	*errmsg = query.error;

      return NULL;
    }

  return ctx;
}

nsqlc *
nsqlc_open_ssh (const char *database, int mode, char **errmsg)
{
  nsqlc *ctx;
  gchar *filename;
  gchar *hostname = NULL;
  const gchar *username = NULL;
  gchar *str;
  gchar *p;
  struct nsqlc_query_context query;
  int in_fds[2], out_fds[2];
  pid_t pid;

  str = g_strdup (database);

  p = strchr (str, ':');
  if (p)
    {
      *p = 0;
      filename = p + 1;
      p = strchr (str, '@');
      if (0)
	{
	  *p = 0;
	  hostname = p + 1;
	  username = str;
	}
      else
	hostname = str;
    }
  else
    filename = str;

  if (hostname == NULL)
    hostname = "localhost";

  if (username == NULL)
    username = g_get_user_name ();

  ctx = g_malloc0 (sizeof (struct nsqlc));

  pipe (in_fds);
  pipe (out_fds);

  pid = fork ();
  if (pid == 0)
    {
      dup2 (out_fds[0], 0);
      dup2 (in_fds[1], 1);
      close (out_fds[1]);
      close (in_fds[0]);
      execlp ("ssh", "ssh", "-l", username, hostname, "nsqld", "--stdout", NULL);
      perror ("exec");
    }

  close (out_fds[0]);
  close (in_fds[1]);

  ctx->outfd = out_fds[1];
  ctx->infd = in_fds[0];

  ctx->hostname = g_strdup (hostname);
  ctx->username = g_strdup (username);
  ctx->filename = g_strdup (filename);

  ctx->busy = 1;

  write_command (ctx, ".open %s", filename);

  memset (&query, 0, sizeof (query));
  query.ctx = ctx;

  while (ctx->busy)
    read_response (&query);

  if (query.result)
    {
      nsqlc_close (ctx);

      if (*errmsg)
	*errmsg = query.error;

      return NULL;
    }

  return ctx;
}

void
nsqlc_close (nsqlc *ctx)
{
  close (ctx->infd);
  if (ctx->infd != ctx->outfd)
    close (ctx->outfd);

  g_free (ctx);
}

int 
nsqlc_exec (nsqlc *ctx, const char *sql, nsqlc_callback cb, void *cb_data, char **err)
{
  struct nsqlc_query_context query;

  memset (&query, 0, sizeof (query));

  query.ctx = ctx;
  query.callback = cb;
  query.cb_data = cb_data;
  query.aborting = FALSE;
  query.result = 0;
  query.error = NULL;
  query.type = TYPE_SQL;

  ctx->busy = 1;

  write_command (ctx, sql);

  while (ctx->busy)
    read_response (&query);

  if (err)
    *err = query.error;

  return query.result;
}

int
nsqlc_get_time (nsqlc *ctx, time_t *t, char **err)
{
  struct nsqlc_query_context query;

  memset (&query, 0, sizeof (query));

  query.ctx = ctx;
  query.aborting = FALSE;
  query.result = 0;
  query.error = NULL;
  query.type = TYPE_TIME;
  query.cb_data = t;

  ctx->busy = 1;

  write_command (ctx, ".time");

  while (ctx->busy)
    read_response (&query);
					       
  return query.result;
}

int 
nsqlc_last_insert_rowid (nsqlc *ctx)
{
  struct nsqlc_query_context query;
  int i;

  memset (&query, 0, sizeof (query));

  query.ctx = ctx;
  query.aborting = FALSE;
  query.result = 0;
  query.error = NULL;
  query.type = TYPE_LAST_ID;
  query.cb_data = &i;

  ctx->busy = 1;

  write_command (ctx, ".lastid");

  while (ctx->busy)
    read_response (&query);
					       
  return i;
}
