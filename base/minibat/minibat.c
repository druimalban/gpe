/* minibat - A tiny battery monitor

   Copyright 2002 Matthew Allum
   Copyright (c) 2002, 2003 Phil Blundell

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libintl.h>

#define _(x) gettext(x)

#define PIXMAPSDIR PREFIX "/share/pixmaps"

#ifdef HAVE_APM_H 		/* Linux */
#include <apm.h>
#endif

#ifdef HAVE_APMVAR_H		/* *BSD  */
#include <err.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <machine/apmvar.h>

#define APMDEV "/dev/apm"

enum apm_state {
    NORMAL,
    SUSPENDING,
    STANDING_BY
};

struct apm_reply {
    int vno;
    enum apm_state newstate;
    struct apm_power_info batterystate;
};

#endif

#define USE_OVERRIDE_REDIRECT

#define TIME_LEFT   0
#define PERCENTAGE  1
#define AC_POWER    2
#define BATT_STATUS 3

#ifndef AC_LINE_STATUS_ON
#define AC_LINE_STATUS_ON 1
#endif

#ifndef BATTERY_STATUS_CHARGING
#define BATTERY_STATUS_CHARGING 1
#endif

#include <mb/mbtray.h>
#include <mb/mbpixbuf.h>

#include <X11/Xft/Xft.h>

static Display* dpy;	
static Window win_panel, win_root;
static int screen;
static GC gc;
static int apm_vals[4];
static MBPixbuf *pb;
static MBPixbufImage *img_icon = NULL, *img_icon_pwr = NULL, *img_backing = NULL; 
static XColor xcol_bg;
static Pixmap pxm_backing = None;

static int time_left_alerts[] = { 0, 2, 5, 10, 20 }; /* times to alert on */
static int time_left_idx = 4;

static Bool ac_power = False;

#define popup_w 48

static Window win_popup;
static Bool popup_visible;
static MBPixbufImage *img_icon_popup;
static Pixmap popup_mask, popup_pixmap;
static GC popup_gc;
static int text_offset;
static int popup_h;

static XftFont *msg_font;
static XftColor fg_xftcol;

static Atom interactive_help_atom;

#ifdef HAVE_APM_H

static int 
read_apm(int *values)
{
  /* add stat function here ? */
 
  apm_info info;
  apm_read(&info);

  values[TIME_LEFT] = info.battery_time;
  values[PERCENTAGE] = info.battery_percentage;
  values[AC_POWER] = info.ac_line_status;
  values[BATT_STATUS] = info.battery_status;

  return 1;
}

#else  /* BSD */

static int 
read_apm(int *values)
{
  int fd;
  struct apm_reply reply;
  struct apm_power_info *api = &reply.batterystate;

  /* open the device directly and get status */
  fd = open(APMDEV, O_RDONLY);
  if (fd == -1) 
	return 0;
  memset(&reply, 0, sizeof(reply));
  if (ioctl(fd, APM_IOC_GETPOWER, &reply.batterystate) == -1) {
        close (fd);
  return 0;
  }
  close (fd);

  values[TIME_LEFT]  = api->minutes_left;
  values[PERCENTAGE] = api->battery_life;
  values[AC_POWER]   = api->ac_state;
  values[BATT_STATUS] = 0;	/* XXX */
 
  return 1;
}

#endif

static void
paint (void)
{
  int power_pixels = 0;
  Bool old_power = ac_power;
  unsigned char r = 0, g = 0, b = 0;
  int x, y;

  while (!read_apm(apm_vals))
    usleep(50000L);

  if (apm_vals[PERCENTAGE] <= 30)
    { r = 0xff; g = 0; b = 0; }
  else if (apm_vals[PERCENTAGE] < 75)
    { r = 0xff; g = 0x99; b = 0x33; }
  else
    { r = 0x66; g = 0xff; b = 0x33; }

  if (apm_vals[AC_POWER] == AC_LINE_STATUS_ON) 
    ac_power = True;
  else 
    ac_power = False;

  if (old_power != ac_power)
    {
      mb_pixbuf_img_fill(pb, img_backing, 
			 xcol_bg.red, xcol_bg.green, xcol_bg.blue, 0);
      if (ac_power)
	mb_pixbuf_img_composite(pb, img_backing, img_icon_pwr , 0, 0);
      else
	mb_pixbuf_img_composite(pb, img_backing, img_icon , 0, 0);
    }

  /* Clear out bar first */
  for ( y = 13; y < 15; y++) 
    for ( x = 3; x < 14; x++)
      if (ac_power)
	{
	  mb_pixbuf_img_set_pixel(img_backing, x, y, 0xff, 0xff, 0);
	}
      else
	{
	  mb_pixbuf_img_set_pixel(img_backing, x, y, 0, 0, 0);
	}

  if (apm_vals[PERCENTAGE] > 0)
    { 
      power_pixels = apm_vals[PERCENTAGE]/10;

      for ( y = 13; y < 15; y++) 
	for ( x = 3; x < 4 + power_pixels; x++)
	    mb_pixbuf_img_set_pixel(img_backing, x, y, r, g, b);
    }

  /* Bubble alerts  */
  if ((time_left_idx > 0) 
      && !ac_power
      && apm_vals[PERCENTAGE] > 0
      && apm_vals[TIME_LEFT]  > 0
      && (apm_vals[TIME_LEFT] < time_left_alerts[time_left_idx]))
    {
      char tray_msg[256];
      sprintf(tray_msg, 
	      _("Battery power very low!\n\nTime left: %.2i minutes"), 
	      time_left_alerts[time_left_idx]);
      mb_tray_send_message(dpy, win_panel, tray_msg, 0);
      time_left_idx--;
    }
  else if (time_left_idx < 4 
	   && apm_vals[TIME_LEFT] > time_left_alerts[time_left_idx+1])
    {
      time_left_idx++;
    }

  mb_pixbuf_img_render_to_drawable(pb, img_backing, pxm_backing, 0, 0);

  XSetWindowBackgroundPixmap(dpy, win_panel, pxm_backing);
  XClearWindow(dpy, win_panel);
}

static void
set_backing (void *user_data)
{
  mb_tray_get_bg_col (dpy, &xcol_bg);
  mb_pixbuf_img_fill (pb, img_backing, xcol_bg.red, xcol_bg.green, xcol_bg.blue, 0);

  mb_pixbuf_img_composite(pb, img_backing, ac_power ? img_icon_pwr : img_icon, 0, 0);

  paint ();
}

void 
usage (char *progname)
{
  ;
}

static void
mbpixbuf_to_mask (MBPixbuf *pb, MBPixbufImage *img, Drawable     drw,
		  GC gc0, GC gc1, int drw_x, int drw_y)
{
  unsigned char *p;
  int x,y;
  int r, g, b;

  p = img->rgba;

  for(y=0; y<img->height; y++)
    {
      for(x=0; x<img->width; x++)
	{
	  r = ( *p++ );
	  g = ( *p++ );
	  b = ( *p++ );
	  XDrawPoint (dpy, drw, (*p++) ? gc1 : gc0, x + drw_x, y + drw_y);
	}
    }
}

void
draw_text_in (Drawable dr, char *s, int x, int y)
{
  XftDraw *xftdraw = XftDrawCreate (dpy, dr,
				    DefaultVisual(dpy, screen),
				    DefaultColormap(dpy, screen));

  XftDrawStringUtf8 (xftdraw, &fg_xftcol, msg_font, x, y, (FcChar8 *)s, strlen (s));

  XftDrawDestroy (xftdraw);
}

void
text_size (char *s, int *x, int *y)
{
  XGlyphInfo g;

  XftTextExtentsUtf8 (dpy, msg_font, (FcChar8 *)s, strlen (s), &g);

  if (x)
    *x = g.width;
  if (y)
    *y = g.height;
}

#define MAX_OPACITY  192
#define XRANGE 5
#define YRANGE 4

static int
opacity (int val, int range, int ramp)
{
  int step = MAX_OPACITY / ramp;

  if (val < ramp)
    {
      return val * step;
    }
  else if (val > (range - ramp))
    {
      return (range - val - 1) * step;
    }

  return MAX_OPACITY;
}

Pixmap
build_pixmap (MBPixbufImage *background)
{
  Pixmap pix;
  MBPixbufImage *img_whiten;
  int i, j;
  char *p;

  img_whiten = mb_pixbuf_img_new (pb, popup_w, text_offset);
  p = img_whiten->rgba;
  
  for (j = 0; j < text_offset; j++)
    {
      for (i = 0; i < popup_w; i++)
	{
	  int v = opacity (i, popup_w, XRANGE) * opacity (j, text_offset, YRANGE) / 256;
	  
	  *p++ = 255;
	  *p++ = 255;
	  *p++ = 255;
	  *p++ = v;
	}
    }

  mb_pixbuf_img_composite (pb, background, img_whiten, 0, 0);

  mb_pixbuf_img_free (pb, img_whiten);

  mb_pixbuf_img_composite (pb, background, img_icon_popup, 
			   popup_w / 2 - mb_pixbuf_img_get_width (img_icon_popup) / 2,
			   text_offset);

  pix = XCreatePixmap (dpy, win_popup, popup_w, popup_h, pb->depth);

  mb_pixbuf_img_render_to_drawable (pb, background, pix, 0, 0);

  return pix;
}

int
GetWinPosition (Display *dpy, Window win, int *x, int *y, int *w, int *h)
{
  Window root, parent, *children;
  unsigned int nchildren;
  XWindowAttributes attr;
  unsigned int udumm;
  
  XGetWindowAttributes (dpy, win, &attr);

  *w = attr.width;
  *h = attr.height;

  *x = 0 ;
  *y = 0 ;
  
  while (XQueryTree (dpy, win, &root, &parent, &children, &nchildren))
    {
      int w_x, w_y ;
      unsigned int border_w ;
      if (children) 
	XFree (children);

      if (!XGetGeometry (dpy, win, &root, &w_x, &w_y, &udumm, &udumm, &border_w, &udumm))
	break ;

      (*x)+=w_x+(int)border_w ;
      (*y)+=w_y+(int)border_w ;
      
      if (parent == root)
	return 1;

      win = parent;
    }

  *x = 0;
  *y = 0;
  return 0;
}

static int
no_errors (Display *dpy, XErrorEvent *ev)
{
  return 0;
}

int 
main (int argc, char **argv)
{
  int i;
  XEvent xevent;
  char *dpy_name = NULL;
  int xfd;
  fd_set fd;

  setlocale (LC_ALL, "");

  for (i = 1; i < argc; i++) {
    if (!strcmp ("-display", argv[i]) || !strcmp ("-d", argv[i])) {
      if (++i>=argc) usage (argv[0]);
      dpy_name = argv[i];
      continue;
    }
    usage(argv[0]);
  }

  if ((dpy = XOpenDisplay(dpy_name)) == NULL)
    {
      fprintf(stderr, _("Cannot connect to X server on display %s\n"),
	      XDisplayName(dpy_name));
      exit(1);
    }

  screen   = DefaultScreen(dpy);
  win_root = DefaultRootWindow(dpy);
  pb       = mb_pixbuf_new(dpy, screen);

  if (!(img_icon = mb_pixbuf_img_new_from_file(pb, PIXMAPSDIR "/minibat.png")))
    {
      fprintf(stderr, _("%s: failed to load image %s\n"), argv[0], PIXMAPSDIR "/minibat.png");
      exit(1);
    }

  if (!(img_icon_pwr = mb_pixbuf_img_new_from_file(pb, PIXMAPSDIR "/minibat-ac.png")))
    {
      fprintf(stderr, _("%s: failed to load image %s\n"), argv[0], PIXMAPSDIR "/minibat-ac.png");
      exit(1);
    }

  if (!(img_icon_popup = mb_pixbuf_img_new_from_file(pb, PIXMAPSDIR "/minibat-battery.png")))
    {
      fprintf(stderr, _("%s: failed to load image %s\n"), argv[0], PIXMAPSDIR "/minibat-battery.png");
      exit(1);
    }

  if ((msg_font = XftFontOpenName(dpy, screen,"sans-7")) 
      == NULL)
    { printf("Cant open XFT font\n"); exit(0); }

  {
    XRenderColor colortmp;
    colortmp.red   = 0;
    colortmp.green = 0;
    colortmp.blue  = 0;
    colortmp.alpha = 0xffff;
    XftColorAllocValue(dpy,
		       DefaultVisual(dpy, screen),
		       DefaultColormap(dpy, screen),
		       &colortmp,
		       &fg_xftcol);
  }

  text_offset = 2 * (msg_font->ascent + msg_font->descent);
  popup_h = text_offset + mb_pixbuf_img_get_height (img_icon_popup);

  img_backing = mb_pixbuf_img_new(pb, 
				  mb_pixbuf_img_get_width(img_icon), 
				  mb_pixbuf_img_get_height(img_icon));


  win_panel = XCreateSimpleWindow(dpy, win_root, 0, 0,
				  mb_pixbuf_img_get_width(img_icon), 
				  mb_pixbuf_img_get_height(img_icon), 
				  0,
				  BlackPixel(dpy, screen),
				  WhitePixel(dpy, screen));

  win_popup = XCreateSimpleWindow(dpy, win_root, 0, 0,
				  popup_w, popup_h,
				  0,
				  BlackPixel(dpy, screen),
				  WhitePixel(dpy, screen));

  pxm_backing = XCreatePixmap(dpy, win_panel, 
			      mb_pixbuf_img_get_width(img_icon), 
			      mb_pixbuf_img_get_height(img_icon),
			      pb->depth);

  {
    XSetWindowAttributes attr;
    attr.override_redirect = True;
    XChangeWindowAttributes (dpy, win_popup, CWOverrideRedirect, &attr);
  }

  gc = XCreateGC(dpy, win_root, 0, NULL);
  popup_gc = XCreateGC (dpy, win_popup, 0, NULL);

  XStoreName(dpy, win_panel, _("Battery Monitor"));
  
  mb_tray_init_session_info(dpy, win_panel, argv, argc);
  mb_tray_init(dpy, win_panel);
  mb_tray_window_icon_set(dpy, win_panel, img_icon);

  set_backing(NULL);
  mb_tray_bg_change_cb_set(set_backing, NULL);

  XSelectInput(dpy, win_panel, StructureNotifyMask|ExposureMask
	       |ButtonPressMask|ButtonReleaseMask|VisibilityChangeMask); 
  XSelectInput(dpy, win_popup, ExposureMask | ButtonPressMask); 

  interactive_help_atom = XInternAtom (dpy, "_GPE_INTERACTIVE_HELP", False);
  XChangeProperty (dpy, win_panel, interactive_help_atom, interactive_help_atom,
		   32, PropModeReplace, NULL, 0);

  xfd = ConnectionNumber (dpy);

  while (1)
    {
      int visible = 1;
      struct timeval tvt;

      tvt.tv_usec = 0;
      tvt.tv_sec = 2; 		/* check batt stats every 2 seconds  */
      
      FD_ZERO (&fd);
      FD_SET (xfd, &fd);

      if (select (xfd+1, &fd, NULL, NULL, &tvt) == 0)
	{
	  paint();
	}

      while (XPending(dpy) || !visible)
	{
	  XNextEvent(dpy, &xevent);
	  switch (xevent.type) 
	    {
	    case ClientMessage:
	      if (xevent.xclient.message_type == interactive_help_atom)
		{
		  Window w = xevent.xclient.data.l[0];
		  const char *str = _("This is the battery monitor.\nTap here to find out how much power is left.\n");
		  void *old_handler = XSetErrorHandler (no_errors);
		  XChangeProperty (dpy, w, interactive_help_atom, XA_STRING, 8,
				   PropModeReplace, str, strlen (str));
		  XFlush (dpy);
		  XSetErrorHandler (old_handler);
		}
	      break;

	    case Expose:
	      XClearWindow (dpy, xevent.xexpose.window);
	      break;

	    case VisibilityNotify:
	      visible = (xevent.xvisibility.state != VisibilityFullyObscured);
	      break;

	    case ButtonPress:
	      popup_visible = !popup_visible;
	      if (popup_visible) 
		{
		  char buf[64];
		  XImage *image;
		  int x, y, w, h;
		  int x_pos, y_pos;
		  MBPixbufImage *img_back;

		  GetWinPosition (dpy, win_panel, &x, &y, &w, &h);
		  x_pos = x + (w / 2) - (popup_w / 2);
		  y_pos = y + h - popup_h;

		  img_back = mb_pixbuf_img_new_from_drawable (pb, win_root,
							      None, x_pos, y_pos, 
							      popup_w, popup_h);

		  popup_pixmap = build_pixmap (img_back);

		  mb_pixbuf_img_free (pb, img_back);

		  if (apm_vals[PERCENTAGE] >= 0)
		    {
		      snprintf (buf, sizeof (buf) - 1, _("%d%%"), apm_vals[PERCENTAGE]);
		      buf[sizeof(buf - 1)] = 0;
		      text_size (buf, &w, &h);
		      draw_text_in (popup_pixmap, buf, (popup_w / 2) - (w / 2), text_offset - msg_font->descent);
		      h += 4;
		    }
		  else
		    h = 0;

		  buf[0] = 0;
		  if (apm_vals[AC_POWER] == AC_LINE_STATUS_ON)
		    {
		      if (apm_vals[BATT_STATUS] & BATTERY_STATUS_CHARGING)
			strncpy (buf, _("Charging"), sizeof (buf) - 1);
		      else
			strncpy (buf, _("AC on"), sizeof (buf) - 1);
		    }
		  else if (apm_vals[TIME_LEFT] >= 0)
		    snprintf (buf, sizeof (buf) - 1, _("%d min"), apm_vals[TIME_LEFT]);

		  if (buf[0])
		    {
		      buf[sizeof(buf) - 1] = 0;
		      text_size (buf, &w, NULL);
		      draw_text_in (popup_pixmap, buf, (popup_w / 2) - (w / 2), 
				    text_offset - h - msg_font->descent);
		      
		    }

		  XSetWindowBackgroundPixmap (dpy, win_popup, popup_pixmap);
		  XMoveWindow (dpy, win_popup, x_pos, y_pos);
		  XMapRaised (dpy, win_popup);
		  XGrabPointer (dpy, win_popup, True, ButtonPressMask, GrabModeAsync, GrabModeAsync,
				None, None, xevent.xbutton.time);
		}
	      else
		{
		  XUngrabPointer (dpy, xevent.xbutton.time);
		  XUnmapWindow (dpy, win_popup);
		  XFreePixmap (dpy, popup_pixmap);
		  popup_pixmap = 0;
		}
	      break;
	    }

	  mb_tray_handle_event(dpy, win_panel, &xevent);
	}
    }
}
