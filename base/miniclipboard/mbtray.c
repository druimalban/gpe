/* libmbdock 
 * Copyright (C) 2002 Matthew Allum
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

#include "mbtray.h"

static Atom Atoms[10];
static Window Dock = None;
static int trapped_error_code = 0;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

static MBTrayBackgroundCB _bg_changed_cb;
static void *_bg_changed_cb_data;

static void mb_tray_send_message_data( Display* dpy, Window w,
				    unsigned char *data);
static int mb_tray_find_dock(Display *dpy);
static void mb_tray_set_xembed_info (Display* dpy, Window win, int flags);
static void mb_tray_send_opcode( Display* dpy,  Window w, long message,
			      long data1, long data2, long data3 );

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

static unsigned long 
_get_server_time(Display* dpy)
{
  XEvent xevent;
  Atom timestamp_atom = XInternAtom(dpy, "_MB_DOCK_TIMESTAMP", False);
  char c = 'a';

  XChangeProperty (dpy, RootWindow(dpy, DefaultScreen(dpy)), 
		   timestamp_atom, timestamp_atom,
		   8, PropModeReplace, &c, 1);
  for (;;) {
    XMaskEvent(dpy, PropertyChangeMask, &xevent);
        if (xevent.xproperty.atom == timestamp_atom)
	  return xevent.xproperty.time;
  }
}


int
mb_tray_init(Display* dpy)
{
   XWindowAttributes root_attr;
   
   int  tray_num = 0;
   char tray_atom_spec[128] = { 0 }; 
   
   if (getenv("SYSTEM_TRAY_ID"))
       tray_num = atoi(getenv("SYSTEM_TRAY_ID"));

   snprintf(tray_atom_spec, 128, "_NET_SYSTEM_TRAY_S%i", tray_num);

   Atoms[ATOM_SYSTEM_TRAY]
      = XInternAtom(dpy, tray_atom_spec, False);

   Atoms[ATOM_KDE_SYSTEM_TRAY]
      = XInternAtom(dpy, "_KDE_NET_SYSTEM_TRAY_WINDOWS", False);
   
   Atoms[ATOM_SYSTEM_TRAY_OPCODE]
      =  XInternAtom (dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
   
   Atoms[ATOM_XEMBED_INFO]
      = XInternAtom(dpy, "_XEMBED_INFO", False);

   Atoms[ATOM_XEMBED]
      = XInternAtom(dpy, "_XEMBED", False);

   Atoms[ATOM_MANAGER]
      = XInternAtom(dpy, "MANAGER", False);

   Atoms[ATOM_NET_SYSTEM_TRAY_MESSAGE_DATA]
      = XInternAtom(dpy, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);

   Atoms[ATOM_NET_WM_ICON]
     = XInternAtom(dpy, "_NET_WM_ICON", False);

   Atoms[ATOM_MB_PANEL_BG]
     = XInternAtom(dpy, "_MB_PANEL_BG", False);

   XGetWindowAttributes(dpy, RootWindow(dpy, DefaultScreen(dpy)), &root_attr);

   XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)), 
		root_attr.your_event_mask|PropertyChangeMask);

   /* Find the dock */

   XFlush(dpy);

   return mb_tray_find_dock(dpy);
}

static int
mb_tray_find_dock(Display *dpy)
{
  XEvent an_event;

  XGrabServer (dpy);
  Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SYSTEM_TRAY]);
  XUngrabServer (dpy);
  XFlush (dpy);

  if (!Dock)
    {
      Bool not_found = True;
      XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		   StructureNotifyMask);
      while (not_found)
	{
	  TRAYDBG("Trying to find dock\n");
	  XNextEvent(dpy, &an_event);
	  switch (an_event.type)
	    {
	    case ClientMessage:
	      if (an_event.xclient.message_type == Atoms[ATOM_MANAGER])
		{ 
		  if (an_event.xclient.data.l[1] == Atoms[ATOM_SYSTEM_TRAY])
		    {
		        XGrabServer (dpy);
			Dock = XGetSelectionOwner(dpy, 
						  Atoms[ATOM_SYSTEM_TRAY]);
			if (Dock) not_found = False;
			XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)),
				     PropertyChangeMask);
  			XUngrabServer (dpy);
			XFlush (dpy);
		    }
		}
	      break;
	    }	    
	}
    }
   
  TRAYDBG("found dock, sending opcode\n");

  return 1;
}

Window mb_tray_get_window(void)
{
   return Dock; 
}

void
mb_tray_send_message(Display *d, Window win, unsigned char* msg, int timeout)
{
   unsigned char buf[20];
   int msg_len = strlen(msg);
   int id = 12345; /* TODO id should unique */
   int bytes_sent = 0;
   
   mb_tray_send_opcode(d,win, SYSTEM_TRAY_BEGIN_MESSAGE, timeout, msg_len, id );  

   while ( bytes_sent < msg_len )
   {
      if ( (msg_len - bytes_sent) > 20)
      {
	 memcpy(buf, &msg[bytes_sent], sizeof(unsigned char)*20);
      } else {
	 memcpy(buf, &msg[bytes_sent],
		sizeof(unsigned char)*(msg_len - bytes_sent) );
      }
      mb_tray_send_message_data( d, win, buf );
      bytes_sent += 20;
   }
}

static void
mb_tray_send_opcode(
		      Display* dpy,
		      Window w,    
		      long message,
		      long data1,  
		      long data2,  
		      long data3   
		      ){
   XEvent ev;
   
   memset(&ev, 0, sizeof(ev));
   ev.xclient.type = ClientMessage;
   ev.xclient.window = w;
   ev.xclient.message_type = Atoms[ATOM_SYSTEM_TRAY_OPCODE];
   ev.xclient.format = 32;
   ev.xclient.data.l[0] = _get_server_time(dpy);
   ev.xclient.data.l[1] = message;
   ev.xclient.data.l[2] = data1;
   ev.xclient.data.l[3] = data2;
   ev.xclient.data.l[4] = data3;
   
   trap_errors();
   XSendEvent(dpy, Dock, False, NoEventMask, &ev);
   XSync(dpy, False);
   if (untrap_errors()) {
      fprintf(stderr, "Tray.c : X error %i on opcode send\n",
	      trapped_error_code );
   }
}

static void
mb_tray_send_message_data(
		      Display* dpy,
		      Window w,
		      unsigned char *data
		      ){
   XEvent ev;
   memset(&ev, 0, sizeof(ev));
   ev.xclient.type = ClientMessage;
   ev.xclient.window = w;
   ev.xclient.message_type = Atoms[ATOM_NET_SYSTEM_TRAY_MESSAGE_DATA];
   ev.xclient.format = 8;
   memcpy(ev.xclient.data.b, data, sizeof(unsigned char)*20);
   
   trap_errors();
   XSendEvent(dpy, Dock, False, NoEventMask, &ev);
   XSync(dpy, False);
   if (untrap_errors()) {
      fprintf(stderr, "Tray.c : X error %i on message send\n",
	      trapped_error_code );
   }
}
