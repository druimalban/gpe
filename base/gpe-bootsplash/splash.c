/*
 * Copyright (C) 2002, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define USE_SVG

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#ifdef USE_SVG
#include <librsvg/rsvg.h>
#endif
#include <errno.h>

#define FB "/dev/fb/0"
#define FB0 "/dev/fb0"

#define IMAGE "/usr/share/gpe/splash.png"
#define SVG_NAME_LANDSCAPE "/usr/share/gpe/splash-l.svg"
#define SVG_NAME_PORTRAIT "/usr/share/gpe/splash-p.svg"

#define SIZE 240 * 320 * 2

static char *cursoff = "\033[?25l\000";

struct fb_fix_screeninfo fix;
struct fb_var_screeninfo var;
int xsize, ysize, stride;

gboolean flip = FALSE;
gboolean rot;
gboolean landscape_fb, landscape_image;
gboolean depth16;
gboolean mono;

int 
main(int argc, char *argv[])
{
  GdkPixbuf *buf;
  guchar *pix;
  int fd;
  guchar *fb;
  guchar *dest, *dest_line;
  int x, y;
  int fb_x, fb_y;
  int xoff, yoff;
  int ostride;
  int tty;
  int mat[2][3];
  int i;
  int size;
#ifdef USE_SVG
  const char *svg_name;
#endif
  gboolean has_alpha, use_landscape;
  int img_xres, img_yres;

  g_type_init ();

  fd = open (FB, O_RDWR);
  if (fd < 0 && errno == ENOENT)
    fd = open (FB0, O_RDWR);
  if (fd < 0)
    {
      perror (FB);
      exit (1);
    }
  
  if (ioctl (fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
      perror ("FBIOGET_FSCREENINFO");
      exit (1);
    }
  
  if (ioctl (fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
      perror ("FBIOGET_VSCREENINFO");
      exit (1);
    }
  /* XXX handle only a few fb formats */
  if ((fix.visual == FB_VISUAL_TRUECOLOR || fix.visual == FB_VISUAL_DIRECTCOLOR) &&
      var.bits_per_pixel == 16 &&
      var.red.offset == 11 && 
      var.green.offset == 5 &&
      var.blue.offset == 0)
    {
      depth16 = TRUE;
      mono = FALSE;
    }
  else if (fix.visual == FB_VISUAL_MONO10 && 
	   var.bits_per_pixel == 1)
    {
      depth16 = FALSE;
      mono = TRUE;
    }
  else
    {
      fprintf (stderr, "%s: frame buffer neither 565 nor monochrome\n", argv[0]);
      fprintf (stderr, "%d, %d, %d, %d, %d\n", fix.visual,
	       var.bits_per_pixel, var.red.offset, var.green.offset, var.blue.offset);
      exit (1);
    }

  landscape_fb = var.xres >= var.yres;

  size = var.xres * var.yres * var.bits_per_pixel / 8;
  fb = mmap (0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (fb == NULL)
    {
      perror ("mmap");
      exit (1);
    }

  tty = open ("/dev/tty0", O_RDWR);
  if (tty < 0)
    perror ("open");
  else
    {
      write (tty, cursoff, strlen (cursoff));
      close (tty);
    }

  img_xres = var.xres;
  img_yres = var.yres;

  use_landscape = landscape_fb;

  for (i = 1; i < argc; i++)
    {
      /* can't autodetect this */
      if (!strcmp(argv[i], "--flip"))
	flip = TRUE;
      else if (!strcmp (argv[i], "--rot"))
	rot = !rot;
      else if (!strcmp (argv[i], "--mono"))
	mono = TRUE;
      else if (!strcmp (argv[i], "--depth1"))
	depth16 = FALSE;
      else if (!strcmp (argv[i], "--depth16"))
	depth16 = TRUE;
      else if (!strcmp (argv[i], "--force-portrait"))
	use_landscape = FALSE;
      else if (!strcmp (argv[i], "--force-landscape"))
	use_landscape = TRUE;
    }

#ifdef USE_SVG
  if (use_landscape)
    {
      if (img_xres < img_yres)
	{
	  img_xres = var.yres;
	  img_yres = var.xres;
	}
      svg_name = SVG_NAME_LANDSCAPE;
    }
  else
    {
      if (img_xres > img_yres)
	{
	  img_xres = var.yres;
	  img_yres = var.xres;
	}
      svg_name = SVG_NAME_PORTRAIT;
    }

  buf = rsvg_pixbuf_from_file_at_size (svg_name, img_xres, img_yres, NULL);
#else
  buf = gdk_pixbuf_new_from_file (IMAGE, NULL);
#endif

  pix = gdk_pixbuf_get_pixels (buf);
  stride = gdk_pixbuf_get_rowstride (buf);
  xsize = gdk_pixbuf_get_width (buf);
  ysize = gdk_pixbuf_get_height(buf);
  
  landscape_image = xsize >= ysize;
  rot = landscape_fb != landscape_image;

  if (buf == NULL)
    exit (1);

  if (rot)
  {
    xoff = ((int) var.yres - xsize) / 2;
    yoff = ((int) var.xres - ysize) / 2;
    if (flip)
    {
      mat[0][0] =  0; mat[0][1] =  1; mat[0][2] = -xoff;
      mat[1][0] = -1; mat[1][1] =  0; mat[1][2] = ysize - 1 + yoff;
    }
    else
    {
      mat[0][0] = 0; mat[0][1] = -1; mat[0][2] = xsize - 1 + xoff;
      mat[1][0] = 1; mat[1][1] =  0; mat[1][2] = -yoff;
    }
  }
  else
  {
    xoff = ((int) var.xres - xsize) / 2;
    yoff = ((int) var.yres - ysize) / 2;
    if (flip)
    {
      mat[0][0] = -1; mat[0][1] =  0; mat[0][2] = xsize - 1 + xoff;
      mat[1][0] =  0; mat[1][1] = -1; mat[1][2] = ysize - 1 + yoff;
    }
    else
    {
      mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = - xoff;
      mat[1][0] = 0; mat[1][1] = 1; mat[1][2] = - yoff;
    }
  }

  dest_line = fb;
  has_alpha = gdk_pixbuf_get_has_alpha (buf);
  for (fb_y = 0; fb_y < var.yres; fb_y++)
  {
    guchar  monobits = 0;
    dest = fb + (fb_y * fix.line_length);
    for (fb_x = 0; fb_x < var.xres; fb_x++)
    {
      guchar  *ppix;
      guchar  red, green, blue;
      x = mat[0][0] * fb_x + mat[0][1] * fb_y + mat[0][2];
      y = mat[1][0] * fb_x + mat[1][1] * fb_y + mat[1][2];
      if (0 <= x && x < xsize && 0 <= y && y < ysize)
      {
	ppix = pix + y * stride + x * (has_alpha ? 4 : 3);
      }
      else
	ppix = pix; /* fill with pixel from upper left */
      red = ppix[0]; green = ppix[1]; blue = ppix[2];
      if (has_alpha)
	{
	  int alpha = ppix[3];
	  red = (((int)red * alpha) + (255 * (255 - alpha))) / 256;
	  green = (((int)green * alpha) + (255 * (255 - alpha))) / 256;
	  blue = (((int)blue * alpha) + (255 * (255 - alpha))) / 256;
	}
      if (mono)
      {
	int	brightness;
	int d, i;

#define DITHER_DIM  4
	static int orderedDither[4][4] = {
	  {  1,  9,  3, 11 },
	  { 13,  5, 15,  7 },
	  {  4, 12,  2, 10 },
	  { 16,  8, 14,  6 }
	};
#define DITHER_SIZE  ((sizeof orderedDither / sizeof orderedDither[0][0]) + 1)

	/* use a simple ordered dither */
	brightness = (red * 153 + green * 301 + blue * 58) >> 9;
	d = orderedDither[fb_x&(DITHER_DIM-1)][fb_y&(DITHER_DIM-1)];
	i = (brightness * DITHER_SIZE + 127) / 255;
	if (depth16)
	{
	  if (i > d)
	    *((gushort *) dest) = 0xffff;
	  else
	    *((gushort *) dest) = 0x0000;
	  dest += 2;
	}
	else
	{
	  int bit = fb_x & 7;
	  if (i > d)
	    monobits |= 1 << bit;
	  if (bit == 7)
	  {
	    *dest++ = monobits;
	    monobits = 0;
	  }
	}
      }
      else
      {
	*(volatile gushort *) dest = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
	dest += 2;
      }
    }
  }

  exit (0);
}
