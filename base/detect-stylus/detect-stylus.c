/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

int
main (int argc, char *argv[])
{
  char b = 1;
  Atom atom;
  Display *dpy;
  Window root;
  int fd = open ("/dev/touchscreen/0", O_RDONLY);

  if (fd >= 0)
    {
      close (fd);

      if ((dpy = XOpenDisplay (NULL)) == NULL)
	{
	  fprintf (stderr, "Cannot connect to X server.");
	  exit (1);
	}
      
      root = RootWindow (dpy, 0);
      atom = XInternAtom (dpy, "GPE_STYLUS_MODE", 0);
      
      XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, &b, 1);

      XCloseDisplay (dpy);
    }
      
  exit (0);
}
