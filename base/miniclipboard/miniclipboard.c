/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include "mbtray.h"

Display *dpy;
Atom clipboard_atom;
Atom clipboard_selection_atom;

Window win;

XGCValues *gv = NULL;
int screen;
char *buf = NULL;
size_t bufsiz = 0;


static int trapped_error_code;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

static void
gpe_popup_infoprint (Display *dpy, char *s)
{
  Window w;
  static Atom infoprint_atom;
  
  if (infoprint_atom == None)
    infoprint_atom = XInternAtom (dpy, "_GPE_INFOPRINT", False);

  w = XGetSelectionOwner (dpy, infoprint_atom);
  if (w != None)
    {
      XChangeProperty (dpy, w, infoprint_atom, XA_STRING, 8, PropModeReplace,
		       s, strlen (s));
    }
}

static int
error_handler(Display     *display,
	      XErrorEvent *error)
{
   trapped_error_code = error->error_code;
   return 0;
}

static void
trap_errors(void)
{
   trapped_error_code = 0;
   old_error_handler = XSetErrorHandler(error_handler);
}

static int
untrap_errors(void)
{
   XSetErrorHandler(old_error_handler);
   return trapped_error_code;
}

void
selection_clear (void)
{
  XConvertSelection (dpy, clipboard_atom, XA_STRING,
		     clipboard_selection_atom, win, CurrentTime);

  XSync (dpy, False);
}

void
selection_request (XEvent *ev)
{
  XSelectionRequestEvent *rev = (XSelectionRequestEvent *)ev;
  XSelectionEvent sev;
  
  sev.type = SelectionNotify;
  sev.selection = rev->selection;
  sev.requestor = rev->requestor;
  sev.time = rev->time;

  if (rev->target != XA_STRING || rev->selection != clipboard_atom)
    {
      /* Not something I understand */
      sev.property = None;
    }
  else
    {
      XChangeProperty (rev->display, rev->requestor, rev->property,
		       rev->target, 8, PropModeReplace, buf, bufsiz);
      sev.property = rev->property;
    }

  trap_errors ();

  XSendEvent (rev->display, rev->requestor, False, 0, (XEvent *)&sev);
  XSync (dpy, False);

  untrap_errors ();
}

void
selection_notify (XEvent *ev)
{
  XSelectionEvent *sev = (XSelectionEvent *)ev;

  if (sev->property != None)
    {
      Atom actual_type;
      int actual_format;
      unsigned long nitems;
      unsigned long bytes_after;
      unsigned char *prop = NULL;
      int i;
      unsigned long n;

      XGetWindowProperty (dpy, sev->requestor, sev->property,
			  0, 0, False, AnyPropertyType, &actual_type,
			  &actual_format, &nitems, &bytes_after, &prop);
      n = bytes_after;
      XGetWindowProperty (dpy, sev->requestor, sev->property,
			  0, bytes_after, False, AnyPropertyType, &actual_type,
			  &actual_format, &nitems, &bytes_after, &prop);

      if (buf)
	free (buf);
      buf = malloc (n);
      memcpy (buf, prop, n);
      bufsiz = n;
      
      gpe_popup_infoprint (dpy, "Copied");
    }

  XSetSelectionOwner (dpy, clipboard_atom, win, CurrentTime);

  XSync (dpy, False);
}

int 
main (int argc, char *argv[])
{
  char *disp = NULL;

  if ((dpy = XOpenDisplay (disp)) == NULL)
    {
      fprintf (stderr, "Cannot connect to X server on display %s.",
	      XDisplayName (disp));
      exit (1);
    }

  clipboard_atom = XInternAtom (dpy, "CLIPBOARD", False);
  clipboard_selection_atom = XInternAtom (dpy, "_GPE_MINICLIPBOARD_SELECTION", False);

  screen = DefaultScreen (dpy);

  win = XCreateSimpleWindow (dpy, RootWindow (dpy, screen), 0, 0, 1, 1, 0, 0, 0);

  selection_clear ();

  for (;;)
    {
      XEvent an_event;
      XAnyEvent *any;
      struct pix *p = NULL;

      XNextEvent (dpy, &an_event);

      any = (XAnyEvent *)&an_event;

      switch (an_event.type) 
	{
	case SelectionNotify:
	  selection_notify (&an_event);
	  break;

	case SelectionRequest:
	  selection_request (&an_event);
	  break;

	case SelectionClear:
	  selection_clear ();
	  break;
	}
    }
}
