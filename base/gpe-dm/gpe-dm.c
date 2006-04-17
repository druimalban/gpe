/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <libintl.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <locale.h>

#include <glib.h>

#define XSERVER_NEW	"/etc/X11/X"
#define XSERVER		"/etc/X11/Xserver"
#define XINIT		"/etc/X11/Xinit"

#define SERVER_CRASH_TIME		5
#define SERVER_CRASHING_THRESHOLD	3

#define PIDFILE "/var/run/gpe-dm.pid"

static pid_t xserver_pid;
static pid_t session_pid;

static char *dpyname;

static time_t server_started;

static int server_crashing_count;

#define _(x) gettext (x)

void
start_server (gboolean crashed)
{
  gchar *xserver;
  time_t t;
  time (&t);

  if (crashed && (t - server_started < SERVER_CRASH_TIME))
    {
      server_crashing_count++;
      if (server_crashing_count == SERVER_CRASHING_THRESHOLD)
	{
	  syslog (LOG_DAEMON | LOG_ERR, _("gpe-dm: X server keeps crashing, giving up\n"));
	  exit (1);
	}
    }

  if (access (XSERVER_NEW, X_OK) == 0)
    xserver = XSERVER_NEW;
  else
    xserver = XSERVER;

  xserver_pid = fork ();
  if (xserver_pid == 0)
    {
      int err;
      execl (xserver, xserver, dpyname, "-noreset", NULL);
      err = errno;
      syslog (LOG_DAEMON | LOG_WARNING, _("gpe-dm: couldn't exec %s: %s\n"), xserver, strerror (err));
      _exit (1);
    }
  server_started = t;
}

void
shutdown (int s)
{
  if (xserver_pid)
    kill (xserver_pid, SIGTERM);
      
  if (session_pid)
    kill (session_pid, SIGTERM);

  exit (128 + s);
}

void
create_pidfile (void)
{
  FILE *fd;
    
  fd = fopen (PIDFILE, "w");

  if (!fd)
    {
      fprintf (stderr, "Could not open pidfile '%s'\n", PIDFILE);
      return;
    }

  fprintf (fd, "%d", getpid());
  fclose (fd);
}

void
remove_pidfile (void)
{
  unlink (PIDFILE);
}

int
main(int argc, char *argv[])
{
  gboolean nodaemon = FALSE;
  int i;

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
  
  atexit (remove_pidfile); 
  create_pidfile ();

  dpyname = ":0";

  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == ':')
	dpyname = argv[i];
      else if (!strcmp (argv[i], "-n"))
	nodaemon = TRUE;
    }

  if (! nodaemon)
    daemon (0, 0);

  openlog ("gpe-dm", 0, 0);

  start_server (FALSE);

  clearenv ();

  setenv ("DISPLAY", dpyname, 1);

  signal (SIGINT, shutdown);
  signal (SIGTERM, shutdown);

  for (;;)
    {
      pid_t wpid;
      gboolean f;

      /* start session */
      session_pid = fork ();
      if (session_pid == 0)
	{
	  int err;
	  execl (XINIT, XINIT, NULL);
	  err = errno;
	  syslog (LOG_DAEMON | LOG_WARNING, _("gpe-dm: couldn't exec %s: %s\n"), XINIT, strerror (err));
	  _exit (1);
	}

      for (f = FALSE; f == FALSE; )
	{
	  int status;
	  wpid = wait (&status);
	  if (wpid == xserver_pid)
	    {
	      syslog (LOG_DAEMON | LOG_NOTICE, _("gpe-dm: Xserver pid %d exited with status %d\n"),
		      wpid, WEXITSTATUS (status));
	      kill (session_pid, SIGTERM);
	      start_server (TRUE);
	      f = TRUE;
	    }
	  else if (wpid == session_pid)
	    {
	      syslog (LOG_DAEMON | LOG_INFO, _("gpe-dm: session pid %d exited with status %d\n"),
		      wpid, WEXITSTATUS (status));
	      if (kill (xserver_pid, SIGHUP))
		{
		  int err;
		  err = errno;
		  syslog (LOG_DAEMON | LOG_ERR, _("gpe-dm: server reset %d failed: %s\n"),
			  xserver_pid, strerror (err));
		}
	      sleep (1);
	      f = TRUE;
	    }
	  else
	    syslog (LOG_DAEMON | LOG_WARNING, _("gpe-dm: unknown pid %d exited with status %d\n"),
		    wpid, WEXITSTATUS (status));
	}
    }
}
