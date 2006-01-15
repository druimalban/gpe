/*
 * Copyright (C) 2003, 2006 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <libintl.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/gpewindowlist.h>
#include <gpe/picturebutton.h>
#include <gpe/launch.h>

#define _(_x) gettext (_x)

struct gpe_icon my_icons[] = {
  { "icon", PREFIX "/share/pixmaps/gpe-taskmanager.png" },
  { "kill", "gpe-taskmanager/xkill" },
  { NULL, NULL }
};

Display *dpy;
GtkListStore *list_store;
GtkWidget *list_view;
Window my_w;

Atom atoms[4];

char *atom_names[] = 
  {
    "WM_PROTOCOLS",
    "_NET_WM_PING",
    "WM_DELETE_WINDOW",
    "_NET_CLIENT_LIST",
    "_NET_ACTIVE_WINDOW",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_WINDOW_TYPE_DOCK",
  };

#define WM_PROTOCOLS 0
#define _NET_WM_PING 1
#define WM_DELETE_WINDOW 2
#define _NET_CLIENT_LIST 3
#define _NET_ACTIVE_WINDOW 4
#define _NET_WM_WINDOW_TYPE 5
#define _NET_WM_WINDOW_TYPE_DESKTOP 6
#define _NET_WM_WINDOW_TYPE_DOCK 7

void
add_window (Display *dpy, Window w)
{
  GtkTreeIter iter;
  gchar *name;
  GdkPixbuf *icon;
  Atom type;

  /* Don't add panels or desktop windows */
  type = gpe_get_window_property (dpy, w, atoms[_NET_WM_WINDOW_TYPE]);
  if (type == atoms[_NET_WM_WINDOW_TYPE_DOCK] || type == atoms[_NET_WM_WINDOW_TYPE_DESKTOP])
    return;

  name = gpe_get_window_name (dpy, w);
  icon = gpe_get_window_icon (dpy, w); 
  
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, name, 1, w, -1);
  if (icon)
    {
      GdkPixbuf *icons = gdk_pixbuf_scale_simple (icon, 16, 16, GDK_INTERP_BILINEAR);
      gdk_pixbuf_unref (icon);
      gtk_list_store_set (list_store, &iter, 2, icons, -1);      
    }
}

void
set_highlight (Display *dpy)
{
  Window *wp;
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  
  if (XGetWindowProperty (dpy, DefaultRootWindow (dpy), atoms[_NET_ACTIVE_WINDOW],
			  0, 4, False, XA_WINDOW, &type, &format, &nitems, &bytes_after, 
			  (unsigned char **)&wp) == Success)
    {
      if (wp)
	{
	  Window w;

	  w = *wp;
	  if (w != 0 && w != my_w)
	    {
	      GtkTreeIter iter;

	      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
		{
		  Window iw;

		  do
		    {
		      gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 1, &iw, -1);

		      if (iw == w)
			{
			  GtkTreePath *path;

			  path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_store), &iter);

			  gtk_tree_view_set_cursor (GTK_TREE_VIEW (list_view), path, NULL, FALSE);

			  gtk_tree_path_free (path);
			  break;
			}
		    }
		  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
		}
	    }

	  XFree (wp);
	}
    }
}

void
update_list (Display *dpy)
{
  Window *list;
  guint nr, i;
  GtkTreeIter iter;
  char *p;

  if (gpe_get_client_window_list (dpy, &list, &nr) == FALSE)
    return;

  p = g_malloc0 (nr);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      gboolean more;

      do 
	{
	  gboolean found = FALSE;
	  Window w;

	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 1, &w, -1);

	  for (i = 0; i < nr; i++)
	    {
	      if (list[i] == w)
		{
		  p[i] = 1;
		  found = TRUE;
		  break;
		}
	    }

	  if (found)
	    more = gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter);
	  else
	    more = gtk_list_store_remove (list_store, &iter);
	} 
      while (more);
    }

  for (i = 0; i < nr; i++)
    {
      if (p[i] == 0 && list[i] != my_w)
	add_window (dpy, list[i]);
    }

  g_free (p);
}

GdkFilterReturn
window_filter (GdkXEvent *xev, GdkEvent *gev, gpointer d)
{
  XEvent *ev = (XEvent *)xev;
  Display *dpy = ev->xany.display;

  if (ev->xany.type == PropertyNotify
      && ev->xproperty.window == DefaultRootWindow (dpy))
    {
      if (ev->xproperty.atom == atoms[_NET_CLIENT_LIST])
	update_list (dpy);
      else if (ev->xproperty.atom == atoms[_NET_ACTIVE_WINDOW])
	set_highlight (dpy);
    }

  return GDK_FILTER_CONTINUE;
}

gboolean
send_delete_message (Display *dpy, Window w)
{
  XEvent e;

  e.type = ClientMessage;
  e.xclient.window = w;
  e.xclient.message_type = atoms[WM_PROTOCOLS];
  e.xclient.format = 32;
  e.xclient.data.l[0] = atoms[WM_DELETE_WINDOW];
  e.xclient.data.l[1] = CurrentTime;

  gdk_error_trap_push ();

  XSendEvent (dpy, w, False, NoEventMask, &e);
  XFlush (dpy);

  if (gdk_error_trap_pop ())
    return FALSE;

  return TRUE;
}

gboolean
really_kill_client (Display *dpy, Window w)
{
  gdk_error_trap_push ();

  XKillClient (dpy, w);
  XFlush (dpy);

  if (gdk_error_trap_pop ())
    return FALSE;

  return TRUE;
}

gboolean
kill_window (Display *dpy, Window w)
{
  Atom *protocols;
  int count, rc;

  gdk_error_trap_push ();

  rc = XGetWMProtocols (dpy, w, &protocols, &count);

  if (gdk_error_trap_pop ())
    return FALSE;

  if (rc)
    {
      int i;
      gboolean delete_supported = FALSE;

      for (i = 0; i < count; i++)
	{
	  if (protocols[i] == WM_DELETE_WINDOW)
	    delete_supported = TRUE;
	}

      XFree (protocols);

      if (delete_supported)
	return send_delete_message (dpy, w);
    }

  return really_kill_client (dpy, w);
}

gboolean
kill_task (GtkWidget *w, GtkWidget *list_view)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;
  
  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      Window w;

      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &w, -1);

      kill_window (dpy, w);
    }
  else
    gpe_error_box (_("No program is selected"));

  return TRUE;
}

static void
row_activated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, GtkTreeModel *model)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      Window w;

      gtk_tree_model_get (model, &iter, 1, &w, -1);

      gpe_launch_activate_window (dpy, w);
    }
}

void
task_manager (void)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *kill_button, *close_button;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  list_store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_OBJECT);
  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), FALSE);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Icon"), renderer, "pixbuf", 2, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  g_signal_connect (G_OBJECT (list_view), "row-activated", G_CALLBACK (row_activated), 
		    GTK_TREE_MODEL (list_store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Running programs"), renderer,
						     "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled), list_view);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), scrolled, TRUE, TRUE, 0);

  close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  kill_button = gpe_picture_button (NULL, _("Kill"), "kill");

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), close_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), kill_button, FALSE, FALSE, 0);
  
  gdk_window_add_filter (NULL, window_filter, NULL);

  XSelectInput (dpy, DefaultRootWindow (dpy), PropertyChangeMask);

  update_list (dpy);

  g_signal_connect (G_OBJECT (kill_button), "clicked", G_CALLBACK (kill_task), list_view);
  g_signal_connect (G_OBJECT (close_button), "clicked", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (gtk_main_quit), NULL);

  gtk_window_set_default_size (GTK_WINDOW (window), -1, 128);
  gtk_window_set_title (GTK_WINDOW (window), _("Running programs"));
  gpe_set_window_icon (window, "icon");

  gtk_widget_show_all (window);

  my_w = GDK_WINDOW_XWINDOW (window->window);
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
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  dpy = GDK_DISPLAY ();

  XInternAtoms (dpy, atom_names, sizeof (atom_names) / sizeof (atom_names[0]),
		False, atoms);
  
  task_manager ();

  gtk_main ();

  exit (0);
}
