/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <glib.h>

#include <pango/pango.h>
#include <pango/pangoxft.h>

#include "mbtray.h"

#define _(x) gettext(x)

static Atom help_atom;
static Atom infoprint_atom;
static Display *dpy;
static int screen;

static Window win_popup;
static Window win_panel;

static PangoLayout *pango_layout;
static PangoFont *pango_font;
static PangoFontMetrics *pango_metrics;

static XftDraw *xftdraw;

static MBPixbuf *mbpixbuf;

static unsigned long bgcol;
static XftColor fg_xftcol;

static Pixmap pixmap, pixmap_active;

#define XPADDING 8
#define YPADDING 4

static struct timeval close_time;

#define TIMEOUT 10
#define INFOTIMEOUT 2

static MBPixbufImage *img_icon, *img_icon_active;

static void
redraw_popup (void)
{
  GSList *lines;

  XClearWindow (dpy, win_popup);
  
  for (lines = pango_layout_get_lines (pango_layout); lines; lines = lines->next)
    {
      PangoLayoutLine *this = lines->data;
      GSList *run;
      
      for (run = this->runs; run; run = run->next)
	{
	  PangoLayoutRun *this = run->data;
	  PangoRectangle rect;
	  
	  pango_layout_index_to_pos (pango_layout, this->item->offset, &rect);

	  pango_xft_render (xftdraw, &fg_xftcol, pango_font, this->glyphs,
			    XPADDING + rect.x / PANGO_SCALE,
			    YPADDING + (rect.y + pango_font_metrics_get_ascent (pango_metrics)) / PANGO_SCALE);
	}
    }
}

static void
close_popup (void)
{
  if (win_popup)
    {
      XftDrawDestroy (xftdraw);
      XDestroyWindow (dpy, win_popup);
      win_popup = None;
    }
}

static void
popup_box (const char *text, int length, int x, int y, int timeout)
{
  PangoRectangle ink_rect;
  unsigned int w, h;
  XSetWindowAttributes attr;
  Pixmap mask;
  GC gc0, gc1;
  unsigned int root_w, root_h;

  if (length < 0)
    length = strlen (text);

  close_popup ();

  {
    unsigned int x, y, b, d;
    Window r;
    XGetGeometry (dpy, DefaultRootWindow (dpy), &r, &x, &y, &root_w, &root_h, &b, &d);
  }

  pango_layout_set_text (pango_layout, text, length);

  pango_layout_get_pixel_extents (pango_layout, &ink_rect, NULL);
  w = ink_rect.width + (2 * XPADDING);
  h = ink_rect.height + (2 * YPADDING);

  if (x < 0 && y < 0)
    {
      x = root_w - w - XPADDING;
      y = YPADDING;
    }
  else
    {
      if (x + w >= root_w)
	x -= w;
      if (y + h >= root_h)
	y -= h;
      
      if (x < 0)
	x = 0;
      if (y < 0)
	y = 0;
    }

  win_popup = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), x, y, w, h, 0, 0, bgcol);
  mask = XCreatePixmap (dpy, win_popup, w, h, 1);

  gc0 = XCreateGC (dpy, mask, 0, NULL);
  gc1 = XCreateGC (dpy, mask, 0, NULL);

  XSetForeground (dpy, gc1, WhitePixel (dpy, screen));

  XFillRectangle (dpy, mask, gc0, 0, 0, w, h);

  XFillRectangle (dpy, mask, gc1, XPADDING, YPADDING, ink_rect.width, ink_rect.height);

  XFillRectangle (dpy, mask, gc1, XPADDING, 0, ink_rect.width, YPADDING);
  XFillRectangle (dpy, mask, gc1, XPADDING, ink_rect.height + YPADDING, ink_rect.width, YPADDING);
  XFillRectangle (dpy, mask, gc1, 0, YPADDING, XPADDING, ink_rect.height);
  XFillRectangle (dpy, mask, gc1, ink_rect.width + XPADDING, YPADDING, XPADDING, ink_rect.height);

  XFillArc (dpy, mask, gc1, 0, 0, XPADDING * 2, YPADDING * 2, 90 * 64, 140 * 64);
  XFillArc (dpy, mask, gc1, ink_rect.width, 0, XPADDING * 2, YPADDING * 2, 0 * 64, 90 * 64);
  XFillArc (dpy, mask, gc1, 0, ink_rect.height, XPADDING * 2, YPADDING * 2, 180 * 64, 270 * 64);
  XFillArc (dpy, mask, gc1, ink_rect.width, ink_rect.height, XPADDING * 2, YPADDING * 2, 0 * 64, -90 * 64);

  XFreeGC (dpy, gc0);
  XFreeGC (dpy, gc1);

  XShapeCombineMask (dpy, win_popup, ShapeBounding, 0, 0, mask, ShapeSet);

  XFreePixmap (dpy, mask);

  attr.override_redirect = True;
  XChangeWindowAttributes (dpy, win_popup, CWOverrideRedirect, &attr);

  xftdraw = XftDrawCreate (dpy, win_popup,
			   DefaultVisual (dpy, screen),
			   DefaultColormap (dpy, screen));

  XSelectInput (dpy, win_popup, ExposureMask | ButtonPressMask);

  XMapRaised (dpy, win_popup);

  XSync (dpy, 0);

  gettimeofday (&close_time, NULL);
  close_time.tv_sec += timeout;
}

static gboolean
handle_click (Window w, Window orig_w, int x, int y)
{
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  XClientMessageEvent event;

  if (XGetWindowProperty (dpy, w, help_atom, 0, 0, False,
			  None, &type, &format, &nitems, &bytes_after, &prop) != Success
      || type == None)
    {
      Window root, parent, *children;
      unsigned int nchildren;  
  
      XQueryTree (dpy, w, &root, &parent, &children, &nchildren);
      if (children)
	XFree (children);

      if (root != w && root != parent)
	{      
	  unsigned int px, py, b, d, root_w, root_h;
	  Window r;
	  XGetGeometry (dpy, w, &r, &px, &py, &root_w, &root_h, &b, &d);

	  return handle_click (parent, orig_w, x + px, y + py);
	}

      return FALSE;
    }

  if (prop)
    XFree (prop);

  event.type = ClientMessage;
  event.window = w;
  event.message_type = help_atom;
  event.format = 32;
  memset (&event.data.b[0], 0, sizeof (event.data.b));
  event.data.l[0] = win_panel;
  event.data.l[1] = x;
  event.data.l[2] = y;
  event.data.l[3] = help_atom;

  XSendEvent (dpy, w, False, 0, (XEvent *)&event);

  return TRUE;
}

static Window 
find_deepest_window (Display *dpy, Window grandfather, Window parent,
		     int x, int y, int *rx, int *ry)
{
  int dest_x, dest_y;
  Window child;
  
  XTranslateCoordinates (dpy, grandfather, parent, x, y,
			 &dest_x, &dest_y, &child);

  if (child == None)
    {
      *rx = dest_x;
      *ry = dest_y;

      return parent;
    }
  
  return find_deepest_window(dpy, parent, child, dest_x, dest_y, rx, ry);
}

static void
prepare_icon (MBPixbufImage *img_icon, Pixmap pix)
{
  XColor xcol_bg;
  MBPixbufImage *img_backing;
  xcol_bg.red   = 0;
  xcol_bg.green = 0;
  xcol_bg.blue  = 0;
  mb_tray_get_bg_col (dpy, &xcol_bg);
  img_backing = mb_pixbuf_img_new (mbpixbuf, 
				   mb_pixbuf_img_get_width (img_icon), 
				   mb_pixbuf_img_get_height (img_icon));
  mb_pixbuf_img_fill (mbpixbuf, img_backing, 
		      xcol_bg.red, xcol_bg.green, xcol_bg.blue, 0);
  mb_pixbuf_img_composite (mbpixbuf, img_backing, img_icon, 0, 0);
  mb_pixbuf_img_render_to_drawable (mbpixbuf, img_backing, pix, 0, 0);
  mb_pixbuf_img_free (mbpixbuf, img_backing);
}

int
main (int argc, char *argv[])
{
  PangoContext *pango_ctx;
  PangoFontDescription *fontdes;
  XRenderColor colortmp;
  XColor c;
  int last_x = 0, last_y = 0;
  int xfd;
  int state = 0;

  g_type_init ();

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  dpy = XOpenDisplay (NULL);
  if (dpy == NULL)
    {
      fprintf (stderr, _("Cannot connect to X server\n"));
      exit (1);
    }

  screen = DefaultScreen (dpy);

  mbpixbuf = mb_pixbuf_new (dpy, screen);

  img_icon = mb_pixbuf_img_new_from_file (mbpixbuf, PREFIX "/share/pixmaps/gpe-what.png");
  if (img_icon == NULL)
    {
      fprintf (stderr, _("Failed to load icon\n"));
      exit (1);
    }
  img_icon_active = mb_pixbuf_img_new_from_file (mbpixbuf, PREFIX "/share/pixmaps/gpe-what-active.png");
  if (img_icon_active == NULL)
    {
      fprintf (stderr, _("Failed to load icon\n"));
      exit (1);
    }

  win_panel = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), 0, 0,
				   mb_pixbuf_img_get_width (img_icon), 
				   mb_pixbuf_img_get_height (img_icon), 
				   0,
				   BlackPixel (dpy, screen),
				   WhitePixel (dpy, screen));

  XStoreName (dpy, win_panel, _("Interactive help"));

  mb_tray_init_session_info (dpy, win_panel, argv, argc);
  mb_tray_init (dpy, win_panel);
  mb_tray_window_icon_set (dpy, win_panel, img_icon);

  pixmap = XCreatePixmap (dpy, win_panel, 
			  mb_pixbuf_img_get_width (img_icon), 
			  mb_pixbuf_img_get_height (img_icon),
			  mbpixbuf->depth);

  pixmap_active = XCreatePixmap (dpy, win_panel, 
				 mb_pixbuf_img_get_width (img_icon), 
				 mb_pixbuf_img_get_height (img_icon),
				 mbpixbuf->depth);

  prepare_icon (img_icon, pixmap);
  prepare_icon (img_icon_active, pixmap_active);

  mb_pixbuf_img_free (mbpixbuf, img_icon);
  mb_pixbuf_img_free (mbpixbuf, img_icon_active);

  XSetWindowBackgroundPixmap (dpy, win_panel, pixmap);
  XClearWindow (dpy, win_panel);

  help_atom = XInternAtom (dpy, "_GPE_INTERACTIVE_HELP", False);
  infoprint_atom = XInternAtom (dpy, "_GPE_INFOPRINT", False);
  
  pango_ctx = pango_xft_get_context (dpy, DefaultScreen (dpy));

  fontdes = pango_font_description_new ();

  pango_font_description_set_family_static (fontdes, "Sans");
  pango_font_description_set_size (fontdes, 8 * PANGO_SCALE);
  
  pango_context_set_font_description (pango_ctx, fontdes);

  pango_font = pango_context_load_font (pango_ctx, fontdes);
  pango_metrics = pango_font_get_metrics (pango_font, NULL);

  pango_layout = pango_layout_new (pango_ctx);
  pango_layout_set_justify (pango_layout, TRUE);

  pango_layout_set_width (pango_layout, 180 * PANGO_SCALE);

  colortmp.red   = 0;
  colortmp.green = 0;
  colortmp.blue  = 0;
  colortmp.alpha = 0xffff;

  XftColorAllocValue (dpy, DefaultVisual(dpy, screen),
		      DefaultColormap (dpy, screen),
		      &colortmp, &fg_xftcol);

  XParseColor (dpy, DefaultColormap (dpy, screen), "gold", &c);
  XAllocColor (dpy, DefaultColormap (dpy, screen), &c);
  bgcol = c.pixel;

  XSetSelectionOwner (dpy, infoprint_atom, win_panel, CurrentTime);

  XChangeProperty (dpy, win_panel, help_atom, XA_STRING, 8, PropModeReplace, NULL, 0);

  XSelectInput (dpy, win_panel, ButtonPressMask | PropertyChangeMask);

  xfd = ConnectionNumber (dpy);

  for (;;)
    {
      XEvent xev;

      if (win_popup != None && ! XPending (dpy))
	{
	  struct timeval now, tvt;
	  fd_set fd;

	  gettimeofday (&now, NULL);

	  tvt.tv_usec = close_time.tv_usec - now.tv_usec;
	  tvt.tv_sec = close_time.tv_sec - now.tv_sec;
	  if (tvt.tv_usec < 0)
	    {
	      if (tvt.tv_sec <= 0)
		{
		  close_popup ();
		  continue;
		}
	      tvt.tv_sec --;
	      tvt.tv_usec += 1000000;
	    }

	  FD_ZERO (&fd);
	  FD_SET (xfd, &fd);	  

	  if (select (xfd+1, &fd, NULL, NULL, &tvt) == 0)
	    close_popup ();
	}

      XNextEvent (dpy, &xev);

      switch (xev.type)
	{
	case Expose:
	  redraw_popup ();
	  break;

	case ButtonPress:
	  close_popup ();

	  if (xev.xbutton.window == win_panel)
	    {
	      if (state == 1)
		{
		  Window w;
		  int x, y;
		  
		  XSetWindowBackgroundPixmap (dpy, win_panel, pixmap);
		  XClearWindow (dpy, win_panel);

		  w = find_deepest_window (dpy, DefaultRootWindow (dpy), DefaultRootWindow (dpy),
					   xev.xbutton.x_root, xev.xbutton.y_root, &x, &y);
		  
		  last_x = xev.xbutton.x_root;
		  last_y = xev.xbutton.y_root;
		  
		  if (handle_click (w, w, x, y) == FALSE)
		    popup_box (_("No help available here."), -1, xev.xbutton.x_root, xev.xbutton.y_root, TIMEOUT);

		  state = 2;
		}
	      else
		{
		  XSetWindowBackgroundPixmap (dpy, win_panel, pixmap_active);
		  XClearWindow (dpy, win_panel);

		  XGrabPointer (dpy, win_panel, False, ButtonPressMask | ButtonReleaseMask,
				GrabModeAsync, GrabModeAsync,
				None, None, xev.xbutton.time);
		  state = 1;
		}
	    }
	  else
	    close_popup ();
	  break;

	case ButtonRelease:
	  if (state == 2)
	    {
	      XUngrabPointer (dpy, xev.xbutton.time);
	      state = 0;
	    }
	  break;

	case ClientMessage:
	  if (xev.xclient.message_type == help_atom)
	    popup_box (_("This is the interactive help button.  Tap here and then on another icon to get help."), -1, last_x, last_y, TIMEOUT);
	  break;

	case PropertyNotify:
	  if (xev.xproperty.atom == help_atom)
	    {
	      unsigned char *prop;
	      unsigned long nitems, bytes_after;
	      Atom type;
	      int format;

	      if (XGetWindowProperty (dpy, win_panel, help_atom, 0, 65536, False,
				      XA_STRING, &type, &format, &nitems, &bytes_after, &prop) == Success)
		{
		  popup_box (prop, nitems, last_x, last_y, TIMEOUT);
		  XFree (prop);
		}
	    }
	  else if (xev.xproperty.atom == infoprint_atom)
	    {
	      unsigned char *prop;
	      unsigned long nitems, bytes_after;
	      Atom type;
	      int format;

	      if (XGetWindowProperty (dpy, win_panel, infoprint_atom, 0, 65536, False,
				      XA_STRING, &type, &format, &nitems, &bytes_after, &prop) == Success)
		{
		  popup_box (prop, nitems, -1, -1, INFOTIMEOUT);
		  XFree (prop);
		}
	    }
	  break;
	}

      mb_tray_handle_event (dpy, win_panel, &xev);
    }

  exit (0);
}
