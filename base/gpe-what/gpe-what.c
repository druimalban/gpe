/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include "tray.h"

#define _(x) gettext(x)

static Atom atom;
static Display *dpy;
static Window root;
static Window mywindow;

struct gpe_icon my_icons[] = {
  { "what" },
  { NULL }
};

static void
clicked (GtkWidget *w)
{
  char b = 1;
  XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, &b, 1);
}

static GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  
  if (xev->type == ClientMessage || xev->type == ReparentNotify)
    {
      XAnyEvent *any = (XAnyEvent *)xev;
      tray_handle_event (any->display, mywindow, xev);
      if (xev->type == ReparentNotify)
	gtk_widget_show (GTK_WIDGET (p));
    }
  return GDK_FILTER_CONTINUE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *button;
  GtkWidget *icon;
  Atom window_type_atom, window_type_dock_atom;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window, 16, 16);
  gtk_widget_realize (window);
  
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gpe_render_icon (window->style, gpe_find_icon ("what"));
  gtk_widget_show (icon);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), icon);
  gtk_widget_show (button);

  gtk_signal_connect (GTK_OBJECT (button), "clicked", clicked, NULL);

  gtk_container_add (GTK_CONTAINER (window), button);

  dpy = GDK_WINDOW_XDISPLAY (window->window);
  root = RootWindow (dpy, 0);
  atom = XInternAtom (dpy, "GPE_WHAT", 0);

  window_type_atom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
  window_type_dock_atom = XInternAtom (dpy,
				       "_NET_WM_WINDOW_TYPE_DOCK", False);

  mywindow = GDK_WINDOW_XWINDOW (window->window);
  XChangeProperty (dpy, mywindow, 
		   window_type_atom, XA_ATOM, 32, 
		   PropModeReplace, (unsigned char *)
		   &window_type_dock_atom, 1);

  tray_init (dpy, GDK_WINDOW_XWINDOW (window->window));
  gdk_window_add_filter (window->window, filter, window);

  gtk_main ();

  exit (0);
}
