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
#include <netdb.h>

#include <sasl/sasl.h>

#include <glib.h>

#include "common.h"

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
  sasl_conn_t *conn;
  
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

/* do the sasl negotiation; return -1 if it fails */
int mysasl_negotiate(FILE *in, FILE *out, sasl_conn_t *conn)
{
  char buf[8192];
  char chosenmech[128];
  const char *data;
  int len;
  int r;
  const char *userid;
  int count;
    
  /* generate the capability list */

  dprintf(1, "generating client mechanism list... ");
  r = sasl_listmech (conn, NULL, NULL, " ", NULL,
		     &data, &len, &count);
  if (r != SASL_OK) 
    saslfail (r, "generating mechanism list");
  dprintf(1, "%d mechanisms\n", count);

  /* send capability list to client */
  send_string (out, data, len);

  dprintf(1, "waiting for client mechanism...\n");
  len = recv_string (in, chosenmech, sizeof chosenmech);
  if (len <= 0) 
    {
      printf("client didn't choose mechanism\n");
      fputc('N', out); /* send NO to client */
      fflush (out);
      return -1;
    }

  /* receive initial response (if any) */
  len = recv_string(in, buf, sizeof (buf));

  /* start libsasl negotiation */
  r = sasl_server_start(conn, chosenmech, buf, len,
			&data, &len);
  if (r != SASL_OK && r != SASL_CONTINUE) 
    {
      saslerr(r, "starting SASL negotiation");
      fputc('N', out); /* send NO to client */
      fflush(out);
      return -1;
    }
  
  while (r == SASL_CONTINUE) 
    {
      if (data) 
	{
	  dprintf(2, "sending response length %d...\n", len);
	  fputc('C', out); /* send CONTINUE to client */
	  send_string(out, data, len);
	  free(data);
	} 
      else 
	{
	  dprintf(2, "sending null response...\n");
	  fputc('C', out); /* send CONTINUE to client */
	  send_string(out, "", 0);
	}
      
      dprintf(1, "waiting for client reply...\n");
      len = recv_string(in, buf, sizeof buf);
      if (len < 0) {
	printf("client disconnected\n");
	return -1;
      }
      
      r = sasl_server_step(conn, buf, len, &data, &len);
      if (r != SASL_OK && r != SASL_CONTINUE) {
	saslerr(r, "performing SASL negotiation");
	fputc('N', out); /* send NO to client */
	fflush(out);
	return -1;
      }
    }
  
  if (r != SASL_OK) {
    saslerr(r, "incorrect authentication");
    fputc('N', out); /* send NO to client */
    fflush(out);
    return -1;
  }
  
  fputc ('O', out); /* send OK to client */
  fflush (out);
  dprintf (1, "negotiation complete\n");
  if (data)
    free(data);
  
  r = sasl_getprop (conn, SASL_USERNAME, (void **)&userid);
  printf("successful authentication '%s'\n", userid);
  
  return 0;
}

int
main_loop (int fd)
{
  struct nsql_context *ctx;
  struct sockaddr_storage local_ip, remote_ip;
  socklen_t salen;
  int fd2;
  int r;
  sasl_security_properties_t secprops;
  char localaddr[NI_MAXHOST | NI_MAXSERV],
    remoteaddr[NI_MAXHOST | NI_MAXSERV];
  char myhostname[1024+1];
  char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];

  fd2 = dup (fd);
  if (fd2 < 0)
    {
      perror ("dup");
      return errno;
    }
  
  ctx = g_malloc0 (sizeof (*ctx));

  ctx->ifp = fdopen (fd, "r");
  ctx->ofp = fdopen (fd2, "w");

  r = gethostname(myhostname, sizeof(myhostname)-1);
  if(r == -1) saslfail(r, "getting hostname");

  /* set ip addresses */
  salen = sizeof(local_ip);
  if (getsockname(fd, (struct sockaddr *)&local_ip, &salen) < 0) {
    perror("getsockname");
  }
  getnameinfo((struct sockaddr *)&local_ip, salen,
	      hbuf, sizeof(hbuf), pbuf, sizeof(pbuf),
	      NI_NUMERICHOST | NI_NUMERICSERV);
  snprintf(localaddr, sizeof(localaddr), "%s;%s", hbuf, pbuf);

  salen = sizeof(remote_ip);
  if (getpeername(fd, (struct sockaddr *)&remote_ip, &salen) < 0) {
    perror("getpeername");
  }
  getnameinfo((struct sockaddr *)&remote_ip, salen,
	      hbuf, sizeof(hbuf), pbuf, sizeof(pbuf),
	      NI_NUMERICHOST | NI_NUMERICSERV);
  snprintf(remoteaddr, sizeof(remoteaddr), "%s;%s", hbuf, pbuf);

  r = sasl_server_new ("nsql", myhostname, NULL, localaddr, remoteaddr,
		       NULL, 0, &ctx->conn);
  if (r != SASL_OK) 
    saslfail (r, "allocating connection state");

  memset (&secprops, 0, sizeof (secprops));
  secprops.min_ssf = 0;
  secprops.max_ssf = 1024;
  secprops.maxbufsize = 1024;  
  secprops.property_names = NULL;
  secprops.property_values = NULL;
  secprops.security_flags = 0;
  
  r = sasl_setprop (ctx->conn, SASL_SEC_PROPS, &secprops);
  if (r != SASL_OK) saslfail(r, "setting security props");

  r = mysasl_negotiate (ctx->ifp, ctx->ofp, ctx->conn);
  if (r != SASL_OK) saslfail(r, "SASL negotiation");

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

static int
getuser(void *context __attribute__((unused)),
	char ** path) 
{
  return SASL_OK;
}

static int
getpasswd(void *context __attribute__((unused)),
	char ** path) 
{
  return SASL_OK;
}

static sasl_callback_t callbacks[] = {
  {
    SASL_CB_USER, &getuser, NULL      /* we'll just use an interaction if this comes up */
  },
  { 
    SASL_CB_PASS, &getpasswd, NULL      /* Call getsecret_func if need secret */
  }, 
  {
    SASL_CB_LIST_END, NULL, NULL
  }
};

int
main (int argc, char *argv[])
{
  int sock;
  struct sockaddr_in6 sin;
  int sopt;
  int r;

  signal (SIGCHLD, SIG_IGN);

  sock = socket (AF_INET6, SOCK_STREAM, 0);    

  /* Initialize SASL */
  r = sasl_server_init (callbacks,      /* Callbacks supported */
			"nsqld");  /* Name of the application */

  if (r != SASL_OK) 
    saslfail (r, "initializing libsasl");

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
