/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <libintl.h>

#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#define _(_x) gettext (_x)

struct gpe_icon my_icons[] = {
  { "icon", PREFIX "/share/pixmaps/gpe-calendar.png" },
  { NULL, NULL }
};

Display *dpy;
GtkListStore *list_store;

Atom atoms[9];

char *atom_names[] = 
  {
    "WINDOW",
    "_NET_CLIENT_LIST",
    "STRING",
    "UTF8_STRING",
    "WM_NAME",
    "_NET_WM_NAME",
    "WM_PROTOCOLS",
    "_NET_WM_PING",
    "WM_DELETE_WINDOW",
  };

#define WINDOW 0
#define _NET_CLIENT_LIST 1
#define STRING 2
#define UTF8_STRING 3
#define WM_NAME 4
#define _NET_WM_NAME 5
#define WM_PROTOCOLS 6
#define _NET_WM_PING 7
#define WM_DELETE_WINDOW 8

gboolean
get_client_windows (Window **list, guint *nr)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after = 0;
  unsigned char *prop = NULL;
  unsigned long length = 0;
  
  do 
    {
      length += bytes_after;
      
      if (prop)
	XFree (prop);
      
      if (XGetWindowProperty (dpy, DefaultRootWindow (dpy), atoms[_NET_CLIENT_LIST],
			      0, length, False, atoms[WINDOW], &actual_type, &actual_format,
			      &nitems, &bytes_after, &prop) != Success)
	return FALSE;
    }
  while (bytes_after);

  *list = (Window *)prop;
  *nr = (guint)nitems;

  return TRUE;
}

gchar *
get_window_name (Window w)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gchar *name = NULL;
 
  if (XGetWindowProperty (dpy, w, atoms[_NET_WM_NAME],
			  0, 65536, False, atoms[UTF8_STRING], &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop) != Success)
    return FALSE;

  if (nitems)
    {
      name = g_strdup (prop);
      XFree (prop);
    }
  else
    {
      if (XGetWindowProperty (dpy, w, atoms[WM_NAME],
			      0, 65536, False, atoms[STRING], &actual_type, &actual_format,
			      &nitems, &bytes_after, &prop) != Success)
	return FALSE;

      if (nitems)
	{
	  name = g_locale_to_utf8 (prop, -1, NULL, NULL, NULL);
	  XFree (prop);
	}
    }

  return name;
}

void
add_window (Window w)
{
  GtkTreeIter iter;
  gchar *name = get_window_name (w);

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, name, 1, w, -1);
}

void
update_list (void)
{
  Window *list;
  guint nr, i;
  GtkTreeIter iter;
  char *p;

  if (get_client_windows (&list, &nr) == FALSE)
    exit (1);

  p = g_malloc0 (nr);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      for (;;)
	{
	  gboolean found = FALSE;
	  Window w;

	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 1, &w, -1);

	  for (i = 0; i < nr; i++)
	    if (list[i] == w)
	      {
		p[i] = 1;
		found = TRUE;
		break;
	      }

	  if (found)
	    {
	      if (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter) == FALSE)
		break;
	    }

	  if (gtk_list_store_remove (list_store, &iter) == FALSE)
	    break;
	} 
    }

  for (i = 0; i < nr; i++)
    {
      if (p[i] == 0)
	add_window (list[i]);
    }

  g_free (p);
}

GdkFilterReturn
window_filter (GdkXEvent *xev, GdkEvent *gev, gpointer d)
{
  XEvent *ev = (XEvent *)xev;
  Display *dpy = ev->xany.display;

  if (ev->xany.type == PropertyNotify
      && ev->xproperty.window == DefaultRootWindow (dpy)
      && ev->xproperty.atom == atoms[_NET_CLIENT_LIST])
    update_list ();

  return GDK_FILTER_CONTINUE;
}

void
task_manager (void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *list_view;
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Tasks"), renderer,
						     "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  gtk_box_pack_start (GTK_BOX (vbox), list_view, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gdk_window_add_filter (NULL, window_filter, NULL);

  XSelectInput (dpy, DefaultRootWindow (dpy), PropertyChangeMask);

  update_list ();

  gtk_window_set_default_size (GTK_WINDOW (window), 200, 128);
  gtk_window_set_title (GTK_WINDOW (window), _("Tasks"));
  gpe_set_window_icon (window, "icon");

  gtk_widget_realize (window);

  gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  dpy = GDK_DISPLAY ();

  XInternAtoms (dpy, atom_names, sizeof (atom_names) / sizeof (atom_names[0]),
		False, atoms);
  
  task_manager ();

  gtk_main ();

  exit (0);
}
