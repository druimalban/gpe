/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>

#include <gpe/stylus.h>

static GdkCursor *nocursor;

static void
_gdk_window_set_cursor (GdkWindow *window,
			GdkCursor *cursor)
{
  Cursor xcursor;

#if GTK_MAJOR_VERSION < 2
  GdkWindowPrivate *window_private;
  GdkCursorPrivate *cursor_private;

  g_return_if_fail (window != NULL);

  window_private = (GdkWindowPrivate*) window;
  cursor_private = (GdkCursorPrivate*) cursor;
  
  if (!cursor)
    xcursor = None;
  else
    xcursor = cursor_private->xcursor;

  if (!window_private->destroyed)
    XDefineCursor (window_private->xdisplay, window_private->xwindow, xcursor);
#else
  if (!cursor)
    xcursor = None;
  else
    xcursor = gdk_x11_cursor_get_xcursor (cursor);
  
  if (!GDK_WINDOW_DESTROYED (window))
    XDefineCursor (GDK_WINDOW_XDISPLAY (window), GDK_WINDOW_XWINDOW (window), xcursor);
#endif
}

void
gdk_window_set_cursor (GdkWindow *window,
		       GdkCursor *cursor)
{
  if (gpe_stylus_mode ())
    {
      if (! nocursor)
	{
	  char bits = 0;
	  GdkColor fg = { 0, 0, 0, 0 };
	  GdkBitmap *pix = gdk_bitmap_create_from_data (NULL, &bits, 1, 1);
	  GdkBitmap *mask = gdk_bitmap_create_from_data (NULL, &bits, 1, 1);
	  nocursor = gdk_cursor_new_from_pixmap (pix, mask,
						 &fg, &fg,
						 1, 1);
	}

      _gdk_window_set_cursor (window, nocursor);
    }
  else
    _gdk_window_set_cursor (window, cursor);
}
