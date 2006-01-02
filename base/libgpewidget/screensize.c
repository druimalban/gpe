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

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

gint
gdk_screen_width (void)
{
  gint x, y;
  guint w, h, b, d;
  Window r;
  XGetGeometry (GDK_DISPLAY (), GDK_ROOT_WINDOW (), &r, &x, &y, &w, &h, &b, &d);
  return w;
}

gint
gdk_screen_height (void)
{
  gint x, y;
  guint w, h, b, d;
  Window r;
  XGetGeometry (GDK_DISPLAY (), GDK_ROOT_WINDOW (), &r, &x, &y, &w, &h, &b, &d);
  return h;
}
