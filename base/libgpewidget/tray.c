/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

static Atom system_tray_atom;
static Atom system_tray_opcode_atom;
static Atom manager_atom;

static Window dock;

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static int (*old_error_handler) (Display *d, XErrorEvent *e);

static int trapped_error_code;

static int
error_handler (Display     *display,
	       XErrorEvent *error)
{
   trapped_error_code = error->error_code;
   return 0;
}

static void
trap_errors (void)
{
   trapped_error_code = 0;
   old_error_handler = XSetErrorHandler (error_handler);
}

static int
untrap_errors (void)
{
   XSetErrorHandler (old_error_handler);
   return trapped_error_code;
}

static void
tray_send_opcode (Display *dpy, Window w,
		  long message, long data1, long data2, long data3)
{
   XEvent ev;
   
   memset(&ev, 0, sizeof(ev));

   ev.xclient.type = ClientMessage;
   ev.xclient.window = w;
   ev.xclient.message_type = system_tray_opcode_atom;
   ev.xclient.format = 32;
   ev.xclient.data.l[0] = CurrentTime;
   ev.xclient.data.l[1] = message;
   ev.xclient.data.l[2] = data1;
   ev.xclient.data.l[3] = data2;
   ev.xclient.data.l[4] = data3;
   
   trap_errors ();
   XSendEvent (dpy, w, False, NoEventMask, &ev);
   XSync (dpy, False);
   untrap_errors ();
}

static void
find_tray (Display *dpy, Window win)
{
   XGrabServer (dpy);

   dock = XGetSelectionOwner (dpy, system_tray_atom);

   if (dock == None)
     XSelectInput (dpy, RootWindow (dpy, DefaultScreen (dpy)),
		   StructureNotifyMask);
   
   XUngrabServer (dpy);
   XFlush (dpy);

   if (dock != None) 
     tray_send_opcode (dpy, dock, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0);
}

static GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  Window win = (Window)p;

  if (xev->type == ClientMessage
      && xev->xclient.message_type == manager_atom
      && dock == None
      && xev->xclient.data.l[1] == system_tray_atom)
    find_tray (xev->xany.display, win);

  return GDK_FILTER_CONTINUE;
}

void
gpe_system_tray_dock (GdkWindow *window)
{
  Display *dpy = GDK_WINDOW_XDISPLAY (window);
  Window win = GDK_WINDOW_XWINDOW (window);
  gchar *tray_atom_name;

  manager_atom = XInternAtom (dpy, "MANAGER", False);
  system_tray_opcode_atom = XInternAtom (dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  tray_atom_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d", DefaultScreen (dpy));
  system_tray_atom = XInternAtom (dpy, tray_atom_name, False);
  g_free (tray_atom_name);

  gdk_window_add_filter (GDK_ROOT_PARENT (), filter, (gpointer)win);

  find_tray (dpy, win);
}
