/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
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
#include <gdk/gdkx.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "gpewindowlist.h"

static guint my_signals[2];

struct _GPEWindowListClass 
{
  GObjectClass parent_class;
  void (*list_changed)           (GPEWindowList *sm);
  void (*active_window_changed)  (GPEWindowList *sm);
};

static GdkFilterReturn window_filter (GdkXEvent *xev, GdkEvent *gev, gpointer d);

static void
gpe_window_list_init (GPEWindowList *list)
{
}

static void
gpe_window_list_setup_for_screen (GPEWindowList *list, GdkScreen *screen)
{
  GdkAtom net_client_list, net_active_window;
  GdkWindow *root;

  list->screen = screen;

  net_client_list = gdk_atom_intern ("_NET_CLIENT_LIST", FALSE);
  net_active_window = gdk_atom_intern ("_NET_ACTIVE_WINDOW", FALSE);

  list->net_client_list_atom = gdk_x11_atom_to_xatom_for_display (gdk_screen_get_display (list->screen),
								  net_client_list);
  list->net_active_window_atom = gdk_x11_atom_to_xatom_for_display (gdk_screen_get_display (list->screen),
								    net_active_window);

  root = gdk_screen_get_root_window (list->screen);

  gdk_window_add_filter (root, window_filter, list);

  XSelectInput (GDK_WINDOW_XDISPLAY (root), GDK_WINDOW_XID (root), PropertyChangeMask);
}

static void
gpe_window_list_fini (GPEWindowList *list)
{
  gdk_window_remove_filter (gdk_screen_get_root_window (list->screen), window_filter, list);
}

static void
gpe_window_list_list_changed (GPEWindowList *i)
{
  g_signal_emit (G_OBJECT (i), my_signals[0], 0);
}

static void
gpe_window_list_active_window_changed (GPEWindowList *i)
{
  g_signal_emit (G_OBJECT (i), my_signals[1], 0);
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
gpe_window_list_new (GdkScreen *screen)
{
  GObject *obj;

  obj = g_object_new (gpe_window_list_get_type (), NULL);

  gpe_window_list_setup_for_screen (GPE_WINDOW_LIST (obj), screen);

  return obj;
}

static GdkFilterReturn
window_filter (GdkXEvent *xev, GdkEvent *gev, gpointer d)
{
  XEvent *ev = (XEvent *)xev;
  GPEWindowList *list = GPE_WINDOW_LIST (d);

  if (ev->xany.type == PropertyNotify)
    {
      if (ev->xproperty.atom == list->net_client_list_atom)
	gpe_window_list_list_changed (list);
      else if (ev->xproperty.atom == list->net_active_window_atom)
	gpe_window_list_active_window_changed (list);
    }

  return GDK_FILTER_CONTINUE;
}

gboolean
gpe_window_list_get_clients (GPEWindowList *list, Window **ret, guint *nr)
{
  GdkWindow *root;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after = 0;
  unsigned char *prop = NULL;

  root = gdk_screen_get_root_window (list->screen);

  if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root), GDK_WINDOW_XID (root), 
			  list->net_client_list_atom,
			  0, G_MAXLONG, False, XA_WINDOW, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop) != Success)
    return FALSE;

  *nr = (guint)nitems;
  *ret = g_malloc0 (sizeof (Window) * nitems);
  memcpy (*ret, prop, sizeof (Window) * nitems);

  XFree (prop);
  return TRUE;
}
