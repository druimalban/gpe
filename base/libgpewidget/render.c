/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gtkgpepixmap.h"
#include "render.h"

static void
gpe_render_pixmap(GdkColor *bgcol, GdkPixbuf *pixbuf, GdkPixmap **pixmap,
		  GdkBitmap **bitmap)
{
  guint width = gdk_pixbuf_get_width (pixbuf),
    height = gdk_pixbuf_get_height (pixbuf);
  guint depth = gdk_pixbuf_get_bits_per_sample (pixbuf);
  
  if (bgcol && gdk_pixbuf_get_has_alpha (pixbuf))	
    {
      GdkColorspace color = gdk_pixbuf_get_colorspace (pixbuf);
      guint y, stride, sstride;
      GdkPixbuf *composited = gdk_pixbuf_new (color, TRUE, depth, width, height);
      guchar *ptr = gdk_pixbuf_get_pixels (composited);
      guchar *sptr = gdk_pixbuf_get_pixels (pixbuf);
      guchar br, bg, bb;

      stride = gdk_pixbuf_get_rowstride (composited);
      sstride = gdk_pixbuf_get_rowstride (pixbuf);

      br = bgcol->red >> 8;
      bg = bgcol->green >> 8;
      bb = bgcol->blue >> 8;
      
      for (y = 0; y < height; y++) 
	{
	  guchar *p = ptr + (y * stride);
	  guchar *sp = sptr + (y * sstride);
	  guchar *ep = p + (width * 4);
	  while (p < ep)
	    {
	      guint red, green, blue;
	      guint sr, sg, sb, sa;
	      
	      sr = *sp++;
	      sg = *sp++;
	      sb = *sp++;
	      sa = *sp++;
	      
	      if (sa == 255 || sa == 0)
		{
		  /* fully opaque or fully transparent is trivial */
		  red = sr;
		  green = sg;
		  blue = sb;
		}
	      else 
		{
		  guint r1 = (guint)sr * sa;
		  guint b1 = (guint)sb * sa;
		  guint g1 = (guint)sg * sa;
		  guint r2 = (guint)br * (255 - sa);
		  guint g2 = (guint)bg * (255 - sa);
		  guint b2 = (guint)bb * (255 - sa);
		  
		  red = (r1 + r2) / 256;
		  green = (g1 + g2) / 256;
		  blue = (b1 + b2) / 256;
	        }
	      
	      *p++ = red;
	      *p++ = green;
	      *p++ = blue;
	      *p++ = sa ? 255 : 0;
	    }
	}
      
      gdk_pixbuf_render_pixmap_and_mask (composited, 
					 pixmap, bitmap, 1);

      gdk_pixbuf_unref (composited);
    }
  else
    gdk_pixbuf_render_pixmap_and_mask (pixbuf,
				       pixmap, bitmap, 127);
}

GtkWidget *
gpe_render_icon(GtkStyle *style, GdkPixbuf *pixbuf)
{
  GtkWidget *widget;
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;

  if (pixbuf == NULL)
    abort ();

  gpe_render_pixmap (style ? &style->bg[GTK_STATE_NORMAL] : NULL, 
		     pixbuf, &pixmap, &bitmap);
  
  widget = gtk_gpe_pixmap_new (pixmap, bitmap);

  if (style && 
      style->bg[GTK_STATE_NORMAL].pixel != style->bg[GTK_STATE_PRELIGHT].pixel)
    {
      gpe_render_pixmap (&style->bg[GTK_STATE_PRELIGHT], pixbuf,
			 &pixmap, &bitmap);
      gtk_gpe_pixmap_set_prelight (GTK_GPE_PIXMAP (widget), pixmap);
    }
  
  if (style && 
      style->bg[GTK_STATE_NORMAL].pixel != style->bg[GTK_STATE_ACTIVE].pixel)
    {
      gpe_render_pixmap (&style->bg[GTK_STATE_ACTIVE], pixbuf,
			 &pixmap, &bitmap);
      gtk_gpe_pixmap_set_active (GTK_GPE_PIXMAP (widget), pixmap);
    }

  return widget;
}
