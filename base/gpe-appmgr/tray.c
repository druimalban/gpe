#include "tray.h"

static Atom Atoms[10];
static Window Dock = None;
static int trapped_error_code = 0;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

static void tray_send_message_data( Display* dpy, Window w,
				    unsigned char *data);
static int tray_find_dock(Display *dpy, Window win);
static void tray_set_xembed_info (Display* dpy, Window win, int flags);
static void tray_send_opcode( Display* dpy,  Window w, long message,
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

int
tray_init(Display* dpy, Window win)
{
   Atoms[ATOM_SYSTEM_TRAY]
      = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);

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
   
   /* Find the dock */
   return tray_find_dock(dpy, win);
}

static int
tray_find_dock(Display *dpy, Window win)
{
   XGrabServer (dpy);

   Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SYSTEM_TRAY]);

   if (!Dock)
      XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)),
		   StructureNotifyMask);

   XUngrabServer (dpy);
   XFlush (dpy);
   
   if (Dock != None) {
      TRAYDBG("found dock, sending opcode\n");
      tray_send_opcode(dpy, Dock, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0);
      return 1;
   } 

   return 0;
}

Window tray_get_window(void)
{
   return Dock; 
}

void
tray_init_session_info(Display *d, Window win, char **argv, int argc)
{
   XSetCommand(d, win, argv, argc);
}

void
tray_send_message(Display *d, Window win, unsigned char* msg)
{
   unsigned char buf[20];
   int msg_len = strlen(msg);
   int id = 12345; /* TODO id should unique */
   int bytes_sent = 0;
   
   tray_send_opcode( d, win, SYSTEM_TRAY_BEGIN_MESSAGE, 0, msg_len, id );  

   while ( bytes_sent < msg_len )
   {
      if ( (msg_len - bytes_sent) > 20)
      {
	 memcpy(buf, &msg[bytes_sent], sizeof(unsigned char)*20);
      } else {
	 memcpy(buf, &msg[bytes_sent],
		sizeof(unsigned char)*(msg_len - bytes_sent) );
      }
      tray_send_message_data( d, win, buf );
      bytes_sent += 20;
   }
}


void
tray_handle_event(Display *dpy, Window win, XEvent *an_event)
{
   switch (an_event->type)
   {
      case ClientMessage:


	 if (an_event->xclient.message_type
	     == Atoms[ATOM_XEMBED])
	 {
	    TRAYDBG("Xembed message recieved %i \n",
		    an_event->xclient.data.l[1] );
	    switch (an_event->xclient.data.l[1])
	    {
	       case XEMBED_EMBEDDED_NOTIFY:
	       case XEMBED_WINDOW_ACTIVATE:
		  tray_map_window (dpy, win);
		  break;
	    }
	 }
	 if (an_event->xclient.message_type == Atoms[ATOM_MANAGER]
	     && Dock == None)
	 { 
	    if (an_event->xclient.data.l[1] == Atoms[ATOM_SYSTEM_TRAY])
	       tray_find_dock(dpy, win);
	 }
	 break;
      case ReparentNotify:
	 TRAYDBG("ouch got reparented\n");
	 break;
   }
}

void
tray_set_xembed_info (Display* dpy, Window win, int flags)
{
   CARD32 list[2];
   
   list[0] = MAX_SUPPORTED_XEMBED_VERSION;
   list[1] = flags;
   XChangeProperty (dpy, win, Atoms[ATOM_XEMBED_INFO], XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *) list, 2);
}

void
tray_map_window (Display* dpy, Window win)
{
   tray_set_xembed_info (dpy, win, XEMBED_MAPPED);
}

void
tray_unmap_window (Display* dpy, Window win)
{
   tray_set_xembed_info (dpy, win, 0);
}

static void
tray_send_opcode(
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
   ev.xclient.data.l[0] = CurrentTime;
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
tray_send_message_data(
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



