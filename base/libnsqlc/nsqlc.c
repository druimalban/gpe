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
  int fd;
  int seq;

  int busy;
  gchar *error;
  
  const gchar *hostname;
  const gchar *username;
  const gchar *filename;
};

int verbose = 1;

static void
write_command (nsqlc *ctx, const char *cmd, ...)
{
  char *buf, *buf2;
  va_list va;

  va_start (va, cmd);
  vasprintf (&buf, cmd, va);
  va_end (va);

  buf2 = g_strdup_printf ("%08x %s\r\n", ctx->seq++, buf);

  if (verbose)
    fprintf (stderr, "-> %s\n", buf);

  free (buf);

  write (ctx->fd, buf2, strlen (buf2));

  g_free (buf2);
}

static void
read_response (nsqlc *ctx)
{
  char buf[1024];
  int p = 0;

  for (;;)
    {
      int rc;
      char c;

      rc = read (ctx->fd, &c, 1);
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
      else if (*p == '!')
	{
	  ctx->busy = 0;
	  ctx->error = strdup (p);
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

  ctx->fd = fd;
  ctx->hostname = g_strdup (hostname);
  ctx->username = g_strdup (username);
  ctx->filename = g_strdup (filename);

  ctx->busy = 1;

  write_command (ctx, ".open %s", filename);

  while (ctx->busy)
    read_response (ctx);

  return ctx;
}

void
nsqlc_close (nsqlc *ctx)
{
  close (ctx->fd);

  g_free (ctx);
}

int 
nsqlc_exec (nsqlc *ctx, const char *sql, nsqlc_callback cb, void *cb_data, char **err)
{
  ctx->error = NULL;
  ctx->busy = 1;

  write_command (ctx, sql);

  while (ctx->busy)
    read_response (ctx);

  if (ctx->error)
    {
      *err = ctx->error;
      return 1;
    }

  return 0;
}
