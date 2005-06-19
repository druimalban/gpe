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
#include <stdlib.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "gpewindowlist.h"

static guint my_signals[4];

struct _GPEWindowListClass 
{
  GObjectClass parent_class;

  void (*list_changed)           (GPEWindowList *);
  void (*active_window_changed)  (GPEWindowList *);

  void (*window_added)		 (GPEWindowList *, Window w);
  void (*window_removed)	 (GPEWindowList *, Window w);
};

static GdkFilterReturn window_filter (GdkXEvent *xev, GdkEvent *gev, gpointer d);

static void
gpe_window_list_emit_list_changed (GPEWindowList *i)
{
  g_signal_emit (G_OBJECT (i), my_signals[0], 0);
}

static void
gpe_window_list_emit_active_window_changed (GPEWindowList *i)
{
  g_signal_emit (G_OBJECT (i), my_signals[1], 0);
}

static void
gpe_window_list_emit_window_added (GPEWindowList *i, Window w)
{
  g_signal_emit (G_OBJECT (i), my_signals[2], 0, w);
}

static void
gpe_window_list_emit_window_removed (GPEWindowList *i, Window w)
{
  g_signal_emit (G_OBJECT (i), my_signals[3], 0, w);
}

static int
window_cmp (const void *a, const void *b)
{
  return (*(Window *)a) - (*(Window *)b);
}

static GList *
remove_window (GPEWindowList *list, GList *link)
{
  GList *next = link->next;

  gpe_window_list_emit_window_removed (list, (Window)link->data);

  list->windows = g_list_remove_link (list->windows, link);
  g_list_free (link);

  return next;
}

static void
add_window_before (GPEWindowList *list, GList *p, Window x)
{
  list->windows = g_list_insert_before (list->windows, p, (void *)x);

  gpe_window_list_emit_window_added (list, x);
}

static gboolean
do_get_clients (GPEWindowList *list, Window **ret, guint *nr)
{
  GdkWindow *root;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after = 0;
  char *prop = NULL;

  root = gdk_screen_get_root_window (list->screen);

  if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root), GDK_WINDOW_XID (root), 
			  list->net_client_list_atom,
			  0, G_MAXLONG, False, XA_WINDOW, &actual_type, &actual_format,
			  &nitems, &bytes_after, (unsigned char **)&prop) != Success)
    return FALSE;

  *nr = (guint)nitems;
  *ret = g_malloc0 (sizeof (Window) * nitems);
  memcpy (*ret, prop, sizeof (Window) * nitems);

  XFree (prop);
  return TRUE;
}

static void
gpe_window_list_changed (GPEWindowList *list)
{
  Window *new_list;
  guint nr, i;
  GList *p;

  if (do_get_clients (list, &new_list, &nr) == FALSE)
    return;

  qsort (new_list, nr, sizeof (Window), window_cmp);

  for (i = 0, p = list->windows; i < nr;)
    {
      Window x;
      Window y;

      x = new_list[i];
      y = p ? ((Window)p->data) : None;

      if (x == y)
	{
	  i++;
	  p = p->next;
	  continue;
	}

      if (x < y || y == None)
	{
	  add_window_before (list, p, x);
	  i++;
	  continue;
	}

      p = remove_window (list, p);
    }

  while (p)
    p = remove_window (list, p);

  g_free (new_list);
}

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
				G_TYPE_NONE, 0);

  my_signals[2] = g_signal_new ("window-added",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEWindowListClass, window_added),
				NULL, NULL,
				gtk_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);

  my_signals[3] = g_signal_new ("window-removed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEWindowListClass, window_removed),
				NULL, NULL,
				gtk_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);
}

/**
 * gpe_window_list_get_type:
 * @Returns: a GType
 *
 * Returns the GType for GPEWindowList objects.
 */
GType
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

/**
 * gpe_window_list_new:
 * @screen: a screen
 *
 * Create a GPEWindowList object for the specified GdkScreen.
 */
GObject *
gpe_window_list_new (GdkScreen *screen)
{
  GObject *obj;
  GPEWindowList *list;

  obj = g_object_new (gpe_window_list_get_type (), NULL);

  list = GPE_WINDOW_LIST (obj);

  gpe_window_list_setup_for_screen (list, screen);

  gpe_window_list_changed (list);

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
	{
	  gpe_window_list_changed (list);
	  gpe_window_list_emit_list_changed (list);
	}
      else if (ev->xproperty.atom == list->net_active_window_atom)
	gpe_window_list_emit_active_window_changed (list);
    }

  return GDK_FILTER_CONTINUE;
}

/**
 * gpe_window_list_get_clients:
 * @list: a GPEWindowList
 * @Returns: a GList of Window pointers
 *
 * Retrieve a list of the active clients on the screen associated with this GPEWindowList.
 */
GList *
gpe_window_list_get_clients (GPEWindowList *list)
{
  return list->windows;
}
