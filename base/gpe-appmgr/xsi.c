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

#include <X11/Xatom.h> // XA_*

#include <glib.h>
#include "xsi.h"

#include "gnome-exec.h"

//#define FALSE	0
//#define TRUE	1

Display *display;
int screen;
int debug;
Atom wm_state_atom = 0;

char *atom_names[] = {
	/* clients */
	"KWM_WIN_ICON",
	"WM_STATE",
	"_MOTIF_WM_HINTS",
	"_NET_WM_STATE",
	"_NET_WM_STATE_SKIP_TASKBAR",
	"_NET_WM_STATE_SHADED",
	"_NET_WM_DESKTOP",
	"_NET_WM_WINDOW_TYPE",
	"_NET_WM_WINDOW_TYPE_DOCK", /* 8 */
	"_NET_WM_STRUT",
	"_WIN_HINTS",
	/* root */
	"_NET_CLIENT_LIST",
	"_NET_NUMBER_OF_DESKTOPS",
	"_NET_CURRENT_DESKTOP",
};

#define ATOM_COUNT (sizeof (atom_names) / sizeof (atom_names[0]))

Atom atoms[ATOM_COUNT];

#define atom_KWM_WIN_ICON atoms[0]
#define atom_WM_STATE atoms[1]
#define atom__MOTIF_WM_HINTS atoms[2]
#define atom__NET_WM_STATE atoms[3]
#define atom__NET_WM_STATE_SKIP_TASKBAR atoms[4]
#define atom__NET_WM_STATE_SHADED atoms[5]
#define atom__NET_WM_DESKTOP atoms[6]
#define atom__NET_WM_WINDOW_TYPE atoms[7]
#define atom__NET_WM_WINDOW_TYPE_DOCK atoms[8]
#define atom__NET_WM_STRUT atoms[9]
#define atom__WIN_HINTS atoms[10]
#define atom__NET_CLIENT_LIST atoms[11]
#define atom__NET_NUMBER_OF_DESKTOPS atoms[12]
#define atom__NET_CURRENT_DESKTOP atoms[13]

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

void *
get_prop_data (Window win, Atom prop, Atom type, int *items)
{
	Atom type_ret;
	int format_ret;
	unsigned long items_ret;
	unsigned long after_ret;
	unsigned char *prop_data;

	prop_data = 0;

	XGetWindowProperty (display, win, prop, 0, 0x7fffffff, False,
			    type, &type_ret, &format_ret, &items_ret,
			    &after_ret, &prop_data);
	if (items)
		*items = items_ret;

	return prop_data;
}

/*
 * Window_With_Name: routine to locate a window with a given name on a display.
 *                   If no window with the given name is found, 0 is returned.
 *                   If more than one window has the given name, the first
 *                   one found will be returned.  Only top and its subwindows
 *                   are looked at.  Normally, top should be the RootWindow.
 */
Window Window_With_Name(Display *dpy, Window top, char *name)
{
	Window *win, ret=0;
	int num=-1, i;

	XInternAtoms (display, atom_names, ATOM_COUNT, False, atoms);

	win = get_prop_data (top, atom__NET_CLIENT_LIST, XA_WINDOW, &num);

	for (i=0;i<num;i++) {
		char *wname;
		wname = get_prop_data (win[i], XA_WM_NAME, XA_STRING, 0);
		if (!wname)
			continue;

		if (!fnmatch(name, wname, 0)) {
			ret = win[i];
			break;
		}
	}

	XFree (win);

	return ret;
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

int run_program (char *exec, char *name)
{
	// We just use the DISPLAY environment variable
	char *disp = NULL;
	int xid;

	// Set up our connection to the X server
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

	// If xid is non-zero, a window with that name was found.
	// Otherwise, we fork the passed program.
	if (xid == 0)
	{
		char *cmd[] = {"/bin/sh", "-c", exec};
		gnome_execute_async (NULL, 3, cmd);
	}
	else
		raise_window(display, xid);

	return 0;
}

