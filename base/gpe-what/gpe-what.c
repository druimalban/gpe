/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <glib.h>

#include <pango/pango.h>
#include <pango/pangoxft.h>

#include "mbtray.h"
#include "mbpixbuf.h"

#define _(x) gettext(x)

static Atom help_atom;
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

static Pixmap pixmap;

#define XPADDING 8
#define YPADDING 4

static gboolean active;

static void
redraw_popup (void)
{
  GSList *lines;
  
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
popup_box (const char *text, int x, int y)
{
  PangoRectangle ink_rect;
  unsigned int w, h;
  XSetWindowAttributes attr;
  Pixmap mask;
  GC gc0, gc1;

  close_popup ();

  pango_layout_set_text (pango_layout, text, strlen (text));

  pango_layout_get_pixel_extents (pango_layout, &ink_rect, NULL);
  w = ink_rect.width + (2 * XPADDING);
  h = ink_rect.height + (2 * YPADDING);

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

  XFillArc (dpy, mask, gc1, 0, 0, XPADDING * 2, YPADDING * 2, 90 * 64, 180 * 64);
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
}

static gboolean
handle_click (Window w, Window orig_w)
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
      
      return (root == parent) ? FALSE : handle_click (parent, orig_w);
    }

  if (prop)
    XFree (prop);

  event.type = ClientMessage;
  event.window = w;
  event.message_type = help_atom;
  event.format = 32;
  memset (&event.data.b[0], 0, sizeof (event.data.b));
  event.data.l[0] = win_panel;

  XSendEvent (dpy, w, False, 0, (XEvent *)&event);

  return TRUE;
}

static Window 
find_deepest_window (Display *dpy, Window grandfather, Window parent,
		     int x, int y)
{
  int dest_x, dest_y;
  Window child;
  
  XTranslateCoordinates (dpy, grandfather, parent, x, y,
			 &dest_x, &dest_y, &child);

  return (child == None) ? parent : find_deepest_window(dpy, parent, child, dest_x, dest_y);
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
  PangoFontMap *fontmap;
  PangoFontDescription *fontdes;
  XRenderColor colortmp;
  XColor c;
  MBPixbufImage *img_icon;
  int last_x = 0, last_y = 0;
  Atom string_atom;

  g_type_init ();

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

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

  prepare_icon (img_icon, pixmap);
  XSetWindowBackgroundPixmap (dpy, win_panel, pixmap);

  help_atom = XInternAtom (dpy, "GPE_INTERACTIVE_HELP", False);
  string_atom = XInternAtom (dpy, "STRING", False);
  
  pango_ctx = pango_xft_get_context (dpy, DefaultScreen (dpy));
  pango_layout = pango_layout_new (pango_ctx);

  fontmap = pango_xft_get_font_map (dpy, DefaultScreen (dpy));
  fontdes = pango_font_description_new ();

  pango_font_description_set_family (fontdes, "Verdana");
  pango_font_description_set_size (fontdes, 6 * PANGO_SCALE);
  
  pango_font = pango_font_map_load_font (fontmap, pango_ctx, fontdes);
  pango_metrics = pango_font_get_metrics (pango_font, NULL);

  pango_layout_set_font_description (pango_layout, fontdes);
  pango_layout_set_justify (pango_layout, TRUE);

  pango_layout_set_width (pango_layout, 120 * PANGO_SCALE);

  colortmp.red   = 0xffff;
  colortmp.green = 0xffff;
  colortmp.blue  = 0xffff;
  colortmp.alpha = 0xffff;

  XftColorAllocValue (dpy, DefaultVisual(dpy, screen),
		      DefaultColormap (dpy, screen),
		      &colortmp, &fg_xftcol);

  c.red   = 0;
  c.green = 0;
  c.blue  = 0xffff;

  XAllocColor (dpy, DefaultColormap (dpy, screen), &c);
  bgcol = c.pixel;

  XSelectInput (dpy, win_panel, ExposureMask | ButtonPressMask | StructureNotifyMask | PropertyChangeMask);

  for (;;)
    {
      XEvent xev;
      XNextEvent (dpy, &xev);

      switch (xev.type)
	{
	case Expose:
	  if (xev.xexpose.window == win_popup)
	    redraw_popup ();
	  break;

	case ButtonPress:
	  if (xev.xbutton.window == win_popup)
	    close_popup ();
	  else if (active)
	    {
	      Window w;

	      XUngrabPointer (dpy, xev.xbutton.time);

	      w = find_deepest_window (dpy, DefaultRootWindow (dpy), DefaultRootWindow (dpy),
				       xev.xbutton.x_root, xev.xbutton.y_root);

	      last_x = xev.xbutton.x_root;
	      last_y = xev.xbutton.y_root;
		 
	      if (handle_click (w, w) == FALSE)
		popup_box (_("No help available."), xev.xbutton.x_root, xev.xbutton.y_root);
	      
	      active = FALSE;
	    }
	  else
	    {
	      active = TRUE;
	      XGrabPointer (dpy, win_panel, False, ButtonPressMask,
			    GrabModeAsync, GrabModeAsync,
			    None, None, xev.xbutton.time);
	    }
	  break;

	case PropertyNotify:
	  if (xev.xproperty.window == win_panel
	      && xev.xproperty.atom == help_atom)
	    {
	      unsigned char *prop;
	      unsigned long nitems, bytes_after;
	      Atom type;
	      int format;

	      if (XGetWindowProperty (dpy, win_panel, help_atom, 0, 65536, False,
				      string_atom, &type, &format, &nitems, &bytes_after, &prop) == Success)
		{
		  popup_box (prop, last_x, last_y);
		  XFree (prop);
		}
	    }
	  break;
	}

      mb_tray_handle_event (dpy, win_panel, &xev);
    }

  exit (0);
}
