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
#include <sqlite.h>

#include <glib.h>

#define PORT 6666
#define BUFFER_LENGTH 2048

#define NSQLD_ERROR nsqld_error_quark ()

enum
  {
    NSQLD_ERROR_BAD_ARGS,
    NSQLD_ERROR_SQL,
    NSQLD_ERROR_NO_DB,
    NSQLD_ERROR_BAD_COMMAND
  };

struct nsql_context
{
  FILE *ifp, *ofp;
  int quit;
  sqlite *sqliteh;
  
  char *cmd_id;
};

GQuark
nsqld_error_quark (void)
{
  return g_quark_from_static_string ("NSQLD");
}

void
nsqld_printf (struct nsql_context *ctx, char *format, ...)
{
  va_list ap;
  va_start (ap, format);

  vfprintf (ctx->ofp, format, ap);
  fflush (ctx->ofp);

  va_end (ap);
}

void
puts_encoded (char *s, FILE *fp)
{
  while (*s)
    {
      char c = *s++;

      if (!isprint (c) || isspace (c) || c == '%')
	fprintf (fp, "%%%02x", c);
      else
	fputc (c, fp);
    }
}

gboolean
do_open (struct nsql_context *ctx, char *cmdline, GError **error)
{
  char *err;

  while (*cmdline && isspace (*cmdline))
    cmdline++;

  if (*cmdline == 0)
    {
      g_set_error (error, NSQLD_ERROR,
		   NSQLD_ERROR_BAD_ARGS,
		   "Bad arguments");
      return FALSE;
    }

  if (ctx->sqliteh)
    sqlite_close (ctx->sqliteh);

  ctx->sqliteh = sqlite_open (cmdline, 0, &err);
  if (ctx->sqliteh == NULL)
    {
      g_set_error (error, NSQLD_ERROR, NSQLD_ERROR_SQL, "%s", err);
      free (err);
      return FALSE;
    }

  return TRUE;
}

gboolean
dot_command (struct nsql_context *ctx, char *cmd, GError **error)
{
  if (!strcasecmp (cmd, "quit"))
    {
      ctx->quit = TRUE;
      return TRUE;
    }
  else if (!strncasecmp (cmd, "open ", 5))
    return do_open (ctx, cmd + 5, error);

  g_set_error (error, NSQLD_ERROR, NSQLD_ERROR_BAD_COMMAND,
	       "Command '%s' not understood", cmd);

  return FALSE;
}

int
data_callback (void *arg, int argc, char **argv, char **names)
{
  struct nsql_context *ctx;
  int i;
  
  ctx = (struct nsql_context *)arg;

  fprintf (ctx->ofp, "%s - %d", ctx->cmd_id, argc);

  for (i = 0; i < argc; i++)
    {
      fputc (' ', ctx->ofp);
      puts_encoded (argv[i], ctx->ofp);
    }

  fprintf (ctx->ofp, "\n");
  fflush (ctx->ofp);

  return 0;
}

gboolean
do_command (struct nsql_context *ctx, char *cmd, GError **error)
{
  char *err;

  if (cmd[0] == '.')
    return dot_command (ctx, cmd + 1, error);
  
  if (ctx->sqliteh == NULL)
    {
      g_set_error (error, NSQLD_ERROR, NSQLD_ERROR_NO_DB, "No database open");
      return FALSE;
    }
  
  if (sqlite_exec (ctx->sqliteh, cmd, data_callback, ctx, &err))
    {
      g_set_error (error, NSQLD_ERROR, NSQLD_ERROR_SQL, "%s", err);
      free (err);
      return FALSE;
    }
  
  return TRUE;
}

gboolean
process_line (struct nsql_context *ctx, char *line)
{
  int p;
  char *id, *cmd;
  GError *error = NULL;
  int rc;

  while (isspace (*line))
    line++;

  p = strlen (line);
  while (p > 0 && isspace(line[p - 1]))
    {
      line[p - 1] = 0;
      p--;
    }

  if (line[0] == 0)
    return TRUE;

  id = line;
  while (*line && !isspace (*line))
    line++;
  if (*line == 0)
    goto bad_input;
  *line++ = 0;
  while (*line && isspace (*line))
    line++;
  if (*line == 0)
    goto bad_input;
  cmd = line;

  ctx->cmd_id = id;

  rc = do_command (ctx, cmd, &error);
  if (rc)
    {
      nsqld_printf (ctx, "%s + ok\n", ctx->cmd_id);
    }
  else
    {
      nsqld_printf (ctx, "%s ! %s\n", ctx->cmd_id, error->message);
      g_clear_error (&error);
    }

  ctx->cmd_id = NULL;
  
  return TRUE;

 bad_input:
  nsqld_printf (ctx, "! ! Bad input\n");
  return FALSE;
}

int
main_loop (int fd)
{
  struct nsql_context *ctx;
  int fd2;

  fd2 = dup (fd);
  if (fd2 < 0)
    {
      perror ("dup");
      return errno;
    }
  
  ctx = g_malloc0 (sizeof (*ctx));

  ctx->ifp = fdopen (fd, "r");
  ctx->ofp = fdopen (fd2, "w");

  while (!ctx->quit && !feof (ctx->ifp))
    {
      char line[BUFFER_LENGTH];

      if (fgets (line, BUFFER_LENGTH, ctx->ifp))
	process_line (ctx, line);
    }

  fclose (ctx->ifp);
  fclose (ctx->ofp);

  g_free (ctx);

  return 0;
}

void
new_connection (int fd, struct sockaddr *sin, socklen_t slen)
{
  if (fork () == 0)
    exit (main_loop (fd));
  
  close (fd);
}

int
main (int argc, char *argv[])
{
  int sock;
  struct sockaddr_in6 sin;
  int sopt;

  signal (SIGCHLD, SIG_IGN);

  sock = socket (AF_INET6, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (1);
    }
  
  sopt = 1;
  if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sopt, sizeof (sopt)))
    {
      perror ("setsockopt");
      exit (1);
    }

  memset (&sin, 0, sizeof (sin));
  sin.sin6_port = htons(PORT);

  if (bind (sock, (struct sockaddr *)&sin, sizeof (sin)))
    {
      perror ("bind");
      exit (1);
    }

  if (listen (sock, 10))
    {
      perror ("listen");
      exit (1);
    }
  
  for (;;)
    {
      int newfd;
      socklen_t slen;

      newfd = accept (sock, (struct sockaddr *)&sin, &slen);
      if (newfd < 0)
	{
	  if (errno == ENETDOWN || errno == EPROTO || errno == ENOPROTOOPT
	      || errno == EHOSTDOWN || errno == ENONET || errno == EHOSTUNREACH
	      || errno == EOPNOTSUPP || errno == ENETUNREACH)
	    continue;

	  perror ("accept");
	  exit (1);
	}

      new_connection (newfd, (struct sockaddr *)&sin, slen);
    }
}
