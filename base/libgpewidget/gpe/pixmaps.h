/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#ifndef PIXMAPS_H
#define PIXMAPS_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

extern void gpe_set_window_icon (GtkWidget *window, gchar *icon);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
