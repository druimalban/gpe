/*
 * minibat -- GPE battery monitor
 *
 * Copyright 2004 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libintl.h>
#include <stdint.h>
#include <apm.h>

#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#include <libmb/mbpixbuf.h>

#include "findargb.h"
#include "mbtray.h"

#define _(x) gettext(x)

#define PIXMAPSDIR PREFIX "/share/pixmaps"

#define IMG_PANEL	0
#define IMG_PANEL_AC	1
#define IMG_POPUP	2
#define NR_PIXBUFS	3

struct
{
  Window window;
  Pixmap pixmap;
  cairo_t *cr;
  cairo_surface_t *surface;
  int width, height;
  GC gc;
  int img_offset;
} popup;

struct
{
  Window window;
  Pixmap pixmap;
  GC gc;
  int width, height;
  int depth;
} panel;

MBPixbuf *mbpixbuf;
MBPixbufImage *pixbufs[NR_PIXBUFS];

const char *pixbuf_names[] =
  {
    PIXMAPSDIR "/minibat.png",
    PIXMAPSDIR "/minibat-ac.png",
    PIXMAPSDIR "/minibat-battery.png"
  };

static void
clear (cairo_t	*cr, double width, double height, double alpha)
{
  cairo_save (cr);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint_with_alpha(cr, alpha);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);
  cairo_restore (cr);
}

void
draw_state (void)
{
  cairo_move_to (popup.cr, 0, 0);
  clear (popup.cr, popup.width, popup.img_offset, 0);

  cairo_set_source_rgb (popup.cr, 1, 1, 1);
  cairo_paint_with_alpha (popup.cr, 0.8);
  cairo_new_path (popup.cr);

#define CORNER 5

  cairo_move_to (popup.cr, CORNER, 0);
  cairo_line_to (popup.cr, popup.width - CORNER, 0);
  cairo_curve_to (popup.cr, 
		  popup.width, 0,
		  popup.width, 0,
		  popup.width, CORNER);
  cairo_line_to (popup.cr, popup.width, popup.img_offset - CORNER);
  cairo_curve_to (popup.cr, 
		  popup.width, popup.img_offset,
		  popup.width, popup.img_offset,
		  popup.width - CORNER, popup.img_offset);
  cairo_line_to (popup.cr, CORNER, popup.img_offset);
  cairo_curve_to (popup.cr, 
		  0, popup.img_offset,
		  0, popup.img_offset,
		  0, popup.img_offset - CORNER);
  cairo_line_to (popup.cr, 0, CORNER);
  cairo_curve_to (popup.cr, 
		  0, 0,
		  0, 0,
		  CORNER, 0);

  cairo_close_path (popup.cr);
  cairo_fill (popup.cr);

  cairo_select_font_face (popup.cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_source_rgb (popup.cr, 0, 0, 0);
  cairo_paint_with_alpha (popup.cr, 1.0);
  cairo_move_to (popup.cr, CORNER * 2, popup.img_offset / 2);
  cairo_set_font_size (popup.cr, 10);
  cairo_show_text (popup.cr, "75%");
}

void
initialize_popup (Display *dpy, Visual *visual, Window root, Colormap cmap)
{
  XSetWindowAttributes wattr;
  unsigned long wmask;
  cairo_surface_t *img_surface;
  XGCValues gcv;
  
  popup.width = 48;
  popup.height = 64;

  memset (&wattr, 0, sizeof (wattr));
  wattr.background_pixel = 0;
  wattr.border_pixel = 0;
  wattr.colormap = cmap;
  wattr.override_redirect = True;
  wattr.event_mask = ExposureMask | StructureNotifyMask;
  wmask = CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask;

  gcv.graphics_exposures = False;

  popup.window = XCreateWindow (dpy, root, 0, 0, popup.width, popup.height, 0,
				32, InputOutput, visual, wmask, &wattr);
  popup.gc = XCreateGC (dpy, popup.window, GCGraphicsExposures, &gcv);
  popup.pixmap = XCreatePixmap (dpy, popup.window, popup.width, popup.height, 32);
  popup.surface = cairo_xlib_surface_create (dpy, popup.pixmap, visual, 0, cmap);

  //cairo_set_target_surface (popup.cr, popup.surface);

  clear (popup.cr, popup.width, popup.height, 0);

  popup.img_offset = popup.height - pixbufs[IMG_POPUP]->height;

  img_surface = cairo_image_surface_create_for_data (pixbufs[IMG_POPUP]->rgba,
						CAIRO_FORMAT_ARGB32,
						pixbufs[IMG_POPUP]->width,
						pixbufs[IMG_POPUP]->height,
						pixbufs[IMG_POPUP]->width * 4);
  
  popup.cr = cairo_create (img_surface);
  cairo_translate (popup.cr, 0, popup.img_offset);
  cairo_set_source_surface (popup.cr, img_surface, pixbufs[IMG_POPUP]->width, pixbufs[IMG_POPUP]->height);
  cairo_translate (popup.cr, 0, -popup.img_offset);

  draw_state ();

  XMapWindow (dpy, popup.window);
}

void
initialize_panel (Display *dpy, Visual *visual, Window root, Colormap cmap)
{
  XSetWindowAttributes wattr;
  unsigned long wmask;
  XGCValues gcv;
  cairo_t *cr;
  cairo_surface_t *surface;
  cairo_surface_t *img_surface;

  panel.width = pixbufs[IMG_PANEL]->width;
  panel.height = pixbufs[IMG_PANEL]->height;
  panel.depth = DefaultDepth (dpy, 0);

  memset (&wattr, 0, sizeof (wattr));
  wattr.background_pixel = 0;
  wattr.border_pixel = 0;
  wattr.colormap = cmap;
  wattr.event_mask = StructureNotifyMask | ExposureMask
    | ButtonPressMask | ButtonReleaseMask | VisibilityChangeMask;
  wmask = CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask;

  panel.window = XCreateWindow (dpy, root, 0, 0, panel.width, panel.height, 0,
				panel.depth, InputOutput, visual, wmask, &wattr);
  
  panel.pixmap = XCreatePixmap (dpy, panel.window, panel.width, panel.height, panel.depth);

  panel.gc = XCreateGC (dpy, panel.window, 0, &gcv);


  surface = cairo_xlib_surface_create (dpy, panel.pixmap, visual, 0, cmap);

  img_surface = cairo_image_surface_create_for_data (pixbufs[IMG_PANEL]->rgba,
						CAIRO_FORMAT_ARGB32,
						pixbufs[IMG_PANEL]->width,
						pixbufs[IMG_PANEL]->height,
						pixbufs[IMG_PANEL]->width * 4);

  cr = cairo_create (img_surface);

  cairo_set_source_surface (cr, img_surface, pixbufs[IMG_PANEL]->width, pixbufs[IMG_PANEL]->height);

  cairo_destroy (cr);

  XSetWindowBackgroundPixmap (dpy, panel.window, panel.pixmap);
}

void
minibat (Display *dpy, int scr, Visual *visual, Window root, int argc, char *argv[])
{
  int xfd;
  XEvent xevent;
  Colormap cmap;
  int transparent_panel = 0;

  cmap = XCreateColormap (dpy, root, visual, AllocNone);

  initialize_panel (dpy, transparent_panel ? visual : DefaultVisual (dpy, scr), root, 
		    transparent_panel ? cmap : DefaultColormap (dpy, scr));
  initialize_popup (dpy, visual, root, cmap);

  mb_tray_init_session_info (dpy, panel.window, argv, argc);
  mb_tray_init (dpy, panel.window);

  xfd = ConnectionNumber (dpy);

  for (;;)
    {
      int visible = 1;
      struct timeval tvt;
      fd_set fd;

      if (visible && ! XPending (dpy))
	{
	  tvt.tv_usec = 0;
	  tvt.tv_sec = 2; 		/* check batt stats every 2 seconds  */
      
	  FD_ZERO (&fd);
	  FD_SET (xfd, &fd);

	  if (select (xfd+1, &fd, NULL, NULL, &tvt) == 0)
	    {
	      /* repaint */
	    }
	}

      XNextEvent (dpy, &xevent);

      switch (xevent.type) 
	{
	case Expose:
	  XCopyArea (dpy, popup.pixmap, popup.window,
		     popup.gc, 0, 0, popup.width, popup.height, 0, 0);
	  break;
	}
      
      mb_tray_handle_event (dpy, panel.window, &xevent);
    }
}

int 
main (int argc, char **argv)
{
  char *dpy_name = NULL;
  int c;
  int scr;
  int i;
  Display *dpy;
  Visual *visual;
  Window root;

  setlocale (LC_ALL, "");

  while ((c = getopt (argc, argv, "d:")) > 0)
    {
      switch (c) 
	{
	case 'd':
	  dpy_name = optarg;
	  break;
	default:
	  fprintf (stderr, "usage: %s [-d <dpy>]\n", argv[0]);
	  exit (1);
	  break;
	}
    }

  if ((dpy = XOpenDisplay (dpy_name)) == NULL)
    {
      fprintf (stderr, _("Cannot connect to X server on display %s\n"),
	       XDisplayName (dpy_name));
      exit (1);
    }

  scr = DefaultScreen (dpy);
  root = RootWindow (dpy, scr);
  visual = find_argb_visual (dpy, scr);

  if (visual == NULL)
    {
      fprintf (stderr, "%s: can't find an ARGB visual\n", argv[0]);
      exit (1);
    }

  mbpixbuf = mb_pixbuf_new (dpy, scr);

  for (i = 0; i < NR_PIXBUFS; i++)
    {
      pixbufs[i] = mb_pixbuf_img_new_from_file (mbpixbuf, pixbuf_names[i]);
      if (pixbufs[i] == NULL)
	{
	  fprintf (stderr, "Can't load %s\n", pixbuf_names[i]);
	  exit (1);
	}
    }

  minibat (dpy, scr, visual, root, argc, argv);

  exit (0);
}
