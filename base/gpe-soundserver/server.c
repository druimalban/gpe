/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <X11/Xlib.h>

pid_t pid;

void
quit ()
{
  kill (pid, SIGTERM);
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

  pid = fork ();
  if (pid == 0)
    {
      execlp ("esd", "esd", "-nobeeps", NULL);
      perror ("exec");
      exit (1);
    }

  atexit (quit);

  for (;;)
    XNextEvent (dpy, &ev);
}
