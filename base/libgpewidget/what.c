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

#include "render.h"
#include "pixmaps.h"
#include "what.h"

static Display *dpy;
static Window root;
static Atom atom;

static GtkWidget *widget;
static GtkWidget *label;

static gboolean query_on, showing;

static GdkFilterReturn
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

  gtk_label_set_text (GTK_LABEL (label), text);

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
  XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, &b, 1);
  query_on = FALSE;
  showing = FALSE;
}

static void
close_clicked (GtkWidget *w, GdkEventButton *ev, GtkWidget *ww)
{
  gtk_widget_hide (ww);
}

void
what_init (void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);
  GtkStyle *style;
  GdkColor col;
  GtkWidget *hbox;
  GtkWidget *close, *closei;

  dpy = GDK_DISPLAY ();
  root = GDK_ROOT_WINDOW ();
  atom = XInternAtom (dpy, "GPE_WHAT", 0);

  gdk_color_parse ("yellow", &col);

  XSelectInput (dpy, root, PropertyChangeMask);

  gdk_window_add_filter (GDK_ROOT_PARENT (), filter, 0);

  style = gtk_style_copy (window->style);
  style->bg[GTK_STATE_NORMAL] = col;
  gtk_widget_set_style (window, style);

  widget = gtk_tips_query_new ();
  label = gtk_label_new ("");
  gtk_widget_show (label);

  closei = gpe_render_icon (window->style, gpe_find_icon ("cancel"));
  gtk_widget_show (closei);
  close = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (close), closei);
  gtk_widget_show (close);
  gtk_button_set_relief (GTK_BUTTON (close), GTK_RELIEF_NONE);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), close, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), hbox);
  gtk_widget_realize (window);
  gtk_widget_realize (widget);

  gtk_signal_connect (GTK_OBJECT (widget), "widget-entered", 
		      do_widget, window);
  gtk_signal_connect (GTK_OBJECT (widget), "stop-query", 
		      stop_query, window);

  gtk_signal_connect (GTK_OBJECT (close), "clicked",
		      close_clicked, window);
}
