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

#if GTK_MAJOR_VERSION < 2
#error Compiling with GTK 1 is not supported
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * gpe_icon:
 * 
 * Struct defining a icon resource in GPE.
 * It consists of three parts:
 *
 * shortname: Short symbolic name to identify the icon.
 *
 * filename: Image file with full path or file in theme location (PREFIX /share/pixmaps/gpe/&lt;theme&gt;).
 *	
 * pixbuf: GdkPixbuf with icon data.
 */
struct gpe_icon
{
  const gchar *shortname;
  const gchar *filename;
  GdkPixbuf *pixbuf;
};

extern gboolean gpe_load_icons (struct gpe_icon *);
extern GdkPixbuf *gpe_find_icon (const gchar *name);
extern GdkPixbuf *gpe_find_icon_scaled (const gchar *name, GtkIconSize size);
extern GdkPixbuf *gpe_find_icon_scaled_free (const gchar *name, gint width, gint height);
extern GdkPixbuf *gpe_try_find_icon (const gchar *name, gchar **error);
extern gboolean gpe_find_icon_pixmap (const gchar *name,
				      GdkPixmap **pixmap,
				      GdkBitmap **bitmap);
extern void gpe_set_theme (const gchar *theme_name);


extern void gpe_set_window_icon (GtkWidget *window, gchar *icon);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
