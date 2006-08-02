/*
 * Copyright (C) 2006 Florian Boor <florian.boor@kernelconcepts.de>
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

#include <gtk/gtk.h>

#include "gpe/colorrenderer.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(x) gettext(x)
#else
#define _(x) (x)
#endif

enum
{
    PROP_COLORLIST = 1,
    PROP_WIDTH = 2,
    PROP_HEIGHT = 3
};

static gpointer parent_class;

static void cell_renderer_color_class_init (gpointer klass, gpointer data);
static void cell_renderer_color_init (CellRendererColor *renderer);
static void cell_renderer_color_finalize (GObject *object);
static void cell_renderer_color_render (GtkCellRenderer *cell,
				      GdkWindow *window,
				      GtkWidget *widget,
				      GdkRectangle *background_area,
				      GdkRectangle *cell_area,
				      GdkRectangle *expose_area,
				      GtkCellRendererState flags);
static void cell_renderer_color_get_size (GtkCellRenderer *cell,
					GtkWidget *widget,
					GdkRectangle *cell_area,
					gint *x_offset,
					gint *y_offset,
					gint *width,
					gint *height);
static void cell_renderer_color_get_property  (GObject    *object,
                                          guint      param_id,
                                          GValue     *value,
                                          GParamSpec *pspec);

static void cell_renderer_color_set_property  (GObject      *object,
                                          guint        param_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);

static GType
cell_renderer_color_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (CellRendererColorClass),
        NULL,
        NULL,
        cell_renderer_color_class_init,
        NULL,
        NULL,
        sizeof (CellRendererColor),
        0,
        (GInstanceInitFunc) cell_renderer_color_init
      };

      type = g_type_register_static (gtk_cell_renderer_get_type (),
				     "CellRendererColor", &info, 0);
    }

  return type;
}

static void
cell_renderer_color_class_init (gpointer klass, gpointer data)
{
  parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cell_renderer_color_finalize;
  object_class->get_property = cell_renderer_color_get_property;
  object_class->set_property = cell_renderer_color_set_property;
  
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);
  cell_class->get_size = cell_renderer_color_get_size;
  cell_class->render = cell_renderer_color_render;
  
  g_object_class_install_property (object_class,
                                   PROP_COLORLIST,
                                   g_param_spec_pointer ("colorlist",
                                                        "Colorlist",
                                                         "List of colors to display.",
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_uint ("width",
                                                      "Width",
                                                      "Display cell width.",
                                                       1, 160, 20,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEIGHT,
                                   g_param_spec_uint ("height",
                                                      "Height",
                                                      "Display cell height.",
                                                       1, 160, 20,
                                                       G_PARAM_READWRITE));
}

static void
cell_renderer_color_init (CellRendererColor *renderer)
{
  GTK_CELL_RENDERER(renderer)->mode = GTK_CELL_RENDERER_MODE_INERT;
  GTK_CELL_RENDERER(renderer)->xpad = 2;
  GTK_CELL_RENDERER(renderer)->ypad = 2;
  renderer->colorlist = NULL;
  renderer->cell_width  = 20;
  renderer->cell_height = 20;
}

static void
cell_renderer_color_finalize (GObject *object)
{
  CellRendererColor *renderer = CELL_RENDERER_COLOR (object);

  /* free remaining resources */
  if (renderer->colorlist)
    {
      g_array_free (renderer->colorlist, TRUE);
      renderer->colorlist = NULL;
    }
    
  (* G_OBJECT_CLASS (num_parent_class)->finalize) (object);
}

static void
cell_renderer_color_get_property (GObject    *object,
                              guint       param_id,
                              GValue     *value,
                              GParamSpec *psec)
{
  CellRendererColor  *renderer = CELL_RENDERER_COLOR (object);

  switch (param_id)
  {
    case PROP_COLORLIST:
      g_value_set_pointer(value, renderer->colorlist);
      break;
    case PROP_WIDTH:
      g_value_set_uint(value, renderer->cell_width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint(value, renderer->cell_height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
      break;
  }
}


static void
cell_renderer_color_set_property (GObject      *object,
                                            guint         param_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  CellRendererColor *renderer = CELL_RENDERER_COLOR (object);

  switch (param_id)
  {
    case PROP_COLORLIST:
      renderer->colorlist = g_value_get_pointer (value);
      break;
    case PROP_WIDTH:
      renderer->cell_width = g_value_get_uint (value);
      break;
    case PROP_HEIGHT:
      renderer->cell_height = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
  }
}


static void
cell_renderer_color_get_size (GtkCellRenderer *cell, GtkWidget *widget,
                              GdkRectangle *cell_area, 
                              gint *x_offset, gint *y_offset,
                              gint *width, gint *height)
{
  CellRendererColor *renderer = CELL_RENDERER_COLOR (cell);

  guint calc_width;
  guint calc_height;

  calc_width  = cell->xpad * 2 + renderer->cell_width;
  calc_height = cell->ypad * 2 + renderer->cell_height;

  if (width)
    *width = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
  {
    if (x_offset)
    {
      *x_offset = cell->xalign * (cell_area->width - calc_width);
      *x_offset = MAX (*x_offset, 0);
    }

    if (y_offset)
    {
      *y_offset = cell->yalign * (cell_area->height - calc_height);
      *y_offset = MAX (*y_offset, 0);
    }
  }
}

static void
cell_renderer_color_render (GtkCellRenderer *cell,
                            GdkWindow *window, GtkWidget *widget,
                            GdkRectangle *background_area, 
                            GdkRectangle *cell_area,
                            GdkRectangle *expose_area,
                            GtkCellRendererState flags)
{
  CellRendererColor *renderer = CELL_RENDERER_COLOR (cell);
  gint i, colwidth;
  
  if ((!renderer->colorlist) || (!renderer->colorlist->len))
    return;

  colwidth = renderer->cell_width / renderer->colorlist->len;
  
  for (i = 0; i < renderer->colorlist->len; i++)
    {
      static GdkColor color;
      GdkGC *gc = gdk_gc_new (GDK_DRAWABLE (window));
        
      color = g_array_index (renderer->colorlist, GdkColor, i);
      gdk_gc_set_foreground (gc, &color);
      gdk_draw_rectangle (GDK_DRAWABLE (window), gc, TRUE, 
                          i * colwidth + cell_area->x, cell_area->y + 1, 
                          colwidth, cell_area->height);
      g_object_unref (gc);
    }
}

GtkCellRenderer *
cell_renderer_color_new (void)
{
  return g_object_new (GTK_TYPE_CELL_RENDERER_COLOR, NULL);
}
