/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GPE_PIXMAP_H__
#define __GPE_PIXMAP_H__


#include <gdk/gdk.h>
#include <gtk/gtkmisc.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_GPE_PIXMAP		 (gtk_gpe_pixmap_get_type ())
#define GTK_GPE_PIXMAP(obj)		 (GTK_CHECK_CAST ((obj), GTK_TYPE_GPE_PIXMAP, GtkGpePixmap))
#define GTK_GPE_PIXMAP_CLASS(klass)	 (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_GPE_PIXMAP, GtkGpePixmapClass))
#define GTK_IS_GPE_PIXMAP(obj)		 (GTK_CHECK_TYPE ((obj), GTK_TYPE_GPE_PIXMAP))
#define GTK_IS_GPE_PIXMAP_CLASS(klass)	 (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_GPE_PIXMAP))


typedef struct _GtkGpePixmap		GtkGpePixmap;
typedef struct _GtkGpePixmapClass	GtkGpePixmapClass;

struct _GtkGpePixmap
{
  GtkMisc misc;
  
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  GdkPixmap *pixmap_insensitive;
  GdkPixmap *pixmap_prelight;
  GdkPixmap *pixmap_active;
  guint build_insensitive : 1;
};

struct _GtkGpePixmapClass
{
  GtkMiscClass parent_class;
};


GtkType	   gtk_gpe_pixmap_get_type	 (void);
GtkWidget* gtk_gpe_pixmap_new	 (GdkPixmap  *pixmap,
				  GdkBitmap  *mask);
void	   gtk_gpe_pixmap_set	 (GtkGpePixmap  *pixmap,
				  GdkPixmap  *val,
				  GdkBitmap  *mask);
void	   gtk_gpe_pixmap_get	 (GtkGpePixmap  *pixmap,
				  GdkPixmap **val,
				  GdkBitmap **mask);

void       gtk_gpe_pixmap_set_build_insensitive (GtkGpePixmap *pixmap,
						 gboolean build);

void	   gtk_gpe_pixmap_set_prelight (GtkGpePixmap *pixmap, GdkPixmap *val);

void	   gtk_gpe_pixmap_set_active (GtkGpePixmap *pixmap, GdkPixmap *val);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_PIXMAP_H__ */
