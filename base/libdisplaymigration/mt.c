/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdio.h>

char *host;
int screen;
Display *dpy;
Atom migrate_ok_atom;
Atom migrate_atom;
Atom string_atom;

void
send_message (Display *dpy, Window w, char *host, int screen)
{
  char buf[256];
  buf[0] = screen;
  strcpy (buf + 1, host);
  
  XChangeProperty (dpy, w, migrate_atom, string_atom, 8, PropModeReplace, buf, strlen (host) + 1);
}

static int
handle_click (Window w)
{
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  XClientMessageEvent event;

  if (XGetWindowProperty (dpy, w, migrate_ok_atom, 0, 0, False,
			  None, &type, &format, &nitems, &bytes_after, &prop) != Success
      || type == None)
    {
      Window root, parent, *children;
      unsigned int nchildren;  
  
      XQueryTree (dpy, w, &root, &parent, &children, &nchildren);
      if (children)
	XFree (children);

      if (root != w && root != parent)
	return handle_click (parent);

      return None;
    }

  if (prop)
    XFree (prop);

  return w;
}

static Window 
find_deepest_window (Display *dpy, Window grandfather, Window parent,
		     int x, int y, int *rx, int *ry)
{
  int dest_x, dest_y;
  Window child;
  
  XTranslateCoordinates (dpy, grandfather, parent, x, y,
			 &dest_x, &dest_y, &child);

  if (child == None)
    {
      *rx = dest_x;
      *ry = dest_y;

      return parent;
    }
  
  return find_deepest_window(dpy, parent, child, dest_x, dest_y, rx, ry);
}

int
main (int argc, char *argv[])
{
  int done = 0;

  if (argc < 3)
    {
      fprintf (stderr, "usage: %s <host> <screen>\n", argv[0]);
      exit (1);
    }

  host = argv[1];
  screen = atoi (argv[2]);

  dpy = XOpenDisplay (NULL);
  if (!dpy)
    {
      fprintf (stderr, "Couldn't open display\n");
      exit (1);
    }

  string_atom = XInternAtom (dpy, "STRING", False);
  migrate_ok_atom = XInternAtom (dpy, "_GPE_DISPLAY_CHANGE_OK", False);
  migrate_atom = XInternAtom (dpy, "_GPE_DISPLAY_CHANGE", False);

  XGrabPointer (dpy, RootWindow (dpy, 0), False, ButtonPressMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync,
		None, None, CurrentTime);

  while (!done)
    {
      XEvent ev;
      Window w = None;
      int dest_x, dest_y;

      XNextEvent (dpy, &ev);
      
      switch (ev.type)
	{
	case ButtonRelease:
	  w = find_deepest_window (dpy, DefaultRootWindow (dpy), DefaultRootWindow (dpy),
				   ev.xbutton.x, ev.xbutton.y, &dest_x, &dest_y);
	  
	  XUngrabPointer (dpy, ev.xbutton.time);

	  w = handle_click (w);

	  if (w == None)
	    fprintf (stderr, "Window not migration capable\n");
	  else
	    send_message (dpy, w, host, screen);

	  done = 1;

	  break;
	}
    }

  exit (0);
}
