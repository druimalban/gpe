/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
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

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "init.h"

#define SYSTEM_TRAY_OPCODE_ATOM	 0
#define MANAGER_ATOM		 1
#define SYSTEM_TRAY_MESSAGE_DATA 2

static Atom atoms[3];

static char *atom_names[] = 
  { 
    "_NET_SYSTEM_TRAY_OPCODE", 
    "MANAGER",
    "_NET_SYSTEM_TRAY_MESSAGE_DATA"
  };

static Atom system_tray_atom;

static Window dock;

#define MAX_DATA_BYTES 20

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static void
tray_send_opcode (Display *dpy, Window w,
		  long message, long data1, long data2, long data3)
{
   XEvent ev;
   
   memset(&ev, 0, sizeof(ev));

   ev.xclient.type = ClientMessage;
   ev.xclient.window = w;
   ev.xclient.message_type = atoms[SYSTEM_TRAY_OPCODE_ATOM];
   ev.xclient.format = 32;
   ev.xclient.data.l[0] = CurrentTime;
   ev.xclient.data.l[1] = message;
   ev.xclient.data.l[2] = data1;
   ev.xclient.data.l[3] = data2;
   ev.xclient.data.l[4] = data3;
   
   gdk_error_trap_push ();

   XSendEvent (dpy, dock, False, NoEventMask, &ev);

   gdk_flush ();
   gdk_error_trap_pop ();
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
      && xev->xclient.message_type == atoms[MANAGER_ATOM]
      && dock == None
      && xev->xclient.data.l[1] == system_tray_atom)
    find_tray (xev->xany.display, win);

  return GDK_FILTER_CONTINUE;
}

/**
 * gpe_system_tray_send_message:
 * @window: Dock window that should pop up the message. 
 * @text: Message to display.
 * @timeout: Time in seconds until message disappears.
 *
 * Pop up a message in the panel. These messages are intended to give the user 
 * some information if something important is going on and should disappear 
 * without user interaction. Note that message timeouts may be not implemented
 * in some environments.
 *
 * Returns: An unique message id.
 */
guint
gpe_system_tray_send_message (GdkWindow *window, const gchar *text, unsigned int timeout)
{
  static int id;
  size_t len, bytes_sent;
  XEvent ev;
  char *buf;
  Display *dpy;
  Window win;

  if (dock == None)
    return -1;		/* not docked */

  dpy = GDK_WINDOW_XDISPLAY (window);
  win = GDK_WINDOW_XWINDOW (window);

  ++id;

  len = strlen (text);

  tray_send_opcode (dpy, win, SYSTEM_TRAY_BEGIN_MESSAGE, timeout, len, id);

  memset (&ev, 0, sizeof (ev));

  ev.xclient.type = ClientMessage;
  ev.xclient.window = win;
  ev.xclient.message_type = atoms[SYSTEM_TRAY_MESSAGE_DATA];
  ev.xclient.format = 8;

  buf = &ev.xclient.data.b[0];

  gdk_error_trap_push ();

  for (bytes_sent = 0; bytes_sent < len; )
    {
      size_t s;

      s = len - bytes_sent;

      if (s > MAX_DATA_BYTES)
	s = MAX_DATA_BYTES;
      
      memcpy (buf, text + bytes_sent, s);

      XSendEvent (dpy, dock, False, NoEventMask, &ev);

      bytes_sent += s;
    }

  gdk_flush ();
  gdk_error_trap_pop ();

  return id;
}

/**
 * gpe_system_tray_cancel_message:
 * @window: Window to remove a message from.
 * @id: Id of the message to remove.
 *
 * Remove a message currently displayed from panel.
 */
void
gpe_system_tray_cancel_message (GdkWindow *window, guint id)
{
  tray_send_opcode (GDK_WINDOW_XDISPLAY (window), GDK_WINDOW_XWINDOW (window),
		    SYSTEM_TRAY_CANCEL_MESSAGE, id, 0, 0);
}

/**
 * gpe_system_tray_dock:
 * @window: Window to dock.
 * 
 * Make a window a dock window and dock it to a system tray.
 */
void
gpe_system_tray_dock (GdkWindow *window)
{
  Display *dpy = GDK_WINDOW_XDISPLAY (window);
  Window win = GDK_WINDOW_XWINDOW (window);
  gchar *tray_atom_name;
  gint argc;
  gchar **argv;

  XInternAtoms (dpy, atom_names, 3, False, atoms);
  tray_atom_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d", DefaultScreen (dpy));
  system_tray_atom = XInternAtom (dpy, tray_atom_name, False);
  g_free (tray_atom_name);

  gpe_saved_args (&argc, &argv);
  XSetCommand (dpy, win, argv, argc);

  gdk_window_add_filter (GDK_ROOT_PARENT (), filter, (gpointer)win);

  find_tray (dpy, win);
  gdk_notify_startup_complete();
}
