/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

GtkWidget *
gpe_render_icon (GtkStyle *style, GdkPixbuf *pixbuf)
{
  if (pixbuf == NULL)
    abort ();

  return gtk_image_new_from_pixbuf (pixbuf);
}
