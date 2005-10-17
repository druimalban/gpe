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

static MBTrayBackgroundCB _bg_changed_cb = NULL;
static void *_bg_changed_cb_data = NULL;

static void mb_tray_send_message_data( Display* dpy, Window w,
				    unsigned char *data);
static int mb_tray_find_dock(Display *dpy, Window win);
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
mb_tray_init(Display* dpy, Window win)
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
		root_attr.your_event_mask|PropertyChangeMask|StructureNotifyMask);

   /* Find the dock */

   XFlush(dpy);

   if (tray_num < 0)  /* if traynum is -ive we print win to stdout and */
     {		      /* ignore the system tray protocol.              */
       fprintf(stdout, "%li\n", win); 
       fclose(stdout);
       return 1;
     }
   else return mb_tray_find_dock(dpy, win);
}

static int
mb_tray_find_dock(Display *dpy, Window win)
{
  XEvent an_event;

  XGrabServer (dpy);
  Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SYSTEM_TRAY]);
  XUngrabServer (dpy);
  XFlush (dpy);

  if (!Dock)
    {
      Bool not_found = True;
      /*
      XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		   StructureNotifyMask);
      */
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
		        TRAYDBG("AHA!, the dock is in my sights\n");
		        XGrabServer (dpy);
			Dock = XGetSelectionOwner(dpy, 
						  Atoms[ATOM_SYSTEM_TRAY]);
			if (Dock) not_found = False;
  			XUngrabServer (dpy);
			XFlush (dpy);
		    }
		}
	      break;
	    }	    
	}
    }
   
  TRAYDBG("found dock, sending opcode\n");
  mb_tray_send_opcode(dpy, Dock, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0);

  XSelectInput(dpy, Dock, PropertyChangeMask);


  return 1;
}

Window mb_tray_get_window(void)
{
   return Dock; 
}

void
mb_tray_init_session_info(Display *dpy, Window win, char **argv, int argc)
{
  pid_t this_pid = getpid();

  Atoms[ATOM_NET_WM_PID] = XInternAtom(dpy, "_NET_WM_PID", False);
  
  XSetCommand(dpy, win, argv, argc);

  XChangeProperty (dpy, win, Atoms[ATOM_NET_WM_PID], /*XA_ATOM*/
		   XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *)this_pid, 0);
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


void
mb_tray_handle_event(Display *dpy, Window win, XEvent *an_event)
{
  switch (an_event->type)
    {
    case ClientMessage:
      if (an_event->xclient.message_type
	  == Atoms[ATOM_XEMBED])
	{
	  TRAYDBG("Xembed message recieved %li \n",
		  an_event->xclient.data.l[1] );
	  switch (an_event->xclient.data.l[1])
	    {
	    case XEMBED_EMBEDDED_NOTIFY:
	    case XEMBED_WINDOW_ACTIVATE:
	      mb_tray_map_window (dpy, win);
	      break;
	    }
	}
      if (an_event->xclient.message_type == Atoms[ATOM_MANAGER]
	  && Dock == None)
	{ 
	  if (an_event->xclient.data.l[1] == Atoms[ATOM_SYSTEM_TRAY])
	    mb_tray_find_dock(dpy, win);
	}
      break;
    case ReparentNotify:
      TRAYDBG("ouch got reparented\n");
      break;
    case PropertyNotify:
      if (an_event->xproperty.atom == Atoms[ATOM_MB_PANEL_BG])
	{
	  TRAYDBG("%s() ATOM_MB_PANEL_BG change\n", __func__ );
	  if (_bg_changed_cb)
	    {
	      TRAYDBG("%s() calling call back\n", __func__ );
	      _bg_changed_cb(_bg_changed_cb_data);	  
	    }
	}
       break;
    }
}

void
mb_tray_set_xembed_info (Display* dpy, Window win, int flags)
{
   CARD32 list[2];
   
   list[0] = MAX_SUPPORTED_XEMBED_VERSION;
   list[1] = flags;
   XChangeProperty (dpy, win, Atoms[ATOM_XEMBED_INFO], /*XA_ATOM*/
		    Atoms[ATOM_XEMBED_INFO] , 32,
		    PropModeReplace, (unsigned char *) list, 2);
   TRAYDBG("%s() called\n", __func__);
}

void
mb_tray_map_window (Display* dpy, Window win)
{
  XMapRaised(dpy, win); /* needed for gnome dock - FIXME*/
  TRAYDBG("%s() called\n", __func__);
 if (_bg_changed_cb) _bg_changed_cb(_bg_changed_cb_data);	  
  mb_tray_set_xembed_info (dpy, win, XEMBED_MAPPED);
}

void
mb_tray_unmap_window (Display* dpy, Window win)
{
   mb_tray_set_xembed_info (dpy, win, 0);
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

void
mb_tray_bg_change_cb_set(MBTrayBackgroundCB bg_changed_cb, void *user_data)
{
  if (bg_changed_cb)
    {
      _bg_changed_cb = bg_changed_cb;
      _bg_changed_cb_data = user_data;
    }
}

void
mb_tray_window_icon_set(Display *dpy, Window win_panel, MBPixbufImage *img)
{
  CARD32 *data = NULL;

  data = malloc(sizeof(CARD32)*((img->width*img->height)+2));
  if (data)
    {
      int i = 0, idx = 0;
      data[0] = img->width;
      data[1] = img->height;

      for( i=2; i < (img->width*img->height)+2; i++)
	{
	  idx = (i-2)*4;
	  data[i] = img->rgba[idx] << 16 | img->rgba[idx+1] << 8  
	    | img->rgba[idx+2] | img->rgba[idx+3] << 24;  
	}
      
      XChangeProperty(dpy, win_panel, Atoms[ATOM_NET_WM_ICON] ,
		      XA_CARDINAL, 32, PropModeReplace,
		      (unsigned char *)data, 
		      (img->width*img->height)+2);
      free(data);
    }
  
}


Bool 
mb_tray_get_bg_col(Display *dpy, XColor *xcol)
{
  Window root;
  Atom realType;
  unsigned long n;
  unsigned long extra;
  int format;
  int status;
  char *value = NULL;


  root = RootWindow(dpy, DefaultScreen(dpy));

  status = XGetWindowProperty(dpy, Dock,
			      Atoms[ATOM_MB_PANEL_BG], 0L, 512L, False,
			      XA_STRING, &realType,
			      &format, &n, &extra,
			      (unsigned char **) &value);

  if (status == Success && value != NULL)
    {
      TRAYDBG("got background %s\n", value);  
      xcol->pixel = (Pixmap)atol(value+4);
      if (XQueryColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), xcol))
	return True;
    }
  
  TRAYDBG("Failed to get panel background\n");
  XParseColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), "white", xcol);

  return False;
}

MBPixbufImage *
mb_tray_get_bg_img(MBPixbuf *pb, Window win)
{

  XWindowAttributes win_attr;
  Window root;
  Atom realType;
  unsigned long n;
  unsigned long extra;
  int format;
  int status;
  char *value = NULL;

  MBPixbufImage *img_result = NULL;
  XColor xcol;

  root = RootWindow(pb->dpy, pb->scr);

  status = XGetWindowProperty(pb->dpy, Dock,
			      Atoms[ATOM_MB_PANEL_BG], 0L, 512L, False,
			      XA_STRING, &realType,
			      &format, &n, &extra,
			      (unsigned char **) &value);

  if (!XGetWindowAttributes(pb->dpy, win, &win_attr))
    {
      fprintf(stderr, "libmb: unable to get window attributes for %li\n",
	      win);
      if (value) XFree(value);
      return NULL;
    }

  if (status == Success && value != NULL && strlen(value) > 4)
    {
      TRAYDBG("%s() got background %s\n", __func__, value);  
      
      TRAYDBG("%s() got %i+%i %ix%i\n", __func__, win_attr.x, 
	     win_attr.y, 
	     win_attr.width, 
	     win_attr.height);
	
      if (strncasecmp("pxm", value, 3) == 0)
	{
	  /* Pixmap */
	  Pixmap foreign_pxm = (Pixmap)atol(value+4);

	  TRAYDBG("%s() pixmap id is %li\n", __func__, foreign_pxm);  

	  if (foreign_pxm)
	    img_result = mb_pixbuf_img_new_from_drawable(pb, foreign_pxm, 
							 None, 
							 win_attr.x, 
							 win_attr.y, 
							 win_attr.width, 
							 win_attr.height);
	}
      else
	{
	  xcol.pixel = (Pixmap)atol(value+4);

	  XQueryColor(pb->dpy, DefaultColormap(pb->dpy, pb->scr), &xcol);

	  TRAYDBG("%s() got col %x, %x, %x\n", __func__, xcol.red, 
		  xcol.green, xcol.blue);  

	  img_result = mb_pixbuf_img_new(pb, win_attr.width, 
					 win_attr.height); 
	  mb_pixbuf_img_fill(pb, img_result, 
			     xcol.red >> 8, 
			     xcol.green >> 8, 
			     xcol.blue >> 8, 0);
	}
    } else {
      TRAYDBG("%s() failed to get bg prop\n",__func__);  
    }

  if (img_result == NULL)
    {				/* Blank */
      fprintf(stderr, "mbdock: unable to get panel background\n"); 
      img_result = mb_pixbuf_img_new(pb, win_attr.width, win_attr.height); 
      TRAYDBG("%s() img_result is NULL !\n",__func__);  
    }

  if (value) XFree(value);

  return img_result;

}

