#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>

#include "rootpixmap.h"

static int
GetRootDimensions( Display *dpy, int* width, int* height )
{
  Window root ;
  int w_x, w_y ;
  unsigned int border_w, w_depth ;

  if( !XGetGeometry (dpy, RootWindow (dpy, DefaultScreen (dpy)), &root,
		     &w_x, &w_y, width, height, &border_w, &w_depth ))
    {
      *width = 0 ;
      *height = 0 ;
    }

  return (*width>0 && *height>0)?1:0 ;
}

int
GetWinPosition (Display *dpy, Window win, int *x, int *y, int *wp, int *hp)
{
  Window root, parent, *children ;
  unsigned int nchildren ;
  int rootWidth = 0, rootHeight = 0;
  XWindowAttributes attr ;
  int my_x, my_y, dumm ;
  unsigned int w, h, udumm ;
  
  XGetWindowAttributes( dpy, win, &attr );

  if( !x ) x = &my_x ;
  if( !y ) y = &my_y ;
  
  *x = 0 ;
  *y = 0 ;
  
  if( !GetRootDimensions(dpy, &rootWidth, &rootHeight)) 
    return 0;

  XGetGeometry( dpy, win, &root, &dumm, &dumm, &w, &h, &udumm, &udumm );
  *wp = w;
  *hp = h;

  while( XQueryTree( dpy, win, &root, &parent, &children, &nchildren ) )
    {
      int w_x, w_y ;
      unsigned int border_w ;
      if (children) 
	XFree (children);

      if (!XGetGeometry( dpy, win, &root, &w_x, &w_y, &udumm, &udumm, &border_w, &udumm ))
	break ;

      (*x)+=w_x+(int)border_w ;
      (*y)+=w_y+(int)border_w ;
      
      if (parent == root)
	{	/* taking in to consideration virtual desktopping */
	  int bRes = 1 ;
	  if( *x>=rootWidth  || (*x+(int)w) <= 0 ||
	      *y>=rootHeight || (*y+(int)h) <= 0 )
	    bRes = 0 ;
	  /* don't want to return position outside the screen even if we fail */
	  /*	    while( *x < 0 ) *x += rootWidth ;
		    while( *y < 0 ) *y += rootHeight ;
		    while( *x+(int)w > rootWidth ) *x -= rootWidth ;
		    while( *y+(int)h > rootHeight) *y -= rootHeight ;
	  */	    return bRes;
	}
      win = parent ;
    }

  *x = 0 ;
  *y = 0 ;
  return 0 ;
}

static Pixmap
CutPixmap ( Display *dpy, Pixmap src, Pixmap trg,
            int x, int y,
	    unsigned int width, unsigned int height,
	    GC gc)
{
  int screen_w, screen_h ;
  int offset_x = 0, offset_y = 0 ;

  screen_w = DisplayWidth( dpy, DefaultScreen (dpy) );
  screen_h = DisplayHeight( dpy, DefaultScreen (dpy) );

  while( x+(int)width < 0 )  x+= screen_w ;
  while( x >= screen_w )  x-= screen_w ;
  while( y+(int)height < 0 )  y+= screen_h ;
  while( y >= screen_h )  y-= screen_h ;

  if( x < 0 )
    {
      offset_x = (-x) ;
      x = 0 ;
      width -= offset_x ;
    }

  if( y < 0 )
    {
      offset_y = (-y) ;
      y = 0 ;
      height -= offset_y ;
    }

  if( x+width >= screen_w ) width = screen_w - x ;
  if( y+height >= screen_h ) height = screen_h - y ;

  /* create target pixmap of the size of the window */
  if( trg == None )    
    {
      XWindowAttributes gattr;
      XGetWindowAttributes(dpy, RootWindow (dpy, DefaultScreen (dpy)), &gattr);
      trg = XCreatePixmap(dpy, RootWindow (dpy, DefaultScreen (dpy)), width + offset_x, height + offset_y, gattr.depth);
    }

  if (trg != None)
    {
      if( !XCopyArea (dpy, src, trg, gc, x, y, width, height, offset_x, offset_y))
	XFillRectangle( dpy, trg, gc, offset_x, offset_y, width, height );
    }

  return trg;
}

Pixmap
CutWinPixmap (Display *dpy, Window win, Drawable src, GC gc)
{
  unsigned int x = 0, y = 0, width, height;

  if (!GetWinPosition (dpy, win, &x, &y, &width, &height))
    return None;
  
  return CutPixmap( dpy, src, None, x, y, width, height, gc);
}


/* PROTO */
Pixmap
GetRootPixmap(Display *dpy)
{
  Pixmap currentRootPixmap = None;
  Atom id;

  id = XInternAtom (dpy, "_XROOTPMAP_ID", True);

  if( id != None )
    {
      Atom act_type;
      int act_format;
      unsigned long nitems, bytes_after;
      unsigned char *prop = NULL;
      
      if (XGetWindowProperty (dpy, RootWindow (dpy, DefaultScreen (dpy)), id, 0, 1, False, XA_PIXMAP,
			      &act_type, &act_format, &nitems, &bytes_after,
			      &prop) == Success)
	{
	  if (prop)
	    {
	      currentRootPixmap = *((Pixmap *) prop);
	      XFree (prop);
	    }
	}
    }

  return currentRootPixmap ;
}
