/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
      unsigned char *prop = NULL;
      Atom type;
      int format;
      unsigned long nitems;
      unsigned long bytes;
      
      dpy = GDK_DISPLAY ();
      root = RootWindow (dpy, 0);
      atom = XInternAtom (dpy, "_GPE_STYLUS_MODE", True);

      stylus_mode_flag = NO;

      if (atom)
	{
	  XGetWindowProperty (dpy, root, atom, 0, 1, 0, XA_INTEGER,
			      &type, &format, &nitems, &bytes,
			      &prop);
      
	  if (nitems)
	    stylus_mode_flag = YES;

	  if (prop)
	    XFree (prop);
	}

      if (getenv ("GPE_PRETEND_STYLUS") != NULL)
	stylus_mode_flag = YES;
    }

  return stylus_mode_flag == YES;
}
