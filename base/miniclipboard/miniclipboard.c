#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

Display *dpy;
Atom window_type_atom;
Atom window_type_dock_atom;
Atom string_atom;
Atom primary_selection_atom;
Atom clipboard_atom;
Atom clipboard_selection_atom;
Atom delete_atom;
Atom active_window_atom;

XGCValues *gv = NULL;
int screen;
char *buf = NULL;
size_t bufsiz = 0;

struct pix
{
  char *name;
  void (*click)(XEvent *);
  Pixmap pix, mask;
  XpmAttributes attrib;
  Window win;
  GC gc;
};

void copy_click (XEvent *);
void paste_click (XEvent *);

struct pix copy = { PREFIX "/share/pixmaps/copy.xpm", copy_click },
  paste = { PREFIX "/share/pixmaps/paste.xpm", paste_click };

void
load_icon (struct pix *p)
{
  p->win = XCreateSimpleWindow (dpy,
				RootWindow (dpy, screen),
				0, 0,
				32, 16,
				0, BlackPixel (dpy, screen),
				WhitePixel (dpy, screen));
  
  XChangeProperty (dpy, p->win, window_type_atom, XA_ATOM, 32, 
		   PropModeReplace, (unsigned char *)
		   &window_type_dock_atom, 1);

  p->gc = XCreateGC (dpy, RootWindow(dpy, screen), 0, gv);

  p->attrib.valuemask = XpmCloseness;
  p->attrib.closeness = 40000;

  if (XpmReadFileToPixmap (dpy, p->win, p->name,
			   &p->pix, &p->mask, &p->attrib) != XpmSuccess )
    {
      fprintf (stderr, "failed loading image '%s'\n",
	       p->name);
      exit(1);
    }

  XResizeWindow (dpy, p->win, p->attrib.width, p->attrib.height);
  XCopyArea (dpy, p->pix, p->win, p->gc, 0, 0,
	     p->attrib.width, p->attrib.height, 0, 0);
  XShapeCombineMask (dpy, p->win, ShapeBounding, 0, 0, p->mask, ShapeSet);
  
  XSelectInput (dpy, p->win, 
		StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask ); 
}

void
copy_click (XEvent *ev)
{
  XConvertSelection (dpy, primary_selection_atom, string_atom, 
		     clipboard_selection_atom, copy.win, ev->xbutton.time);
}

void
paste_click (XEvent *ev)
{
  Window focus;
  Atom actual_type;
  unsigned long nitems;
  unsigned long bytes_after;
  int actual_format;
  XButtonEvent evb;
  int ret;
  Window root;

  memset (&evb, 0, sizeof (evb));
  
  XGetInputFocus (dpy, &focus, &ret);

  evb.root = RootWindow (dpy, screen);
  evb.display = dpy;
  evb.window = focus;
  evb.button = 2;
  evb.time = CurrentTime;
  evb.same_screen = True;

  evb.type = ButtonPress;
  XSendEvent (dpy, focus, True, 0, (XEvent *)&evb);
  evb.state = Button2Mask;
  evb.type = ButtonRelease;
  XSendEvent (dpy, focus, True, 0, (XEvent *)&evb);
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

  if (rev->target != string_atom || rev->selection != clipboard_atom)
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

  XSendEvent (rev->display, rev->requestor, False, 0, (XEvent *)&sev);
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

      XConvertSelection (dpy, primary_selection_atom, delete_atom, 
			 clipboard_selection_atom, copy.win, CurrentTime);

      XSetSelectionOwner (dpy, clipboard_atom, sev->requestor, CurrentTime);
    }
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

  window_type_atom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
  window_type_dock_atom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
  string_atom = XInternAtom (dpy, "STRING", False);
  primary_selection_atom = XInternAtom (dpy, "PRIMARY", False);
  clipboard_atom = XInternAtom (dpy, "CLIPBOARD", False);
  delete_atom = XInternAtom (dpy, "DELETE", False);
  clipboard_selection_atom = XInternAtom (dpy, "MINICLIPBOARD_SELECTION", False);
  active_window_atom = XInternAtom (dpy, "_NET_ACTIVE_WINDOW", False);

  screen = DefaultScreen (dpy);
  
  load_icon (&copy);
  load_icon (&paste);

  tray_init_session_info (dpy, copy.win, argv, argc);

  tray_init (dpy, copy.win);  
  tray_init (dpy, paste.win);

  XSelectInput (dpy, DefaultRootWindow (dpy), SubstructureNotifyMask);

  for (;;)
    {
      XEvent an_event;
      XAnyEvent *any;
      struct pix *p = NULL;

      XNextEvent (dpy, &an_event);

      any = (XAnyEvent *)&an_event;
      if (any->window == copy.win)
	p = &copy;
      else if (any->window == paste.win)
	p = &paste;
      
      switch (an_event.type) 
	{
	case Expose:
	  if (p)
	    {
	      XCopyArea (dpy, p->pix, p->win, p->gc, 0, 0,
			 p->attrib.width, p->attrib.height, 0, 0);
	      XShapeCombineMask (dpy, p->win, ShapeBounding,
				 0, 0, p->mask, ShapeSet);
	    }
	  break;

	case ButtonRelease:
	  if (p)
	    p->click (&an_event);
	  break;

	case SelectionNotify:
	  selection_notify (&an_event);
	  break;

	case SelectionRequest:
	  selection_request (&an_event);
	  break;
	}

      tray_handle_event (any->display, any->window, &an_event);
    }
}
