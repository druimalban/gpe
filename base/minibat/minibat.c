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
#include <stdint.h>

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

#define popup_w 64

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

  if (apm_read(&info))
    {
      memset (&values, 0, sizeof (values));
      return 1;
    }

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

void
draw_text_in (Drawable dr, char *s, int x, int y)
{
  XftDraw *xftdraw = XftDrawCreate (dpy, dr,
				    DefaultVisual (dpy, screen),
				    DefaultColormap (dpy, screen));

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

void
pixelColorWeightNolock (MBPixbufImage *dst, int x, int y, uint32_t color, int a)
{
  unsigned char *p = dst->rgba + (4 * (x + (dst->width * y)));
  int r = (color >> 24) & 0xff;
  int g = (color >> 16) & 0xff;
  int b = (color >> 8) & 0xff;
  int ap;

  a = (160 * a) >> 8;

  ap = 255 - a;
  
  *p = ((*p * ap) + (r * a)) >> 8;
  p++;
  *p = ((*p * ap) + (g * a)) >> 8;
  p++;
  *p = ((*p * ap) + (b * a)) >> 8;
}

void
draw_ellipse (MBPixbufImage *dst, int xc, int yc, int rx, int ry, uint32_t color)
{
    int i;
    int a2, b2, ds, dt, dxt, t, s, d;
    short x, y, xs, ys, dyt, xx, yy, xc2, yc2;
    float cp;
    unsigned char weight, iweight;

    /* Sanity check radius */
    if (rx < 1)
	rx = 1;
    if (ry < 1)
	ry = 1;

    /* Variable setup */
    a2 = rx * rx;
    b2 = ry * ry;

    ds = 2 * a2;
    dt = 2 * b2;

    xc2 = 2 * xc;
    yc2 = 2 * yc;

    dxt = (int) (a2 / sqrt(a2 + b2));

    t = 0;
    s = -2 * a2 * ry;
    d = 0;

    x = xc;
    y = yc - ry;

    /* "End points" */
    pixelColorWeightNolock(dst, x, y, color, 255);
    pixelColorWeightNolock(dst, xc2 - x, y, color, 255);
    pixelColorWeightNolock(dst, x, yc2 - y, color, 255);
    pixelColorWeightNolock(dst, xc2 - x, yc2 - y, color, 255);
    {
      int yi;
      for (yi = y; yi <= yc; yi++)
	{
	  pixelColorWeightNolock(dst, xc, yi, color, 255);
	  pixelColorWeightNolock(dst, xc, yc2 - yi, color, 255);
	}
    }

    for (i = 1; i <= dxt; i++) {
	x--;
	d += t - b2;

	if (d >= 0)
	    ys = y - 1;
	else if ((d - s - a2) > 0) {
	    if ((2 * d - s - a2) >= 0)
		ys = y + 1;
	    else {
		ys = y;
		y++;
		d -= s + a2;
		s += ds;
	    }
	} else {
	    y++;
	    ys = y + 1;
	    d -= s + a2;
	    s += ds;
	}

	t -= dt;

	/* Calculate alpha */
	if (s != 0.0) {
	    cp = (float) abs(d) / (float) abs(s);
	    if (cp > 1.0) {
		cp = 1.0;
	    }
	} else {
	    cp = 1.0;
	}

	/* Calculate weights */
	weight = (unsigned char) (cp * 255);
	iweight = 255 - weight;

	/* Upper half */
	xx = xc2 - x;
	pixelColorWeightNolock(dst, x, y, color, iweight);
	pixelColorWeightNolock(dst, xx, y, color, iweight);

	pixelColorWeightNolock(dst, x, ys, color, weight);
	pixelColorWeightNolock(dst, xx, ys, color, weight);

	/* Lower half */
	yy = yc2 - y;
	pixelColorWeightNolock(dst, x, yy, color, iweight);
	pixelColorWeightNolock(dst, xx, yy, color, iweight);

	yy = yc2 - ys;
	pixelColorWeightNolock(dst, x, yy, color, weight);
	pixelColorWeightNolock(dst, xx, yy, color, weight);

	{
	  int yi;
	  for (yi = y; yi < yc; yi++)
	    {
	      pixelColorWeightNolock(dst, x, yi, color, 255);
	      pixelColorWeightNolock(dst, x, yc2 - yi, color, 255);
	      pixelColorWeightNolock(dst, xx, yi, color, 255);
	      pixelColorWeightNolock(dst, xx, yc2 - yi, color, 255);
	    }
	}
    }

    dyt = abs(y - yc);

    for (i = 1; i <= dyt; i++) {
	y++;
	d -= s + a2;

	if (d <= 0)
	    xs = x + 1;
	else if ((d + t - b2) < 0) {
	    if ((2 * d + t - b2) <= 0)
		xs = x - 1;
	    else {
		xs = x;
		x--;
		d += t - b2;
		t -= dt;
	    }
	} else {
	    x--;
	    xs = x - 1;
	    d += t - b2;
	    t -= dt;
	}

	s += ds;

	/* Calculate alpha */
	if (t != 0.0) {
	    cp = (float) abs(d) / (float) abs(t);
	    if (cp > 1.0) {
		cp = 1.0;
	    }
	} else {
	    cp = 1.0;
	}

	/* Calculate weight */
	weight = (unsigned char) (cp * 255);
	iweight = 255 - weight;

	/* Left half */
	xx = xc2 - x;
	yy = yc2 - y;
	pixelColorWeightNolock(dst, x, y, color, iweight);
	pixelColorWeightNolock(dst, xx, y, color, iweight);

	pixelColorWeightNolock(dst, x, yy, color, iweight);
	pixelColorWeightNolock(dst, xx, yy, color, iweight);

	/* Right half */
	xx = 2 * xc - xs;
	pixelColorWeightNolock(dst, xs, y, color, weight);
	pixelColorWeightNolock(dst, xx, y, color, weight);

	pixelColorWeightNolock(dst, xs, yy, color, weight);
	pixelColorWeightNolock(dst, xx, yy, color, weight);

	{
	  int xi;
	  for (xi = x; xi < xc; xi++)
	    {
	      pixelColorWeightNolock(dst, xi, y, color, 255);
	      pixelColorWeightNolock(dst, xc2 - xi, y, color, 255);
	      pixelColorWeightNolock(dst, xi, yy, color, 255);
	      pixelColorWeightNolock(dst, xc2 - xi, yy, color, 255);
	    }
	}
    }
}

Pixmap
build_pixmap (int x_pos, int y_pos, int text_w, int height)
{
  Pixmap pix;
  MBPixbufImage *background;
  int i, j;
  char *p;

  background = mb_pixbuf_img_new_from_drawable (pb, win_root, None, x_pos, y_pos, 
					      popup_w, popup_h);

  draw_ellipse (background, popup_w / 2, text_offset / 2, (text_w * 3) / 4, height, 0xffffffff);
  
  mb_pixbuf_img_composite (pb, background, img_icon_popup, 
			   popup_w / 2 - mb_pixbuf_img_get_width (img_icon_popup) / 2,
			   text_offset);

  pix = XCreatePixmap (dpy, win_popup, popup_w, popup_h, pb->depth);

  mb_pixbuf_img_render_to_drawable (pb, background, pix, 0, 0);

  mb_pixbuf_img_free (pb, background);

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
  popup_h = (text_offset * 5) / 3 + mb_pixbuf_img_get_height (img_icon_popup);

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
      int visible = 0;
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
		  char line1_buf[64], line2_buf[64];
		  int x, y, w, h;
		  int x_pos, y_pos;
		  int text_w;
		  int text1_w, text1_h;
		  int text2_w, text2_h;
		  int height;
		  
		  GetWinPosition (dpy, win_panel, &x, &y, &w, &h);
		  x_pos = x + (w / 2) - (popup_w / 2);
		  y_pos = y + h - popup_h + 4;

		  if (x_pos < 0)
		    x_pos = 0;
		  if (y_pos < 0)
		    y_pos = 0;

		  line1_buf[0] = 0;
		  line2_buf[0] = 0;

		  if (apm_vals[PERCENTAGE] >= 0)
		    {
		      snprintf (line1_buf, sizeof (line1_buf) - 1, _("%d%%"), apm_vals[PERCENTAGE]);
		      line1_buf[sizeof (line1_buf - 1)] = 0;
		    }

		  if (apm_vals[AC_POWER] == AC_LINE_STATUS_ON)
		    {
		      if (apm_vals[BATT_STATUS] & BATTERY_STATUS_CHARGING)
			strncpy (line2_buf, _("Charging"), sizeof (line2_buf) - 1);
		      else
			strncpy (line2_buf, _("AC on"), sizeof (line2_buf) - 1);
		    }
		  else if (apm_vals[TIME_LEFT] >= 0)
		    snprintf (line2_buf, sizeof (line2_buf) - 1, _("%d min"), apm_vals[TIME_LEFT]);

		  text_size (line1_buf, &text1_w, &text1_h);
		  text_size (line2_buf, &text2_w, &text2_h);
		  text_w = (text1_w > text2_w) ? text1_w : text2_w;

		  height = (text1_h && text2_h) ? text_offset / 2 : text_offset / 3;

		  popup_pixmap = build_pixmap (x_pos, y_pos, text_w, height);

		  if (text1_h && text2_h)
		    {
		      draw_text_in (popup_pixmap, line1_buf, 
				    (popup_w / 2) - (text1_w / 2), (text_offset * 1) / 3 + (text1_h / 2));
		      draw_text_in (popup_pixmap, line2_buf, 
				    (popup_w / 2) - (text2_w / 2), (text_offset * 2) / 3 + (text2_h / 2));
		    }
		  else
		    {
		      char *buf = line1_buf[0] ? line1_buf : line2_buf;
		      int len = line1_buf[0] ? text1_w : text2_w;
		      int height = line1_buf[0] ? text1_h : text2_h;

		      draw_text_in (popup_pixmap, buf, 
				    (popup_w / 2) - (len / 2), (text_offset / 2) + (height / 2));
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
