/*
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <libsn/sn.h>
#include <glib.h>
#include <time.h>

#include <mb/mbpixbuf.h>

#define TIMEOUT		20
#define POLLTIME	10

#define _(x) gettext(x)

struct launch
{
  gchar *id;
  time_t when;
};

static Window window;
static Display *xdisplay;
 
static GSList *launch_list;
static gboolean hourglass_shown;

static int target_x;

static MBPixbuf *mbpb;

#define wy 2

static void
show_hourglass (void)
{
  XMoveWindow (xdisplay, window, target_x, wy);
  XMapRaised (xdisplay, window);
  hourglass_shown = TRUE;
}

static void
hide_hourglass (void)
{
  XUnmapWindow (xdisplay, window);
  hourglass_shown = FALSE;
}

static void
monitor_event_func (SnMonitorEvent *event,
                    void            *user_data)
{
  SnMonitorContext *context;
  SnStartupSequence *sequence;
  const char *id;
  struct launch *l;
  time_t t;
  
  context = sn_monitor_event_get_context (event);
  sequence = sn_monitor_event_get_startup_sequence (event);
  id = sn_startup_sequence_get_id (sequence);
  
  switch (sn_monitor_event_get_type (event))
    {
    case SN_MONITOR_EVENT_INITIATED:
      l = g_malloc (sizeof (struct launch));
      l->id = g_strdup (id);
      t = time (NULL);
      l->when = t + TIMEOUT;
      launch_list = g_slist_append (launch_list, l);
      if (! hourglass_shown)
	show_hourglass ();
      break;

    case SN_MONITOR_EVENT_COMPLETED:
    case SN_MONITOR_EVENT_CANCELED:
      {
	GSList *iter, *next;
	
	for (iter = launch_list; iter; iter = next)
	  {
	    struct launch *l = (struct launch *)iter->data;

	    next = iter->next;

	    if (!strcmp (l->id, id))
	      {
		g_free (l->id);
		g_free (l);
		launch_list = g_slist_remove_link (launch_list, iter);
		g_slist_free_1 (iter);
	      }
	  }

	if (launch_list == NULL && hourglass_shown)
	  hide_hourglass ();
      }
      break;
    }
}

static int error_trap_depth = 0;

static int
x_error_handler (Display     *xdisplay,
                 XErrorEvent *error)
{
  char buf[64];
  
  XGetErrorText (xdisplay, error->error_code, buf, 63);

  if (error_trap_depth == 0)
    {
      fprintf (stderr, "Unexpected X error: %s serial %ld error_code %d request_code %d minor_code %d)\n",
                buf,
                error->serial, 
                error->error_code, 
                error->request_code,
                error->minor_code);

      exit (1);
    }

  return 1; /* return value is meaningless */
}

static void
error_trap_push (SnDisplay *display,
                 Display   *xdisplay)
{
  ++error_trap_depth;
}

static void
error_trap_pop (SnDisplay *display,
                Display   *xdisplay)
{
  if (error_trap_depth == 0)
    {
      fprintf (stderr, "Error trap underflow!\n");
      exit (1);
    }
  
  XSync (xdisplay, False); /* get all errors out of the queue */
  --error_trap_depth;
}

static void
mbpixbuf_to_mask (MBPixbuf    *pb,
		  MBPixbufImage *img,
		  Drawable     drw,
		  GC gc0, GC gc1,
		  int drw_x,
		  int drw_y)
{
  unsigned char *p;
  int x,y;
  int r, g, b;
  
  p = img->rgba;
  
  for (y=0; y<img->height; y++)
    {
      for (x=0; x<img->width; x++)
	{
	  r = ( *p++ );
	  g = ( *p++ );
	  b = ( *p++ );
	  XDrawPoint (xdisplay, drw, (*p++) ? gc1 : gc0, x + drw_x, y + drw_y);
	}
    }
}

void
root_window_size (Display *dpy, unsigned int *rx, unsigned int *ry)
{
  int x, y;
  unsigned int w, h, b, d;
  Window r;
  XGetGeometry (dpy, DefaultRootWindow (dpy), &r, &x, &y, &w, &h, &b, &d);
  *rx = w;
  *ry = h;
}

int
main (int argc, char **argv)
{
  SnDisplay *display;
  SnMonitorContext *context;
  int x, y, w, h;
  MBPixbufImage *img;
  static char *icon_name = PREFIX "/share/gpe/pixmaps/default/loading.png";
  GC mask_gc_0, mask_gc_1;
  Pixmap mask, pixmap;
  Atom window_type_atom, window_type_splash_atom;
  int fd;
  Window root;
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  xdisplay = XOpenDisplay (NULL);
  if (xdisplay == NULL)
    {
      fprintf (stderr, "Could not open display\n");
      return 1;
    }

  if (getenv ("LIBSN_SYNC") != NULL)
    XSynchronize (xdisplay, True);

  mbpb = mb_pixbuf_new (xdisplay, DefaultScreen (xdisplay));
  img = mb_pixbuf_img_new_from_file (mbpb, icon_name);
  if (img == NULL)
    {
      fprintf (stderr, "Couldn't load %s\n", icon_name);
      exit (1);
    }

  w = mb_pixbuf_img_get_width (img);
  h = mb_pixbuf_img_get_height (img);

  root_window_size (xdisplay, &x, &y);

  root = DefaultRootWindow (xdisplay);

  target_x = x - (w + 4);

  window = XCreateSimpleWindow (xdisplay, root,
				target_x, wy, w, h, 0, 
				BlackPixel (xdisplay, DefaultScreen (xdisplay)),
				WhitePixel (xdisplay, DefaultScreen (xdisplay)));

  window_type_atom = XInternAtom (xdisplay, "_NET_WM_WINDOW_TYPE", False);
  window_type_splash_atom = XInternAtom (xdisplay, "_NET_WM_WINDOW_TYPE_SPLASH", False);

  XChangeProperty (xdisplay, window, window_type_atom,
		   XA_ATOM, 32,  PropModeReplace,
		   (unsigned char *)&window_type_splash_atom, 1);

  XStoreName (xdisplay, window, _("Startup monitor"));

  pixmap = XCreatePixmap (xdisplay, window, w, h,
			  mbpb->depth);
  
  mb_pixbuf_img_render_to_drawable (mbpb, img, pixmap, 0, 0);

  mask = XCreatePixmap (xdisplay, window, w, h, 1);
  mask_gc_0 = XCreateGC (xdisplay, mask, 0, NULL);
  mask_gc_1 = XCreateGC (xdisplay, mask, 0, NULL);
  XSetForeground (xdisplay, mask_gc_1, WhitePixel (xdisplay, DefaultScreen (xdisplay)));
  mbpixbuf_to_mask (mbpb, img, mask, mask_gc_0, mask_gc_1, 0, 0);

  XShapeCombineMask (xdisplay, window, ShapeBounding, 0, 0, mask, ShapeSet);

  XSetWindowBackgroundPixmap (xdisplay, window, pixmap);

  XSetErrorHandler (x_error_handler);

  /* We have to select for property events on at least one
   * root window (but not all as INITIATE messages go to
   * all root windows)
   */
  XSelectInput (xdisplay, root, PropertyChangeMask | StructureNotifyMask);
  
  display = sn_display_new (xdisplay,
                            error_trap_push,
                            error_trap_pop);

  context = sn_monitor_context_new (display, DefaultScreen (xdisplay),
                                    monitor_event_func,
                                    NULL, NULL);  

  fd = ConnectionNumber (xdisplay);
  
  for (;;)
    {
      XEvent xevent;

      while (launch_list && ! XPending (xdisplay))
	{
	  struct timeval tv;
	  fd_set fds;
 
	  tv.tv_sec = POLLTIME;
	  tv.tv_usec = 0;

	  FD_ZERO (&fds);
	  FD_SET (fd, &fds);

	  if (select (fd + 1, &fds, NULL, NULL, &tv) == 0)
	    {
	      GSList *iter, *next;
	      time_t t = time (NULL);

	      for (iter = launch_list; iter; iter = next)
		{
		  struct launch *l = (struct launch *)iter->data;

		  next = iter->next;
 
		  if ((l->when - t) <= 0)
		    {
		      g_free (l->id);
		      g_free (l);
		      launch_list = g_slist_remove_link (launch_list, iter);
		      g_slist_free_1 (iter);
		    }
		}

	      if (launch_list == NULL && hourglass_shown)
		hide_hourglass ();
	    }
	  else
	    break;
	}

      XNextEvent (xdisplay, &xevent);

      switch (xevent.type)
	{
	case Expose:
	  XClearWindow (xdisplay, window);
	  break;
	case ConfigureNotify:
	  if (xevent.xconfigure.window == root)
	    target_x = xevent.xconfigure.width - (w + 4);
	  break;
	}

      sn_display_process_event (display, &xevent);
    }

  sn_monitor_context_unref (context);
  
  return 0;
}
