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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "init.h"

#define _(x) gettext(x)

static Atom atom;
static Display *dpy;
static Window root;

static void
clicked (GtkWidget *w)
{
  char b = 1;
  XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, &b, 1);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *button;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (window);

  button = gtk_button_new_with_label ("What?");
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", clicked, NULL);

  gtk_container_add (GTK_CONTAINER (window), button);

  dpy = GDK_WINDOW_XDISPLAY (window->window);
  root = RootWindow (dpy, 0);
  atom = XInternAtom (dpy, "GPE_WHAT", 0);

  gtk_widget_show (window);

  gtk_main ();

  exit (0);
}
