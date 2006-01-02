/*
 * Copyright (C) 2003, 2004 Philip Blundell <philb@gnu.org>
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

#include <string.h>

#include <glib.h>
#include <gdk/gdk.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "gpewindowlist.h"
#include "link-warning.h"

static gboolean initialized;

static Atom atoms[6];
static char *atom_names[] = 
  {
    "_NET_CLIENT_LIST",
    "_NET_WM_NAME",
    "_NET_WM_ICON",
    "UTF8_STRING",
    "_NET_ACTIVE_WINDOW",
    "WM_CLIENT_LEADER"
  };

#define _NET_CLIENT_LIST 0
#define _NET_WM_NAME 1
#define _NET_WM_ICON 2
#define UTF8_STRING 3
#define _NET_ACTIVE_WINDOW 4
#define WM_CLIENT_LEADER 5

static void
initialize (Display *dpy)
{
  if (initialized)
    return;

  XInternAtoms (dpy, atom_names, sizeof (atoms) / sizeof (atoms[0]), False, atoms);

  initialized = TRUE;
}

gboolean
gpe_get_client_window_list (Display *dpy, Window **list, guint *nr)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after = 0;
  unsigned char *prop = NULL;
  int rc;

  initialize (dpy);
  
  rc = XGetWindowProperty (dpy, DefaultRootWindow (dpy), atoms[_NET_CLIENT_LIST],
			   0, G_MAXLONG, False, XA_WINDOW, &actual_type, &actual_format,
			   &nitems, &bytes_after, &prop);

  if (rc != Success || prop == NULL)
    return FALSE;

  *nr = (guint)nitems;

  *list = g_malloc (sizeof (Window) * nitems);
  memcpy (*list, prop, sizeof (Window) * nitems);
  XFree (prop);

  return TRUE;
}

link_warning (gpe_get_client_window_list, "gpe_get_client_window_list is obsolescent: use GPEWindowList instead");

gboolean
gpe_get_wm_class (Display *dpy, Window w, gchar **instance, gchar **class)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *uprop = NULL;
  char *prop;
  int rc;

  initialize (dpy);

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, XA_WM_CLASS,
			   0, G_MAXLONG, False, XA_STRING, &actual_type, &actual_format,
			   &nitems, &bytes_after, &uprop);

  prop = (char *)uprop;

  if (gdk_error_trap_pop () || rc != Success || prop == NULL)
    return FALSE;

  if (instance)
    *instance = g_strdup (prop);
  if (class)
    *class = g_strdup (prop + strlen (prop) + 1);
  XFree (prop);
  return TRUE;
}

Window
gpe_get_wm_leader (Display *dpy, Window w)
{
  Window result = None;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  int rc;

  initialize (dpy);

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, atoms[WM_CLIENT_LEADER],
			  0, 1, False, XA_WINDOW, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop);

  if (gdk_error_trap_pop () || rc != Success)
    return None;

  if (prop)
    {
      memcpy (&result, prop, sizeof (result));
      XFree (prop);
    }
  return result;
}

Atom
gpe_get_window_property (Display *dpy, Window w, Atom property)
{
  Atom result = None;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  int rc;

  initialize (dpy);

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, property,
			  0, 1, False, XA_ATOM, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop);

  if (gdk_error_trap_pop () || rc != Success)
    return None;

  if (prop)
    {
      memcpy (&result, prop, sizeof (result));
      XFree (prop);
    }
  return result;
}

gchar *
gpe_get_window_name (Display *dpy, Window w)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *uprop = NULL;
  char *prop;
  gchar *name = NULL;
  int rc;

  initialize (dpy);

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, atoms[_NET_WM_NAME],
			  0, G_MAXLONG, False, atoms[UTF8_STRING], &actual_type, &actual_format,
			  &nitems, &bytes_after, &uprop);

  prop = (char *)uprop;

  if (gdk_error_trap_pop () || rc != Success)
    return NULL;

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
			       &nitems, &bytes_after, &uprop);

      prop = (char *)uprop;

      if (gdk_error_trap_pop () || rc != Success)
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
  unsigned char *data = NULL;
  int rc;
  GdkPixbuf *pixbuf = NULL;

  initialize (dpy);

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, atoms[_NET_WM_ICON],
			  0, G_MAXLONG, False, XA_CARDINAL, &actual_type, &actual_format,
			  &nitems, &bytes_after, &data);

  if (gdk_error_trap_pop () || rc != Success)
    return NULL;

  if (nitems)
    {
      guint *prop = (guint *)data;
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

  if (data)
    XFree (data);

  return pixbuf;
}
