/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

static Atom atom;
static Display *dpy;
static Window root;

int
main (int argc, char *argv[])
{
  char b = 1;

  gtk_init (&argc, &argv);
  
  dpy = GDK_DISPLAY ();
  root = RootWindow (dpy, 0);
  atom = XInternAtom (dpy, "GPE_STYLUS_MODE", 0);

  XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, &b, 1);

  exit (0);
}
