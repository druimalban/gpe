/* Code mostly from:
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
 * GNU General Public License for more details.
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
#include <fnmatch.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>

#include <glib.h>

#include <xsi.h>
#include <gpe-exec.h>


static Display *display;
static int screen;
static Atom wm_state_atom = 0;

struct window_info *window_info_new (Window xid, char *name)
{
	struct window_info *w;

	w = (struct window_info *) malloc (sizeof (struct window_info));

	w->xid = xid;
	w->name = strdup(name);

	return w;
}

void window_info_free (struct window_info *w)
{
	g_assert (w != NULL);

	if (w->name)
		free (w->name);
	free (w);
}

GList *get_windows_with_name_in (Display *dpy, Window top, char *name)
{
        Window *children, dummy;
        unsigned int nchildren;
        int i;
        char *window_name;
	GList *l=NULL;

        if (XFetchName(dpy, top, &window_name) && !fnmatch(name, window_name, FNM_PERIOD))
		l = g_list_append (l, window_info_new (top, window_name));

        if (!XQueryTree(dpy, top, &dummy, &dummy, &children, &nchildren))
          return(0);

        for (i=0; i<nchildren; i++)
                l = g_list_concat (l, get_windows_with_name_in (dpy, children[i], name));

        if (children) XFree ((char *)children);

        return(l);
}

GList *get_windows_with_name (char *name)
{
	g_assert (name != NULL);

	if ((display = XOpenDisplay(NULL)) == NULL)
	{
		fprintf(stderr, "Cannot connect to X server on display %s.", XDisplayName(NULL));
		exit(1);
	}

	screen = DefaultScreen(display);
	wm_state_atom = XInternAtom(display, "WM_STATE", True);

	return get_windows_with_name_in (display, RootWindow(display, screen), name);
}

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

        if (XFetchName(dpy, top, &window_name) && !fnmatch(name, window_name, FNM_PERIOD))
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
	 XGetWindowProperty(dpy, win, wm_state_atom, 0, 0, False, AnyPropertyType,
	                    &type, &format, &nitems, &after, &data);
	 if (type)
	     return win;
	 inf = TryChildren(dpy, win, wm_state_atom);
	 if (!inf)
	     inf = win;
	 return inf;
}

/*
 * Find a window's state
 */
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

/*
 * bring a window to the foreground
 */
void raise_window(Display *display, Window w)
{
	int iState;

	iState = wm_state(display, w);

	if (iState == NormalState)
	 		XRaiseWindow(display, w);
  	else if( iState == IconicState )
	 		XMapRaised(display, w);

	/* We do it this way not ^ that way 'cos mallum's wm
	 * doesn't give the state properly 
	if (iState == IconicState)
		XMapRaised (display, w);
	else
		XRaiseWindow (display, w); */

	XFlush(display);
	XSync(display, True);
	XCloseDisplay(display);
}

void raise_window_default(Window w)
{
	raise_window (display, w);
}

void sendClientMessage(Window sendTo, Window w, Atom a, long x)
{
	XEvent ev;
	long mask;

	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = a;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = x;
	ev.xclient.data.l[1] = CurrentTime;
	mask = 0L;
	XSendEvent(display, sendTo, False, mask, &ev);
}

int run_program (char *exec, char *name)
{
	// We just use the DISPLAY environment variable
	char *disp = NULL;
	int xid;

	/* Set up our connection to the X server			*/
	if ((display = XOpenDisplay(disp)) == NULL)
	{
		fprintf(stderr, "Cannot connect to X server on display %s.", XDisplayName(disp));
		exit(1);
	}

	screen = DefaultScreen(display);

	wm_state_atom = XInternAtom(display, "WM_STATE", True);

	if (name)
		xid = Window_With_Name(display, RootWindow(display, screen), name);
	else
		xid = 0;

	/* If xid is non-zero, a window with that name was found.	*/
	/* Otherwise, we execute the passed program.			*/
	if (xid == 0)
	{
		XCloseDisplay(display);
		gpe_execute_async_cmd(exec);

#if 0
		switch (fork())
	        {
	        case 0: // Child process
			fprintf (stderr, "Running new process\n");
	              	execlp("/bin/sh", "sh", "-c", exec, NULL);

	              	// If we got here, we couldn't exec the process.
	              	fprintf(stderr, "Can't exec '/bin/sh -c \"%s\"'\n", exec);
			break;
	        case -1: // fork failed
	              	fprintf(stderr, "Couldn't fork\n");
			return -1;
	              	break;
	        }
#endif
	}
	else
	{
		fprintf (stderr, "Raising window %d on display %s\n", xid, display->display_name);
		raise_window(display, xid);
	}
	return 0;
}

