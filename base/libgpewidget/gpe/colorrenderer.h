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

#ifndef CELL_RENDERER_COLOR_H
#define CELL_RENDERER_COLOR_H

#include <gtk/gtk.h>
#include <glib-object.h>

#define GTK_TYPE_CELL_RENDERER_COLOR (cell_renderer_color_get_type ())
#define CELL_RENDERER_COLOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CELL_RENDERER_COLOR, CellRendererColor))
#define CELL_RENDERER_COLOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            GTK_TYPE_CELL_RENDERER_COLOR, CellRendererColorClass))
#define CELL_RENDERER_COLOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              GTK_TYPE_CELL_RENDERER_COLOR, CellRendererColorClass))

typedef struct
{
  GtkCellRendererClass parent_class;
} CellRendererColorClass;

static GObjectClass *num_parent_class;

typedef struct
{
  GtkCellRenderer parent;
    
  GArray *colorlist;
  guint  cell_height;
  guint  cell_width;
} CellRendererColor;

GtkCellRenderer *cell_renderer_color_new (void);


#endif
