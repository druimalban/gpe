/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

static GdkAtom string_gdkatom, display_change_gdkatom, display_change_ok_gdkatom;

void
do_change_display (GtkWidget *w, char *display_name, int screen_nr)
{
  GdkDisplay *newdisplay = gdk_display_open (display_name);
  if (newdisplay)
    {
      GdkScreen *screen = gdk_display_get_screen (newdisplay, screen_nr);
      if (screen)
	gtk_window_set_screen (GTK_WINDOW (w), screen);
    }
}

static GdkFilterReturn 
filter_func (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XPropertyEvent *xev = (XPropertyEvent *)xevp;

  if (xev->type == PropertyNotify)
    {
      unsigned char *prop = NULL;
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      GdkDisplay *gdisplay;
      Atom string_atom, display_change_atom;

      gdisplay = gdk_x11_lookup_xdisplay (xev->display);
      string_atom = gdk_x11_atom_to_xatom_for_display (gdisplay, string_gdkatom);
      display_change_atom = gdk_x11_atom_to_xatom_for_display (gdisplay, display_change_gdkatom);

      if (XGetWindowProperty (xev->display, xev->window, display_change_atom, 0, 65536, False, 
			      string_atom, &actual_type, &actual_format, &nitems, &bytes_after,
			      &prop) == Success)
	{
	  if (actual_type == string_atom
	      && nitems != 0)
	    {
	      GtkWidget *widget;
	      GdkWindow *gwindow;
	      int screen_nr = *(prop++);

	      gwindow = gdk_window_lookup_for_display (gdisplay, xev->window);
	      if (gwindow)
		{
		  gdk_window_get_user_data (gwindow, (gpointer*) &widget);
		  if (widget)
		    do_change_display (widget, prop, screen_nr);
		}
	    }

	  XDeleteProperty (xev->display, xev->window, display_change_atom);

	  if (prop)
	    XFree (prop);
	}

      return GDK_FILTER_REMOVE;
    }

  return GDK_FILTER_CONTINUE;
}

void
libdm_mark_window (GtkWidget *w)
{
  if (GTK_WIDGET_REALIZED (w))
    {
      GdkWindow *window = w->window;
      
      gdk_window_add_filter (window, filter_func, NULL);
      
      gdk_property_change (window, display_change_ok_gdkatom, display_change_ok_gdkatom,
			   32, GDK_PROP_MODE_REPLACE, NULL, 0);
    }
  else
    gtk_signal_connect (GTK_OBJECT (w), "realized", GTK_SIGNAL_FUNC (libdm_mark_window), NULL);
}

void
libdm_init (void)
{
  string_gdkatom = gdk_atom_intern ("STRING", FALSE);
  display_change_gdkatom = gdk_atom_intern ("_GPE_DISPLAY_CHANGE", FALSE);
  display_change_ok_gdkatom = gdk_atom_intern ("_GPE_DISPLAY_CHANGE_OK", FALSE);
}
