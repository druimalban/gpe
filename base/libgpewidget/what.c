/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

static Display *dpy;
static Window root;
static Atom atom;

static GtkWidget *widget;

static gboolean query_on, showing;

GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  
  if (xev->type == PropertyNotify)
    {
      if (xev->xproperty.atom == atom)
	{
	  unsigned char *prop;
	  Atom type;
	  int format;
	  unsigned long nitems;
	  unsigned long bytes;
	  XGetWindowProperty (dpy, root, atom, 0, 1, 0, XA_INTEGER,
			      &type, &format, &nitems, &bytes,
			      &prop);
	  if (*prop)
	    {
	      query_on = TRUE;
	      gtk_tips_query_start_query (GTK_TIPS_QUERY (widget));
	    }
	  else
	    {
	      if (query_on)
		gtk_tips_query_stop_query (GTK_TIPS_QUERY (widget));
	    }
	}
    }

  return GDK_FILTER_CONTINUE;
}

static void
do_widget (GtkWidget *tips, GtkWidget *widget, gchar *text, gchar *text_private, gpointer p)
{
  GtkWidget *window = p;
  guint x, y;

  gdk_window_get_pointer (NULL, &x, &y, NULL);
  y += 16;
  gtk_widget_set_uposition (window, MAX (x, 0), MAX (y, 0));

  if (!showing)
    {
      showing = TRUE;
      gtk_widget_show (window);
    }
}

static void
stop_query (GtkWidget *w, GtkWidget *window)
{
  char b = 0;
  gtk_widget_hide (window);
  XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, &b, 1);
  query_on = FALSE;
  showing = FALSE;
}

void
what_init (void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);
  dpy = GDK_DISPLAY ();
  root = GDK_ROOT_WINDOW ();
  atom = XInternAtom (dpy, "GPE_WHAT", 0);

  XSelectInput (dpy, root, PropertyChangeMask);

  gdk_window_add_filter (GDK_ROOT_PARENT (), filter, 0);

  widget = gtk_tips_query_new ();
  gtk_widget_show (widget);
  gtk_container_add (GTK_CONTAINER (window), widget);
  gtk_widget_show (window);

  gtk_signal_connect (widget, "widget-entered", do_widget, window);
  gtk_signal_connect (widget, "stop-query", stop_query, window);
}
