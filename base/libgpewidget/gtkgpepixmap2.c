/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Insensitive pixmap building code by Eckehard Berns from GNOME Stock
 * Copyright (C) 1997, 1998 Free Software Foundation
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <math.h>
#include <gtk/gtk.h>
#include "gtkgpepixmap.h"


static void gtk_gpe_pixmap_class_init (GtkGpePixmapClass  *klass);
static void gtk_gpe_pixmap_init       (GtkGpePixmap       *pixmap);
static gint gtk_gpe_pixmap_expose     (GtkWidget       *widget,
				   GdkEventExpose  *event);
static void gtk_gpe_pixmap_finalize   (GObject         *object);
static void build_insensitive_pixmap (GtkGpePixmap *gtkpixmap);

static GtkWidgetClass *parent_class;

GtkType
gtk_gpe_pixmap_get_type (void)
{
  static GtkType pixmap_type = 0;

  if (!pixmap_type)
    {
      static const GtkTypeInfo pixmap_info =
      {
	"GtkGpePixmap",
	sizeof (GtkGpePixmap),
	sizeof (GtkGpePixmapClass),
	(GtkClassInitFunc) gtk_gpe_pixmap_class_init,
	(GtkObjectInitFunc) gtk_gpe_pixmap_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      pixmap_type = gtk_type_unique (GTK_TYPE_MISC, &pixmap_info);
    }

  return pixmap_type;
}

static void
gtk_gpe_pixmap_class_init (GtkGpePixmapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  parent_class = gtk_type_class (gtk_misc_get_type ());

  gobject_class->finalize = gtk_gpe_pixmap_finalize;

  widget_class->expose_event = gtk_gpe_pixmap_expose;
}

static void
gtk_gpe_pixmap_init (GtkGpePixmap *pixmap)
{
  GTK_WIDGET_SET_FLAGS (pixmap, GTK_NO_WINDOW);

  pixmap->pixmap = NULL;
  pixmap->mask = NULL;
}

GtkWidget*
gtk_gpe_pixmap_new (GdkPixmap *val,
		GdkBitmap *mask)
{
  GtkGpePixmap *pixmap;
   
  g_return_val_if_fail (val != NULL, NULL);
  
  pixmap = gtk_type_new (gtk_gpe_pixmap_get_type ());
  
  pixmap->build_insensitive = TRUE;
  gtk_gpe_pixmap_set (pixmap, val, mask);
  
  return GTK_WIDGET (pixmap);
}

static void
gtk_gpe_pixmap_finalize (GObject *object)
{
  gtk_gpe_pixmap_set (GTK_GPE_PIXMAP (object), NULL, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gtk_gpe_pixmap_set (GtkGpePixmap *pixmap,
		GdkPixmap *val,
		GdkBitmap *mask)
{
  gint width;
  gint height;
  gint oldwidth;
  gint oldheight;

  g_return_if_fail (GTK_IS_GPE_PIXMAP (pixmap));

  if (pixmap->pixmap != val)
    {
      oldwidth = GTK_WIDGET (pixmap)->requisition.width;
      oldheight = GTK_WIDGET (pixmap)->requisition.height;
      if (pixmap->pixmap)
	gdk_pixmap_unref (pixmap->pixmap);
      if (pixmap->pixmap_insensitive)
	gdk_pixmap_unref (pixmap->pixmap_insensitive);
      pixmap->pixmap = val;
      pixmap->pixmap_insensitive = NULL;
      if (pixmap->pixmap)
	{
	  gdk_pixmap_ref (pixmap->pixmap);
	  gdk_window_get_size (pixmap->pixmap, &width, &height);
	  GTK_WIDGET (pixmap)->requisition.width =
	    width + GTK_MISC (pixmap)->xpad * 2;
	  GTK_WIDGET (pixmap)->requisition.height =
	    height + GTK_MISC (pixmap)->ypad * 2;
	}
      else
	{
	  GTK_WIDGET (pixmap)->requisition.width = 0;
	  GTK_WIDGET (pixmap)->requisition.height = 0;
	}
      if (GTK_WIDGET_VISIBLE (pixmap))
	{
	  if ((GTK_WIDGET (pixmap)->requisition.width != oldwidth) ||
	      (GTK_WIDGET (pixmap)->requisition.height != oldheight))
	    gtk_widget_queue_resize (GTK_WIDGET (pixmap));
	  else
	    gtk_widget_queue_clear (GTK_WIDGET (pixmap));
	}
    }

  if (pixmap->mask != mask)
    {
      if (pixmap->mask)
	gdk_bitmap_unref (pixmap->mask);
      pixmap->mask = mask;
      if (pixmap->mask)
	gdk_bitmap_ref (pixmap->mask);
    }
}

void
gtk_gpe_pixmap_get (GtkGpePixmap  *pixmap,
		GdkPixmap **val,
		GdkBitmap **mask)
{
  g_return_if_fail (GTK_IS_GPE_PIXMAP (pixmap));

  if (val)
    *val = pixmap->pixmap;
  if (mask)
    *mask = pixmap->mask;
}

static gint
gtk_gpe_pixmap_expose (GtkWidget      *widget,
		   GdkEventExpose *event)
{
  GtkGpePixmap *pixmap;
  GtkMisc *misc;
  gint x, y;
  gfloat xalign;

  g_return_val_if_fail (GTK_IS_GPE_PIXMAP (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      pixmap = GTK_GPE_PIXMAP (widget);
      misc = GTK_MISC (widget);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
	xalign = misc->xalign;
      else
	xalign = 1.0 - misc->xalign;
  
      x = floor (widget->allocation.x + misc->xpad
		 + ((widget->allocation.width - widget->requisition.width) * xalign)
		 + 0.5);
      y = floor (widget->allocation.y + misc->ypad 
		 + ((widget->allocation.height - widget->requisition.height) * misc->yalign)
		 + 0.5);
      
      if (pixmap->mask)
	{
	  gdk_gc_set_clip_mask (widget->style->black_gc, pixmap->mask);
	  gdk_gc_set_clip_origin (widget->style->black_gc, x, y);
	}

      if (GTK_WIDGET_STATE (widget) == GTK_STATE_INSENSITIVE
          && pixmap->build_insensitive)
        {
	  if (!pixmap->pixmap_insensitive)
	    build_insensitive_pixmap (pixmap);
          gdk_draw_pixmap (widget->window,
	   	           widget->style->black_gc,
		           pixmap->pixmap_insensitive,
		           0, 0, x, y, -1, -1);
        }
      else if (GTK_WIDGET_STATE (widget) == GTK_STATE_PRELIGHT
	       && pixmap->pixmap_prelight)
	{
          gdk_draw_pixmap (widget->window,
	   	           widget->style->black_gc,
		           pixmap->pixmap_prelight,
		           0, 0, x, y, -1, -1);
	}
      else if (GTK_WIDGET_STATE (widget) == GTK_STATE_ACTIVE
	       && pixmap->pixmap_active)
	{
          gdk_draw_pixmap (widget->window,
	   	           widget->style->black_gc,
		           pixmap->pixmap_active,
		           0, 0, x, y, -1, -1);
	}
      else if (GTK_WIDGET_STATE (widget) == GTK_STATE_SELECTED
	       && pixmap->pixmap_selected)
	{
          gdk_draw_pixmap (widget->window,
	   	           widget->style->black_gc,
		           pixmap->pixmap_selected,
		           0, 0, x, y, -1, -1);
	}
      else
	{
          gdk_draw_pixmap (widget->window,
	   	           widget->style->black_gc,
		           pixmap->pixmap,
		           0, 0, x, y, -1, -1);
	}

      if (pixmap->mask)
	{
	  gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
	  gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
	}
    }
  return FALSE;
}

void
gtk_gpe_pixmap_set_build_insensitive (GtkGpePixmap *pixmap, gboolean build)
{
  g_return_if_fail (GTK_IS_GPE_PIXMAP (pixmap));

  pixmap->build_insensitive = build;

  if (GTK_WIDGET_VISIBLE (pixmap))
    {
      gtk_widget_queue_clear (GTK_WIDGET (pixmap));
    }
}

void
gtk_gpe_pixmap_set_prelight (GtkGpePixmap *pixmap, GdkPixmap *val)
{
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (GTK_IS_GPE_PIXMAP (pixmap));

  if (pixmap->pixmap_prelight)
    gdk_pixmap_unref (pixmap->pixmap_prelight);
  pixmap->pixmap_prelight = val;
  if (val)
    gdk_pixmap_ref (val);
}

void
gtk_gpe_pixmap_set_active (GtkGpePixmap *pixmap, GdkPixmap *val)
{
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (GTK_IS_GPE_PIXMAP (pixmap));

  if (pixmap->pixmap_active)
    gdk_pixmap_unref (pixmap->pixmap_active);
  pixmap->pixmap_active = val;
  if (val)
    gdk_pixmap_ref (val);
}

void
gtk_gpe_pixmap_set_selected (GtkGpePixmap *pixmap, GdkPixmap *val)
{
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (GTK_IS_GPE_PIXMAP (pixmap));

  if (pixmap->pixmap_selected)
    gdk_pixmap_unref (pixmap->pixmap_selected);
  pixmap->pixmap_selected = val;
  if (val)
    gdk_pixmap_ref (val);
}

static void
build_insensitive_pixmap (GtkGpePixmap *gtkpixmap)
{
  GdkPixmap *pixmap = gtkpixmap->pixmap;
  GdkPixmap *insensitive;
  gint w, h;
  GdkPixbuf *pixbuf;
  GdkPixbuf *stated;
  
  gdk_window_get_size (pixmap, &w, &h);

  pixbuf = gdk_pixbuf_get_from_drawable (NULL,
                                         pixmap,
                                         gtk_widget_get_colormap (GTK_WIDGET(gtkpixmap)),
                                         0, 0,
                                         0, 0,
                                         w, h);
  
  stated = gdk_pixbuf_copy (pixbuf);
  
  gdk_pixbuf_saturate_and_pixelate (pixbuf, stated,
                                    0.8, TRUE);

  g_object_unref (G_OBJECT (pixbuf));
  pixbuf = NULL;
  
  insensitive = gdk_pixmap_new (GTK_WIDGET (gtkpixmap)->window, w, h, -1);

  gdk_pixbuf_render_to_drawable (stated,
                                 insensitive,
                                 GTK_WIDGET (gtkpixmap)->style->white_gc,
                                 0, 0,
                                 0, 0,
                                 w, h,
                                 GDK_RGB_DITHER_NORMAL,
                                 0, 0);

  gtkpixmap->pixmap_insensitive = insensitive;

  g_object_unref (G_OBJECT (stated));
}


