/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

gint
gdk_screen_width (void)
{
  guint x, y, w, h, b, d;
  Window r;
  XGetGeometry (GDK_DISPLAY (), GDK_ROOT_WINDOW (), &r, &x, &y, &w, &h, &b, &d);
  return w;
}

gint
gdk_screen_height (void)
{
  guint x, y, w, h, b, d;
  Window r;
  XGetGeometry (GDK_DISPLAY (), GDK_ROOT_WINDOW (), &r, &x, &y, &w, &h, &b, &d);
  return h;
}
