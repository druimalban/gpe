/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef PIXMAPS_H
#define PIXMAPS_H

#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

struct gpe_icon
{
  const char *shortname;
  const char *filename;
  GdkPixbuf *pixbuf;
};

extern gboolean gpe_load_icons (struct gpe_icon *);
extern GdkPixbuf *gpe_find_icon (const char *name);
extern GdkPixbuf *gpe_try_find_icon (const char *name, gchar **error);
extern gboolean gpe_find_icon_pixmap (const char *name,
				      GdkPixmap **pixmap,
				      GdkBitmap **bitmap);

#endif
