/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <libintl.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>

#define _(x) gettext(x)

Display *dpy;
Atom migrate_ok_atom;
Atom migrate_atom;
Atom string_atom;

GtkListStore *list_store;

struct gpe_icon
my_icons[] = 
  {
    { "ok" },
    { NULL }
  };

void
send_message (Display *dpy, Window w, char *host, int screen)
{
  char buf[256];
  buf[0] = screen;
  strcpy (buf + 1, host);
  
  XChangeProperty (dpy, w, migrate_atom, string_atom, 8, PropModeReplace, buf, strlen (host) + 1);

  XFlush (dpy);
}

static int
handle_click (Window w)
{
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty (dpy, w, migrate_ok_atom, 0, 0, False,
			  None, &type, &format, &nitems, &bytes_after, &prop) != Success
      || type == None)
    {
      Window root, parent, *children;
      unsigned int nchildren;  
  
      XQueryTree (dpy, w, &root, &parent, &children, &nchildren);
      if (children)
	XFree (children);

      if (root != w && root != parent)
	return handle_click (parent);

      return None;
    }

  if (prop)
    XFree (prop);

  return w;
}

static Window 
find_deepest_window (Display *dpy, Window grandfather, Window parent,
		     int x, int y, int *rx, int *ry)
{
  int dest_x, dest_y;
  Window child;
  
  XTranslateCoordinates (dpy, grandfather, parent, x, y,
			 &dest_x, &dest_y, &child);

  if (child == None)
    {
      *rx = dest_x;
      *ry = dest_y;

      return parent;
    }
  
  return find_deepest_window(dpy, parent, child, dest_x, dest_y, rx, ry);
}

void
add_callback (void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *ok = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  GtkWidget *cancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *host_label = gtk_label_new (_("Host"));
  GtkWidget *host_entry = gtk_entry_new ();
  GtkWidget *host_box = gtk_hbox_new (FALSE, 0);
  GtkWidget *dpy_label = gtk_label_new (_("Display"));
  GtkWidget *screen_label = gtk_label_new (_("Screen"));
  GtkWidget *dpy_spin = gtk_spin_button_new_with_range (0, 255, 1);
  GtkWidget *screen_spin = gtk_spin_button_new_with_range (0, 255, 1);
  GtkWidget *dpy_box = gtk_hbox_new (FALSE, 0);
  GtkWidget *screen_box = gtk_hbox_new (FALSE, 0);
  GtkWidget *sep = gtk_hseparator_new ();

  gtk_box_pack_start (GTK_BOX (host_box), host_label, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (host_box), host_entry, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (screen_box), screen_label, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (screen_box), screen_spin, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (dpy_box), dpy_label, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (dpy_box), dpy_spin, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (hbox), ok, TRUE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), cancel, TRUE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), host_box, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), dpy_box, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), screen_box, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);
  
  gtk_widget_show_all (window);
}

void
open_window (void)
{
  GtkWidget *window;
  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *go_button;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *scrolled_window;
  GtkWidget *list_view;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_widget_realize (window);

  add_button = gpe_button_new_from_stock (GTK_STOCK_ADD, GPE_BUTTON_TYPE_ICON);
  remove_button = gpe_button_new_from_stock (GTK_STOCK_REMOVE, GPE_BUTTON_TYPE_ICON);
  go_button = gpe_button_new_from_stock (GTK_STOCK_YES, GPE_BUTTON_TYPE_ICON);

  g_signal_connect (G_OBJECT (add_button), "clicked", G_CALLBACK (add_callback), NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  gtk_container_add (GTK_CONTAINER (scrolled_window), list_view);

  gtk_box_pack_start (GTK_BOX (hbox), add_button, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), remove_button, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), go_button, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

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

  list_store = gtk_list_store_new (1, G_TYPE_STRING);

  open_window ();

  gtk_main ();

  exit (0);
}
