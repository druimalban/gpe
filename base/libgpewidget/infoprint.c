/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
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

#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/infoprint.h>

void
gpe_popup_infoprint (Display *dpy, char *s)
{
  Window w;
  static Atom infoprint_atom;
   
  if (infoprint_atom == None)
    infoprint_atom = XInternAtom (dpy, "_GPE_INFOPRINT", False);
 
  w = XGetSelectionOwner (dpy, infoprint_atom);
  if (w != None)
    {
      XChangeProperty (dpy, w, infoprint_atom, XA_STRING, 8,
	 PropModeReplace, s, strlen (s));
    }
  /* else infoprints not available */
}
