/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/fb.h>

#define FB "/dev/fb/0"
#define IMAGE "/usr/share/gpe/splash.png"

#define SIZE 240 * 320 * 2

static char *cursoff = "\033[?25l\000";

int 
main(void)
{
  GdkPixbuf *buf =  gdk_pixbuf_new_from_file (IMAGE);
  guchar *pix;
  int fd;
  gushort *fb;
  int x, y;
  size_t stride;
  struct fb_cursorstate curs;
  int tty;

  if (buf == NULL)
    exit (1);

  pix = gdk_pixbuf_get_pixels (buf);
  fd = open (FB, O_RDWR);
  if (fd < 0)
    {
      perror (FB);
      exit (1);
    }
  fb = mmap (0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (fb == NULL)
    {
      perror ("mmap");
      exit (1);
    }

  tty = open ("/dev/tty0", O_RDWR);
  if (tty < 0)
    perror (open);
  else
    {
      write (tty, cursoff, strlen (cursoff));
      close (tty);
    }

  stride = gdk_pixbuf_get_rowstride (buf);

  for (y = 0; y < 320; y++)
    {
      guchar *ppix = pix;
      for (x = 0; x < 240; x++)
	{
	  guchar red, green, blue;
	  gushort w;
	  red = *(ppix++);
	  green = *(ppix++);
	  blue = *(ppix++);

	  red >>= 3;
	  green >>= 2;
	  blue >>= 3;
	  w = blue | (green << 5) | (red << 11);

	  fb[(319 - y) + (320 * x)] = w;
	}
      pix += stride;
    }
}
