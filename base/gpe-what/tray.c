#include "tray.h"

static Atom Atoms[10];
static Window Dock;
static Bool   Is_docked;
static Bool   Found_tray;

static
void tray_send_message_data( Display* dpy, Window w, unsigned char *data);

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
   
   /* !!! This could mess events up !!! */
   XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)), StructureNotifyMask);
		
   Is_docked = False;
		
   /* Find the dock */
   Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SYSTEM_TRAY]);
   if (Dock)
   {
      TRAYDBG("found dock, sending opcode\n");
      tray_unmap_window (dpy, win);
      tray_send_opcode(dpy, Dock, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0);
      /*
	next request map or have this already set ?
	or wait for XEMBED_EMBEDDED_NOTIFY ? 
      */
      Found_tray = True;
      return 1;
   }

   /* -- to fix one day --- 
   TRAYDBG("cant find dock, checking for kde root prop\n");

   {
      Atom realType;
      unsigned long n;
      unsigned long extra;
      int format;
      int status;
      char * value;
   
      status = XGetWindowProperty(dpy, DefaultRootWindow(dpy),
				  Atoms[ATOM_KDE_SYSTEM_TRAY], 0L, 32L, False,
				  XA_WINDOW, &realType, &format,
				  &n, &extra, (unsigned char **) &value);
      if (status)
      {
	 TRAYDBG("found some kde prop gonna message win %i\n", value[0]);
	 tray_send_opcode(dpy, value[0], SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0);
	 Found_tray = True;
	 return 1;
      }
   }
   */
   
   Found_tray = False;
   return 0;
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
   /*
     tray should handle -
     ReparentNotify, if parent != Dock then not docked anymore - hide!
     ClientMessage, XEMBED_EMBEDDED_NOTIFY to set truly docked flag
                    MANAGER on root -> selection appeared ?
   */

   //tray_map_window (dpy, win);
   
   switch (an_event->type)
   {
      case ClientMessage:
	 TRAYDBG("message recieved\n");

	 if (an_event->xclient.message_type
	     == Atoms[ATOM_XEMBED])
	 {
	    switch (an_event->xclient.data.l[1])
	    {
	       case XEMBED_EMBEDDED_NOTIFY:
	       case XEMBED_WINDOW_ACTIVATE:
		  tray_map_window (dpy, win);
		  break;
	    }
	 }
	 if (an_event->xclient.message_type
	     == Atoms[ATOM_MANAGER] && !Found_tray)
	 {   /* dock has appeared  */
	    if (an_event->xclient.data.l[1] == Atoms[ATOM_SYSTEM_TRAY])
	    {
	       Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SYSTEM_TRAY]);
	       TRAYDBG("got manager message on root\n");
	       if (Dock)
	       {
		  tray_unmap_window (dpy, win);
		  tray_send_opcode(dpy, Dock, SYSTEM_TRAY_REQUEST_DOCK,
				   win, 0, 0);
		  Found_tray = True;
	       }
	    }
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
   printf("changing win %d\n", win);
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

void
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
   
   //trap_errors();
   XSendEvent(dpy, Dock, False, NoEventMask, &ev);
   XSync(dpy, False);
   //if (untrap_errors()) {
      /* Handle failure */
   //}
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
   
   //trap_errors();
   XSendEvent(dpy, Dock, False, NoEventMask, &ev);
   XSync(dpy, False);
   //if (untrap_errors()) {
      /* Handle failure */
   //}
}



