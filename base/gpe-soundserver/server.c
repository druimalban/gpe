/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <X11/Xlib.h>

pid_t pid;

void
quit ()
{
  kill (pid, SIGTERM);
}

void
child_dead ()
{
  int rc = wait (NULL);
  exit (rc);
}

void
fatal_signal (int sig)
{
  quit ();
  exit (128 + sig);
}

int
main ()
{
  Display *dpy = XOpenDisplay (NULL);
  XEvent ev;

  if (dpy == NULL)
    {
      fprintf (stderr, "Unable to connect to X display\n");
      exit (1);
    }

  signal (SIGCHLD, child_dead);
  pid = vfork ();
  if (pid == 0)
    {
      execlp ("wrapesd", "esd", "-nobeeps", "-as", "5", NULL);
      perror ("exec");
      _exit (1);
    }

  signal (SIGTERM, fatal_signal);
  signal (SIGINT, fatal_signal);
  signal (SIGQUIT, fatal_signal);

  atexit (quit);

  for (;;)
    XNextEvent (dpy, &ev);
}
