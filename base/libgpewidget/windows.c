/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <gdk/gdk.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "windows.h"

gboolean
gpe_get_client_window_list (Display *dpy, Window **list, guint *nr)
{
  Atom net_client_list_atom = XInternAtom (dpy, "_NET_CLIENT_LIST", False);
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after = 0;
  unsigned char *prop = NULL;
  
  if (XGetWindowProperty (dpy, DefaultRootWindow (dpy), net_client_list_atom,
			  0, G_MAXLONG, False, XA_WINDOW, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop) != Success)
    return FALSE;

  *list = (Window *)prop;
  *nr = (guint)nitems;

  return TRUE;
}

gchar *
gpe_get_window_name (Display *dpy, Window w)
{
  Atom actual_type;
  Atom net_wm_name_atom = XInternAtom (dpy, "_NET_WM_NAME", False);
  Atom utf8_string_atom = XInternAtom (dpy, "UTF8_STRING", False);
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gchar *name = NULL;
  int rc;

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, net_wm_name_atom,
			  0, G_MAXLONG, False, utf8_string_atom, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop);

  gdk_error_trap_pop ();

  if (rc != Success)
    return FALSE;

  if (nitems)
    {
      name = g_strdup (prop);
      XFree (prop);
    }
  else
    {
      gdk_error_trap_push ();

      rc = XGetWindowProperty (dpy, w, XA_WM_NAME,
			       0, G_MAXLONG, False, XA_STRING, &actual_type, &actual_format,
			       &nitems, &bytes_after, &prop);

      if (gdk_error_trap_pop ())
	return FALSE;

      if (rc != Success)
	return FALSE;

      if (nitems)
	{
	  name = g_locale_to_utf8 (prop, -1, NULL, NULL, NULL);
	  XFree (prop);
	}
    }

  return name;
}

GdkPixbuf *
gpe_get_window_icon (Display *dpy, Window w)
{
  Atom actual_type;
  Atom net_wm_icon_atom = XInternAtom (dpy, "_NET_WM_ICON", False);
  int actual_format;
  unsigned long nitems, bytes_after;
  gulong *prop = NULL;
  int rc;
  GdkPixbuf *pixbuf = NULL;

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, net_wm_icon_atom,
			  0, G_MAXLONG, False, XA_CARDINAL, &actual_type, &actual_format,
			  &nitems, &bytes_after, (guchar **)&prop);

  if (gdk_error_trap_pop ())
    return FALSE;

  if (rc != Success)
    return FALSE;

  if (nitems)
    {
      guint w = prop[0], h = prop[1];
      guint i;
      guchar *pixels = g_malloc (w * h * 4);
      guchar *p = pixels;
      
      for (i = 0; i < w * h; i++)
	{
	  gulong l = prop[2 + i];
	  *(p++) = (l & 0x00ff0000) >> 16;
	  *(p++) = (l & 0x0000ff00) >> 8;
	  *(p++) = (l & 0x000000ff);
	  *(p++) = (l & 0xff000000) >> 24;
	}
      
      pixbuf = gdk_pixbuf_new_from_data (pixels,
					 GDK_COLORSPACE_RGB,
					 TRUE,
					 8,
					 w, h,
					 w * 4,
					 g_free,
					 NULL);
    }

  if (prop)
    XFree (prop);

  return pixbuf;
}
