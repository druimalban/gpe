/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#include <errno.h>
#include <X11/Xlib.h>

#include <glib.h>

#define XSERVER		"/etc/X11/Xserver"
#define XINIT		"/etc/X11/Xinit"
#define LOGIN		"/usr/bin/gpe-login"

#define SERVER_CRASH_TIME		5
#define SERVER_CRASHING_THRESHOLD	3

static pid_t xserver_pid;
static pid_t xinit_pid;
static pid_t session_pid;

static char *dpyname;
static Display *dpy;

static time_t server_started;

static int server_crashing_count;

#define _(x) gettext (x)

void
start_server (gboolean crashed)
{
  time_t t;
  time (&t);

  if (crashed && (t - server_started < SERVER_CRASH_TIME))
    {
      server_crashing_count++;
      if (server_crashing_count == SERVER_CRASHING_THRESHOLD)
	{
	  fprintf (stderr, _("gpe-dm: X server keeps crashing, giving up\n"));
	  exit (1);
	}
    }

  xserver_pid = fork ();
  if (xserver_pid == 0)
    {
      execl (XSERVER, XSERVER, dpyname, NULL);
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

int
main(int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  daemon (0, 0);

  if (argc == 2)
    dpyname = argv[1];
  else
    dpyname = ":0";

  start_server (FALSE);

  clearenv ();

  setenv ("DISPLAY", dpyname, 1);

  signal (SIGINT, shutdown);
  signal (SIGTERM, shutdown);

  for (;;)
    {
      pid_t wpid;
      gboolean f = FALSE;
      Window win;

      dpy = XOpenDisplay (dpyname);
      if (dpy == NULL)
	continue;

      win = XCreateSimpleWindow(dpy,
				RootWindow (dpy, DefaultScreen (dpy)),
				0, 0,
				0, 0,
				0, BlackPixel(dpy, DefaultScreen (dpy)),
				WhitePixel(dpy, DefaultScreen (dpy)));

      /* start Xinit and wait for it to finish */
      sleep (1);
      xinit_pid = fork ();
      if (xinit_pid == 0)
	{
	  execl (XINIT, XINIT, NULL);
	  _exit (1);
	}
      waitpid (xinit_pid, NULL, 0);

      /* start gpe-login */
      session_pid = fork ();
      if (session_pid == 0)
	{
	  execl (LOGIN, LOGIN, NULL);
	  _exit (1);
	}

      /* wait for news */
      while (!f)
	{
	  wpid = wait (NULL);
	  if (wpid == xserver_pid)
	    {
	      kill (session_pid, SIGTERM);
	      start_server (TRUE);
	      f = TRUE;
	    }
	  else if (wpid == session_pid)
	    {
	      kill (xserver_pid, SIGHUP);
	      f = TRUE;
	    }
	}
    }
}
