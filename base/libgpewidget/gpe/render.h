/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef RENDER_H
#define RENDER_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

extern GtkWidget *gpe_render_icon(GtkStyle *style, GdkPixbuf *pixbuf);
extern void gpe_render_pixmap(GdkColor *bgcol, GdkPixbuf *pixbuf, GdkPixmap **pixmap,
		  GdkBitmap **bitmap);
extern void gpe_render_pixmap_alpha(GdkColor *bgcol, GdkPixbuf *pixbuf, GdkPixmap **pixmap,
		  GdkBitmap **bitmap, guint overall_alpha);

#endif
