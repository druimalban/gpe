/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <ctype.h>
#include <libintl.h>
#include <netinet/in.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "libdm.h"

#define _(x) gettext(x)

static GdkAtom string_gdkatom, display_change_gdkatom;

static gboolean
do_change_display (GtkWidget *w, char *display_name,  gchar **error)
{
  GdkDisplay *newdisplay;
  guint screen_nr = 1;
  guint i;

  if (display_name[0] == 0)
    {
      *error = g_strdup (_("Null display name not permitted"));
      return FALSE;
    }

  i = strlen (display_name) - 1;
  while (i > 0 && isdigit (display_name[i]))
    i--;

  if (display_name[i] == '.')
    {
      screen_nr = atoi (display_name + i + 1);
      display_name[i] = 0;
    }

  newdisplay = gdk_display_open (display_name);
  if (newdisplay)
    {
      GdkScreen *screen = gdk_display_get_screen (newdisplay, screen_nr);
      if (screen)
	{
	  gtk_window_set_screen (GTK_WINDOW (w), screen);
	  return TRUE;
	}
      else
	*error = g_strdup_printf (_("Screen %d does not exist on target display"), screen_nr);
    }
  else
    *error = g_strdup_printf (_("Couldn't connect to display %s"), display_name);

  return FALSE;
}

static GdkFilterReturn 
filter_func (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XPropertyEvent *xev = (XPropertyEvent *)xevp;

  if (xev->type == PropertyNotify)
    {
      GdkDisplay *gdisplay;
      GdkWindow *gwindow;

      gdisplay = gdk_x11_lookup_xdisplay (xev->display);
      gwindow = gdk_window_lookup_for_display (gdisplay, xev->window);

      if (gwindow)
	{
	  GdkAtom actual_type;
	  gint actual_format;
	  gint actual_length;
	  unsigned char *prop = NULL;

	  if (gdk_property_get (gwindow, display_change_gdkatom, display_change_gdkatom,
				0, 65536, FALSE, &actual_type, &actual_format,
				&actual_length, &prop))
	    {
	      if (actual_length != 0)
		{
		  if (actual_type == display_change_gdkatom
		      && actual_length > 8)
		    {
		      GtkWidget *widget;

		      gdk_window_get_user_data (gwindow, (gpointer*) &widget);
		      if (widget)
			{
			  gchar *error = NULL;
			  if (do_change_display (widget, prop + 8, &error) == FALSE)
			    {
			      Window initiating_window;
			      Atom error_atom;
			      Atom string_atom = gdk_x11_atom_to_xatom_for_display (gdisplay, 
										    string_gdkatom);

			      initiating_window = ntohl (*((uint32_t *)prop));
			      error_atom = ntohl (*((uint32_t *)(prop + 4)));

			      if (initiating_window && error_atom)
				XChangeProperty (xev->display, initiating_window, 
						 error_atom, string_atom,
						 8, PropModeReplace,
						 error, strlen (error));

			      g_free (error);
			    }
			}
		    }

		  gdk_property_change (gwindow, display_change_gdkatom, display_change_gdkatom,
				       32, GDK_PROP_MODE_REPLACE, NULL, 0);
		}
	    }

	  if (prop)
	    g_free (prop);
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
      
      gdk_property_change (window, display_change_gdkatom, display_change_gdkatom,
			   32, GDK_PROP_MODE_REPLACE, NULL, 0);
    }
  else
    gtk_signal_connect (GTK_OBJECT (w), "realize", GTK_SIGNAL_FUNC (libdm_mark_window), NULL);
}

void
libdm_init (void)
{
  string_gdkatom = gdk_atom_intern ("STRING", FALSE);
  display_change_gdkatom = gdk_atom_intern ("_GPE_DISPLAY_CHANGE", FALSE);
}
