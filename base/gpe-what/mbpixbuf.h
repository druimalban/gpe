#ifndef _HAVE_MBPIXBUF_H
#define _HAVE_MBPIXBUF_H

/* libmb
 * Copyright (C) 2002 Matthew Allum
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef USE_PNG
#include <png.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>

#define BYTE_ORD_24_RGB 0
#define BYTE_ORD_24_RBG 1
#define BYTE_ORD_24_BRG 2
#define BYTE_ORD_24_BGR 3
#define BYTE_ORD_24_GRB 4
#define BYTE_ORD_24_GBR 5

typedef struct _mb_pixbuf_col {
  int                 r, g, b;
  unsigned long       pixel;
} MBPixbufColor;

typedef struct _mb_pixbuf {

  Display *dpy;
  int scr;	
  Visual *vis;
  Window root;
  int depth;
  Colormap root_cmap;
  int byte_order;
  int num_of_cols;
  GC gc;

  MBPixbufColor *palette;

} MBPixbuf;


typedef struct _mb_pixbuf_img {

  int width;
  int height;
  unsigned char *rgba;
  Bool has_alpha;

  XImage *ximg;

  Bool _safe;   /* Set if the image has been remapped to safe r,g,b  
		   values ... TODO */
  MBPixbuf *pb_ref;

} MBPixbufImage;

typedef unsigned short ush;

/* macros */

#define alpha_composite(composite, fg, alpha, bg) {               \
    ush temp = ((ush)(fg)*(ush)(alpha) +                          \
                (ush)(bg)*(ush)(255 - (ush)(alpha)) + (ush)128);  \
    (composite) = (ush)((temp + (temp >> 8)) >> 8);               \
}

#define mb_pixbuf_img_set_pixel(i, x, y, r, g, b) { \
  (i)->rgba[(((y)*(i)->width*4)+((x)*4))]   = r;    \
  (i)->rgba[(((y)*(i)->width*4)+((x)*4))+1] = g;    \
  (i)->rgba[(((y)*(i)->width*4)+((x)*4))+2] = b;    \
  (i)->rgba[(((y)*(i)->width*4)+((x)*4))+3] = 0;    \
}

#define mb_pixbuf_img_set_pixel_alpha(i, x, y, a) { \
  (i)->rgba[(((y)*(i)->width*4)+((x)*4))+3] = a;    \
}

#define mb_pixbuf_img_composite_pixel(i, x, y, r, g, b, a) {     \
  int idx = (((y)*(i)->width*4)+((x)*4));                        \
 alpha_composite((i)->rgba[idx],   (r), (a), (i)->rgba[idx]);    \
 alpha_composite((i)->rgba[idx+1], (g), (a), (i)->rgba[idx+1]);  \
 alpha_composite((i)->rgba[idx+2], (b), (a), (i)->rgba[idx+2]);  \
 /* (i)->rgba[idx+3] = 0; */ \
 /* alpha_composite((i)->rgba[idx+3], (a), (a), (i)->rgba[idx+3]); */ \
}

#define mb_pixbuf_img_get_width(i)  (i)->width

#define mb_pixbuf_img_get_height(i) (i)->height

/* ------ */


unsigned long
mb_pixbuf_get_pixel(MBPixbuf *pb, int *r, int *g, int *b);

MBPixbuf *
mb_pixbuf_new(Display *dpy, int scr);

MBPixbufImage *
mb_pixbuf_img_new(MBPixbuf *pb, int w, int h);

MBPixbufImage *
mb_pixbuf_img_new_from_drawable(MBPixbuf *pb, Drawable drw, Drawable msk,
				int sx, int sy, int sw, int sh);

MBPixbufImage *
mb_pixbuf_img_new_from_file(MBPixbuf *pb, const char *filename);

void
mb_pixbuf_img_free(MBPixbuf *pb, MBPixbufImage *img);

MBPixbufImage *
mb_pixbuf_img_clone(MBPixbuf *pb, MBPixbufImage *img);

void
mb_pixbuf_img_fill(MBPixbuf *pb, MBPixbufImage *img,
		   int r, int g, int b, int a);

void
mb_pixbuf_img_composite(MBPixbuf *pb, MBPixbufImage *dest,
			MBPixbufImage *src, int dx, int dy);

void
mb_pixbuf_img_copy_composite(MBPixbuf *pb, MBPixbufImage *dest,
			     MBPixbufImage *src, int sx, int sy, 
			     int sw, int sh, int dx, int dy);

void
mb_pixbuf_img_copy(MBPixbuf *pb, 
		   MBPixbufImage *dest,
		   MBPixbufImage *src, 
		   int sx, int sy, int sw, int sh,
		   int dx, int dy);

MBPixbufImage *
mb_pixbuf_img_scale_down(MBPixbuf *pb, MBPixbufImage *img, 
			 int new_width, int new_height);

MBPixbufImage *
mb_pixbuf_img_scale_up(MBPixbuf *pb, MBPixbufImage *img, 
		       int new_width, int new_height);

MBPixbufImage *
mb_pixbuf_img_scale(MBPixbuf *pb, MBPixbufImage *img, 
		    int new_width, int new_height);

void
mb_pixbuf_img_render_to_drawable(MBPixbuf    *pb,
				 MBPixbufImage *img,
				 Drawable     drw,
				 int drw_x,
				 int drw_y);

#endif
