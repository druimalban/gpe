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

static guint my_signals[2];

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

struct _GPEWindowListClass 
{
  GObjectClass parent_class;
  void (*list_changed)           (GPEWindowList *sm);
  void (*active_window_changed)  (GPEWindowList *sm, Window w);
};

static void
initialize (Display *dpy)
{
  if (initialized)
    return;

  XInternAtoms (dpy, atom_names, 5, False, atoms);

  initialized = TRUE;
}

static void
gpe_window_list_init (GPEWindowList *item)
{
}

void
gpe_window_list_list_changed (GPEWindowList *i)
{
  g_signal_emit (G_OBJECT (i), my_signals[0], 0);
}

void
gpe_window_list_active_window_changed (GPEWindowList *i, Window w)
{
  g_signal_emit (G_OBJECT (i), my_signals[1], 0, w);
}

static void
gpe_window_list_class_init (GPEWindowListClass * klass)
{
  my_signals[0] = g_signal_new ("list-changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEWindowListClass, list_changed),
				NULL, NULL,
				gtk_marshal_VOID__VOID,
				G_TYPE_NONE, 0);

  my_signals[1] = g_signal_new ("active-window-changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEWindowListClass, active_window_changed),
				NULL, NULL,
				gtk_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
gpe_window_list_fini (GPEWindowList *item)
{
}

GtkType
gpe_window_list_get_type (void)
{
  static GType item_type = 0;

  if (! item_type)
    {
      static const GTypeInfo info =
      {
	sizeof (GPEWindowListClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) gpe_window_list_fini,
	(GClassInitFunc) gpe_window_list_class_init,
	(GClassFinalizeFunc) NULL,
	NULL /* class_data */,
	sizeof (GPEWindowList),
	0 /* n_preallocs */,
	(GInstanceInitFunc) gpe_window_list_init,
      };

      item_type = g_type_register_static (G_TYPE_OBJECT, "GPEWindowList", &info, (GTypeFlags)0);
    }
  return item_type;
}

GObject *
gpe_window_list_new (GdkDisplay *dpy)
{
  GObject *obj;
  GPEWindowList *w;

  obj = g_object_new (gpe_window_list_get_type (), NULL);

  w = (GPEWindowList *)obj;

  w->dpy = dpy;

  return obj;
}

static GdkFilterReturn
window_filter (GdkXEvent *xev, GdkEvent *gev, gpointer d)
{
  XEvent *ev = (XEvent *)xev;
  Display *dpy = ev->xany.display;

  if (ev->xany.type == PropertyNotify
      && ev->xproperty.window == DefaultRootWindow (dpy))
    {
      if (ev->xproperty.atom == atoms[_NET_CLIENT_LIST])
	;
      else if (ev->xproperty.atom == atoms[_NET_ACTIVE_WINDOW])
	;
    }

  return GDK_FILTER_CONTINUE;
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
