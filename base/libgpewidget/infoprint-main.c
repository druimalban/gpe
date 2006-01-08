/*
 * Copyright (C) 2005 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "infoprint.h"

int
main (int argc, char *argv[])
{
  Display *dpy;

  if (argc != 2) 
    {
      fprintf (stderr, "usage: %s <message>\n", argv[0]);
      exit (1);
    }

  dpy = XOpenDisplay (NULL);
  
  gpe_popup_infoprint (dpy, argv[1]);

  XFlush (dpy);
  
  exit (0);
}
