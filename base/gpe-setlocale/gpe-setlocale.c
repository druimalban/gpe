/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

int
main (int argc, char *argv[])
{
  Atom atom;
  Display *dpy;
  Window root;
  char *locale;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s <locale>\n", argv[0]);
      exit (1);
    }

  if ((dpy = XOpenDisplay (NULL)) == NULL)
    {
      fprintf (stderr, "Cannot connect to X server.");
	  exit (1);
    }
  
  root = RootWindow (dpy, 0);
  atom = XInternAtom (dpy, "_GPE_LOCALE", 0);
  
  locale = argv[1];

  XChangeProperty (dpy, root, atom, XA_STRING, 8, PropModeReplace, locale, strlen (locale));

  XCloseDisplay (dpy);
      
  exit (0);
}
