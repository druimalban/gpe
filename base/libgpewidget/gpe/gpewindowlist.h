/*
 * Copyright (C) 2003, 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GPE_WINDOW_LIST_H
#define GPE_WINDOW_LIST_H

#include <gtk/gtk.h>
#include <glib-object.h>

#include <X11/Xlib.h>

#define GPE_TYPE_WINDOW_LIST            (gpe_window_list_get_type ())
#define GPE_WINDOW_LIST(obj)            G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_window_list_get_type(), GPEWindowList)
#define GPE_WINDOW_LIST_CONST(obj)	G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_window_list_get_type(), GPEWindowList const)
#define GPE_WINDOW_LIST_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST ((klass), gpe_window_list_get_type(), GPEIConListItemClass)
#define GPE_IS_WINDOW_LIST(obj)		G_TYPE_CHECK_INSTANCE_TYPE ((obj), gpe_window_list_get_type ())

#define GPE_WINDOW_LIST_GET_CLASS(obj)  G_TYPE_INSTANCE_GET_CLASS ((obj), gpe_window_list_get_type(), GPEWindowListClass)

struct _GPEWindowList
{
  GObject class;

  GdkScreen *screen;
  Atom net_client_list_atom;
  Atom net_active_window_atom;
};

typedef struct _GPEWindowList	      GPEWindowList;
typedef struct _GPEWindowListClass    GPEWindowListClass;

GtkType		 gpe_window_list_get_type (void);

GObject         *gpe_window_list_new (GdkScreen *);

gboolean         gpe_window_list_get_clients (GPEWindowList *, Window **, guint *nr);

extern gboolean  gpe_get_client_window_list (Display *dpy, Window **list, guint *nr);

extern gchar    *gpe_get_window_name (Display *dpy, Window w);

extern GdkPixbuf *gpe_get_window_icon (Display *dpy, Window w);

#endif
