/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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

#include <glib.h>
#include <gdk/gdk.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "gpewindowlist.h"

static gboolean initialized;

static Atom atoms[5];
static char *atom_names[] = 
  {
    "_NET_CLIENT_LIST",
    "_NET_WM_NAME",
    "_NET_WM_ICON",
    "UTF8_STRING",
    "_NET_ACTIVE_WINDOW"
  };

#define _NET_CLIENT_LIST 0
#define _NET_WM_NAME 1
#define _NET_WM_ICON 2
#define UTF8_STRING 3
#define _NET_ACTIVE_WINDOW 4

static void
initialize (Display *dpy)
{
  if (initialized)
    return;

  XInternAtoms (dpy, atom_names, 5, False, atoms);

  initialized = TRUE;
}

gboolean
gpe_get_client_window_list (Display *dpy, Window **list, guint *nr)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after = 0;
  unsigned char *prop = NULL;

  initialize (dpy);
  
  if (XGetWindowProperty (dpy, DefaultRootWindow (dpy), atoms[_NET_CLIENT_LIST],
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
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gchar *name = NULL;
  int rc;

  initialize (dpy);

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, atoms[_NET_WM_NAME],
			  0, G_MAXLONG, False, atoms[UTF8_STRING], &actual_type, &actual_format,
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
  int actual_format;
  unsigned long nitems, bytes_after;
  gulong *prop = NULL;
  int rc;
  GdkPixbuf *pixbuf = NULL;

  initialize (dpy);

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, atoms[_NET_WM_ICON],
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
					 (GdkPixbufDestroyNotify)g_free,
					 NULL);
    }

  if (prop)
    XFree (prop);

  return pixbuf;
}
