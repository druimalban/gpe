/* minilite -- frontlight control

   derived from: minivol - A tiny volume control tray app

   Copyright 2002 Matthew Allum

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/extensions/shape.h>

#include "mbtray.h"
#include "mbpixbuf.h"

#define PIXMAPSDIR  PREFIX "/share/pixmaps"

#define _(_x)  gettext (_x)

static Display* dpy;	
static Window win_panel, win_root, win_popup;
static int screen;
static GC gc;
static MBPixbuf *pb;
static MBPixbufImage *img_icon = NULL, *img_backing = NULL; 
static XColor xcol_bg;
static Pixmap pxm_backing = None;

static Atom interactive_help_atom;

static int mixerfd;

static Bool popup_is_mapped = False;

static int popup_y_offset = 0; /* 15 for upside down ; */
static Bool slider_active = False;

static void paint( void *user_data );

static
void popup_shape(int fx, int width, int height, Bool flip)
{
   XPoint tri[3];
   Pixmap mask;
   int screen = DefaultScreen(dpy);
   
   GC ShapeGC;
   int hoff = 10 ;
   
   mask = XCreatePixmap( dpy, win_root, width, height, 1 );
   ShapeGC = XCreateGC( dpy, mask, 0, 0 );
   
   XSetForeground( dpy, ShapeGC, BlackPixel( dpy, screen ));
   XFillRectangle( dpy, mask, ShapeGC, 0, 0, width, height );
   XSetForeground( dpy, ShapeGC, WhitePixel( dpy, screen ));
   XSetBackground( dpy, ShapeGC, BlackPixel( dpy, screen ));

   if (flip)
   {
      XFillRectangle(dpy, mask, ShapeGC, 10, 0, width-20, height-hoff);
      XFillRectangle(dpy, mask, ShapeGC, 0, 10, width, height-hoff-20);

      XFillArc( dpy, mask, ShapeGC, 0, 0, 21, 21, 90*64, 90*64 );
      XFillArc( dpy, mask, ShapeGC, width-21, 0, 21, 21, 0, 90*64 );

      if ( (fx+hoff) > width)
	XFillRectangle( dpy, mask, ShapeGC, width-21, height-21-hoff,
			21, 21);
      else
	XFillArc( dpy, mask, ShapeGC, width-21, height-21-hoff,
		  21, 21, 270*64, 90*64 );

      if ( (fx-hoff) < 0 )
	XFillRectangle( dpy, mask, ShapeGC, 0, height-21-hoff,
			21, 21);
      else
	XFillArc( dpy, mask, ShapeGC, 0, height-21-hoff,
		  21, 21, 180*64, 90*64 );

      tri[0].x = fx;	        tri[0].y = height;
      tri[1].x = fx+hoff;	tri[1].y = height-hoff;
      tri[2].x = fx-hoff;	tri[2].y = height-hoff;;

   } else {
      XFillRectangle(dpy, mask, ShapeGC, 10, hoff, width-20, height-hoff);
      XFillRectangle(dpy, mask, ShapeGC, 0, hoff+10, width, height-hoff-20);
      XFillRectangle(dpy, mask, ShapeGC, 0, hoff, width-20, height-hoff-20);
      
      //XFillArc( dpy, mask, ShapeGC, 0, hoff, 21, 21, 90*64, 90*64 );
      XFillArc( dpy, mask, ShapeGC, width-21, hoff, 21, 21, 0, 90*64 );
      XFillArc( dpy, mask, ShapeGC, width-21, height-21,
		21, 21, 270*64, 90*64 );
      XFillArc( dpy, mask, ShapeGC, 0, height-21, 21, 21, 180*64, 90*64 );
      
      tri[0].x = fx;	        tri[0].y = 0;
      tri[1].x = fx+hoff;       tri[1].y = hoff;
      tri[2].x = fx-hoff;	tri[2].y = hoff;
   }
   
   XFillPolygon( dpy, mask, ShapeGC, tri, 3, Nonconvex, CoordModeOrigin );
   
   XShapeCombineMask( dpy, win_popup, ShapeBounding, 0, 0, mask, ShapeSet);
   
   XFreePixmap( dpy, mask );
   XFreeGC( dpy, ShapeGC );
}


static void
popup_create(void)
{
  int i;
  XSetWindowAttributes attr;
  XColor xcol_bg, exact;
  Pixmap popup_backing;

  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "gold", 
		   &xcol_bg, &exact);

  XSetBackground(dpy, gc, xcol_bg.pixel);
  XSetForeground(dpy, gc, xcol_bg.pixel);

  popup_backing = XCreatePixmap(dpy, win_root, 33, 130, 
				DefaultDepth(dpy, screen));

  XFillRectangle(dpy, popup_backing, gc, 0, 0, 33, 130);

  XSetForeground(dpy, gc, BlackPixel(dpy, screen));
  
  for(i=0;i<=50;i++)
    XDrawLine(dpy, popup_backing, gc, 
	      4+((i/2)), 4+(i*2)+popup_y_offset, 
	      4+25, 4+(i*2)+popup_y_offset );


  attr.override_redirect = True;
  attr.event_mask = ButtonPressMask|ExposureMask;
  /* attr.background_pixel = xcol_bg.pixel; */
  attr.background_pixmap = popup_backing;

  /*   attr.background_pixel = bg_col.pixel;
       attr.border_pixel = fg_col.pixel; */
  
  win_popup = XCreateWindow(dpy, win_root,
			    400, 100, 33, 130, 0,
			    DefaultDepth(dpy, DefaultScreen(dpy)),
			    CopyFromParent,
			    DefaultVisual(dpy, DefaultScreen(dpy)),
			    CWOverrideRedirect|CWEventMask|
			    /*CWBackPixel|*/ CWBackPixmap,
			    &attr);

  XClearWindow(dpy, win_panel);

  XFreePixmap(dpy, popup_backing);

  popup_shape(8, 33, 130, True);
}

void 
popup_draw_slider(int level)
{
  XClearWindow(dpy, win_popup);
  XDrawRectangle(dpy, win_popup, gc, 2, (100-level) + popup_y_offset, 29, 8); 
}

int 
set_level(int level)
{
  char buf[64];

  if (level < 0)
    {
      system ("bl off");
      return -1;
    }

  if (level > 100) level = 100;

  snprintf (buf, sizeof (buf) - 1, "bl %d", level);
  buf[sizeof (buf) - 1] = 0;
  system (buf);
    
  popup_draw_slider(level);

  return level;
}

int
read_old_level (void)
{
  int level = 0;
  FILE *fp = popen ("bl", "r");
  if (fp)
    {
      char buf[64];
      if (fgets (buf, sizeof (buf), fp))
	{
	  char *str;
	  sscanf (buf, "%s %d", &str, &level);
	}
      fclose (fp);
    }
  return level;
}

static void
set_backing( void *user_data )
{
  mb_tray_get_bg_col(dpy, &xcol_bg);
  mb_pixbuf_img_fill(pb, img_backing, 
		     xcol_bg.red, xcol_bg.green, xcol_bg.blue, 0);
  mb_pixbuf_img_composite(pb, img_backing, img_icon , 0, 0);
  paint(NULL);
}

static void 
paint( void *user_data )
{
  if (pxm_backing) XFreePixmap(dpy, pxm_backing);

  pxm_backing = XCreatePixmap(dpy, win_panel, 
			      mb_pixbuf_img_get_width(img_icon), 
			      mb_pixbuf_img_get_height(img_icon),
			      pb->depth);

  mb_pixbuf_img_render_to_drawable(pb, img_backing, pxm_backing, 0, 0);

  XSetWindowBackgroundPixmap(dpy, win_panel, pxm_backing);
  XClearWindow(dpy, win_panel);
}

void 
usage(char *progname)
{
  fprintf (stderr, _("usage: %s [-d <display>]\n"), progname);
  exit (1);
}

static int
no_errors (Display *dpy, XErrorEvent *ev)
{
  return 0;
}

int 
main(int argc, char **argv)
{
  int i;
  XEvent xevent;
  char *dpy_name = NULL;
  int devmask = 0;
  int level, orig_level;

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

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
	      dpy_name);
      exit(1);
    }

  screen   = DefaultScreen(dpy);
  win_root = DefaultRootWindow(dpy);
  pb       = mb_pixbuf_new(dpy, screen);

  if (!(img_icon = mb_pixbuf_img_new_from_file(pb, PIXMAPSDIR "/minilite.png")))
    {
      fprintf(stderr, _("%s: failed to load image %s\n"), argv[0], PIXMAPSDIR "/minilite.png");
      exit(1);
    }

  img_backing = mb_pixbuf_img_new(pb, 
				  mb_pixbuf_img_get_width(img_icon), 
				  mb_pixbuf_img_get_height(img_icon));


  win_panel = XCreateSimpleWindow(dpy, win_root, 0, 0,
				  mb_pixbuf_img_get_width(img_icon), 
				  mb_pixbuf_img_get_height(img_icon), 
				  0,
				  BlackPixel(dpy, screen),
				  WhitePixel(dpy, screen));

  gc = XCreateGC(dpy, win_root, 0, NULL);

  XStoreName(dpy, win_panel, _("Frontlight control"));

  popup_create();
  
  mb_tray_init_session_info(dpy, win_panel, argv, argc);
  mb_tray_init(dpy, win_panel);
  mb_tray_window_icon_set(dpy, win_panel, img_icon);

  set_backing(NULL);
  mb_tray_bg_change_cb_set(set_backing, NULL);

  XSelectInput(dpy, win_panel, VisibilityChangeMask|ExposureMask
	       |ButtonPressMask|ButtonReleaseMask|ButtonMotionMask); 
  XSelectInput(dpy, win_popup, VisibilityChangeMask|ExposureMask
	       |ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionHintMask); 

  interactive_help_atom = XInternAtom (dpy, "_GPE_INTERACTIVE_HELP", False);
  XChangeProperty (dpy, win_panel, interactive_help_atom, interactive_help_atom,
		   32, PropModeReplace, NULL, 0);

  level = read_old_level ();

  while (1)
    {
      XNextEvent(dpy, &xevent);
      switch (xevent.type) 
	{
	case ClientMessage:
	  if (xevent.xclient.message_type == interactive_help_atom)
	    {
	      Window w = xevent.xclient.data.l[0];
	      const char *str = _("This is the frontlight control.\nTap here to alter the brightness.\n");
	      void *old_handler = XSetErrorHandler (no_errors);
	      XChangeProperty (dpy, w, interactive_help_atom, XA_STRING, 8,
			       PropModeReplace, str, strlen (str));
	      XFlush (dpy);
	      XSetErrorHandler (old_handler);
	    }
	  break;
	  
	case Expose:
	  popup_draw_slider(level);
	  break;

	case ButtonPress:
	  if (xevent.xmotion.window == win_popup)
	    {
	      orig_level = level 
		= set_level(100-(xevent.xmotion.y-2-popup_y_offset));
	      slider_active = True;
	    } else {
	      Window win_mouse_root, win_mouse;
	      int popup_win_x, popup_win_y, x, y;
	      unsigned int mouse_mask;

	      if (popup_is_mapped)
		{
		  XUngrabPointer(dpy, CurrentTime);
		  XUnmapWindow(dpy, win_popup);
		  popup_is_mapped = False;
		} else {
		  XGrabPointer(dpy, win_root, True,
			       (ButtonPressMask|ButtonReleaseMask),
			       GrabModeAsync,GrabModeAsync, 
			       None, None, CurrentTime);


		  XQueryPointer(dpy, win_root,
				&win_mouse_root, &win_mouse,
				&x, &y, &popup_win_x, &popup_win_y, 
				&mouse_mask);

		  /*if (y < 118 ) y = 136; */
		  XMoveWindow(dpy,win_popup,x-8, y-136);
		  XMapRaised(dpy, win_popup);
		  popup_is_mapped = True;
		}
	    }
	  break;

	case ButtonRelease:
	  slider_active = False;
	  break;

	case MotionNotify:
	  if (slider_active)
	    {
	      Window win_mouse_root, win_mouse;
	      int popup_win_x, popup_win_y, x, y;
	      unsigned int mouse_mask;

	      level = 100-(xevent.xmotion.y+2-popup_y_offset);
	      if (abs(level-orig_level) > 5)
		orig_level = set_level(level);
	      popup_draw_slider(level);
	      XQueryPointer(dpy, win_root,
			    &win_mouse_root, &win_mouse,
			    &x, &y, &popup_win_x, &popup_win_y, 
			    &mouse_mask);
	    }
	  break;

	}
      mb_tray_handle_event(dpy, win_panel, &xevent);
    } 

}
