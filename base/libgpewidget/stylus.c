/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define YES 1
#define NO 2

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static int stylus_mode_flag;

gboolean
gpe_stylus_mode (void)
{
  if (! stylus_mode_flag)
    {
      Display *dpy;
      Window root;
      Atom atom;
      unsigned char *prop;
      Atom type;
      int format;
      unsigned long nitems;
      unsigned long bytes;
      
      dpy = GDK_DISPLAY ();
      root = RootWindow (dpy, 0);
      atom = XInternAtom (dpy, "GPE_STYLUS_MODE", 0);

      XGetWindowProperty (dpy, root, atom, 0, 1, 0, XA_INTEGER,
			  &type, &format, &nitems, &bytes,
			  &prop);
      
      stylus_mode_flag = nitems ? YES : NO;
    }

  return stylus_mode_flag == YES;
}
