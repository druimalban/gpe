/*
 * xsingleinstance
 *
 * Program to ensure that a single instance of an X window program
 * exists on the screen.
 *
 * Copyright (c) 2000, Merle F. McClelland for CompanionLink
 * Portions of this code, such as TryChildren and ClientWindow, are
 * based upon routines in the Xmu library, which is Copyright 1989, 1998,
 * The Open Group.  All Rights Reserved.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * See the files COPYRIGHT and LICENSE for distribution information.
*/

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <signal.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#define FALSE	0
#define TRUE	1

Atom wm_state_atom    = 0;

/*
 * Window_With_Name: routine to locate a window with a given name on a display.
 *                   If no window with the given name is found, 0 is returned.
 *                   If more than one window has the given name, the first
 *                   one found will be returned.  Only top and its subwindows
 *                   are looked at.  Normally, top should be the RootWindow.
 */
Window Window_With_Name(dpy, top, name)
     Display *dpy;
     Window top;
     char *name;
{
        Window *children, dummy;
        unsigned int nchildren;
        int i;
        Window w=0;
        char *window_name;

        if (XFetchName(dpy, top, &window_name) && !strcmp(window_name, name))
          return(top);

        if (!XQueryTree(dpy, top, &dummy, &dummy, &children, &nchildren))
          return(0);

        for (i=0; i<nchildren; i++) {
                w = Window_With_Name(dpy, children[i], name);
                if (w)
                  break;
        }
        if (children) XFree ((char *)children);
        return(w);
}

int Window_With_xid(dpy, top, xid)
     Display *dpy;
     Window top;
     Window xid;
{
        Window *children, dummy;
        unsigned int nchildren;
        int i;
        int w=0;

	if (top == xid) return True;
	
        if (!XQueryTree(dpy, top, &dummy, &dummy, &children, &nchildren))
          return(0);

        for (i=0; i<nchildren; i++) {
                w = Window_With_xid(dpy, children[i], xid);
                if (w)
                  break;
        }
        if (children) XFree ((char *)children);
        return(w);
}


static Window
TryChildren(Display *dpy, Window win, Atom WM_STATE)
{
	Window root, parent;
	Window *children;
	unsigned int nchildren;
	unsigned int i;
	Atom type = None;
	int format;
	unsigned long nitems, after;
	unsigned char *data;
	Window inf = 0;

	if (!XQueryTree(dpy, win, &root, &parent, &children, &nchildren))
		return 0;
	for (i = 0; !inf && (i < nchildren); i++) 
	{
		XGetWindowProperty(dpy, children[i], WM_STATE, 0, 0, False,
	                        AnyPropertyType, &type, &format, &nitems,
	                        &after, &data);
		if (type)
			inf = children[i];
	}
	for (i = 0; !inf && (i < nchildren); i++)
		inf = TryChildren(dpy, children[i], WM_STATE);
	if (children) XFree((char *)children);
	return inf;
}

Window
ClientWindow(Display *dpy, Window win)
{
	 Atom type = None;
	 int format;
	 unsigned long nitems, after;
	 unsigned char *data;
	 Window inf;

	 if (!wm_state_atom)
	     return win;
	 XGetWindowProperty(dpy, win, wm_state_atom, 0, 0,
			    False, AnyPropertyType,
	                    &type, &format, &nitems, &after, &data);
	 if (type)
	     return win;
	 inf = TryChildren(dpy, win, wm_state_atom);
	 if (!inf)
	     inf = win;
	 return inf;
}

//
// Find a window's state
//
int
wm_state(Display *display, Window w)
{
	Atom real_type;
	int real_format, state = 0;
	unsigned long n, extra;
	unsigned char *data;

	if (XGetWindowProperty(display, w, wm_state_atom, 0L, 2L, False, AnyPropertyType,
	   		&real_type, &real_format, &n, &extra, &data) == Success && n) 
	{
		state = *(int *)data;
		XFree(data);
		return state;
	}
	else
		return -1;
}

//
// bring a window to the foreground
//
void raise_window(Display *display, Window w)
{
	int iState;
	iState = wm_state(display, w);
	if (iState == NormalState)
	 		XMapRaised(display, w);
  	else if( iState == IconicState )
	 		XMapRaised(display, w);
	else XRaiseWindow (display, w);
}

int try_to_raise_window (Display *display, char *window)
{
	if (window)
	{
	        Window xid = 0;
		int screen;

		screen = DefaultScreen(display);

		xid = Window_With_Name(display, RootWindow(display, screen),
			               window);

		if (xid != 0)
		{
			XWindowAttributes xid_attrib;

			wm_state_atom = XInternAtom(display, "WM_STATE", True);
        		XGetWindowAttributes(display, xid, &xid_attrib);
			if (True || xid_attrib.map_state == IsUnmapped ||
				wm_state(display, xid) != NormalState)
			{
				raise_window (display, xid);
				printf ("raised window\n");
			}
			else
			{
				printf ("window already raised?\n");
			}
		}
		else
		{
			printf ("can't find\n");
			return 1;
		}

	}
	else
	{
		return 1;
	}
	return 0;
}
