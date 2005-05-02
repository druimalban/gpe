/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <libintl.h>
#include <stdio.h>

#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

static GSList *widgets;
static gboolean filter_installed;
static GdkAtom locale_atom;

static void
remove_widget (GObject *o)
{
  widgets = g_slist_remove (widgets, o);
}

static GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;

  if (xev->type == PropertyNotify)
    {
      GdkDisplay *gdisplay;
      Atom atom;

      gdisplay = gdk_x11_lookup_xdisplay (xev->xany.display);
      atom = gdk_x11_atom_to_xatom_for_display (gdisplay, locale_atom);

      if (xev->xproperty.atom == atom)
	{
	  Atom actual_type;
	  int actual_format;
	  unsigned long actual_length;
	  unsigned long bytes_after;
	  unsigned char *prop = NULL;

	  if (XGetWindowProperty (xev->xany.display, xev->xany.window, 
				  atom, 0, 65536, False, XA_STRING,
				  &actual_type, &actual_format,
				  &actual_length, &bytes_after, &prop) == Success
	      && actual_length > 0
	      && actual_type == XA_STRING)
	    {
	      GSList *iter;

	      setlocale (LC_MESSAGES, prop);

	      for (iter = widgets; iter; iter = iter->next)
		{
		  GtkWidget *w = GTK_WIDGET (iter->data);
		  void (*func)(GtkWidget *, void *) = g_object_get_data (G_OBJECT (w), 
									 "translate-callback");
		  gpointer data = g_object_get_data (G_OBJECT (w), "translate-callback-data");

		  if (func)
		    func (w, data);
		}
	    }

	  if (prop)
	    XFree (prop);

	  return TRUE;
	}
    }

  return FALSE;
}

static void
label_translation (GtkWidget *w, void *pointer)
{
  gchar *domain = g_object_get_data (G_OBJECT (w), "translate-domain");
  gchar *string = g_object_get_data (G_OBJECT (w), "translate-string");
		  
  gtk_label_set_text (GTK_LABEL (w), dgettext (domain, string));
}

/**
 * gtk_widget_add_translation_hook:
 * @w: A widget.
 * @func: A callback function.
 * @data: Passed as the second argument when the callback is invoked.
 * 
 * Register a callback to be invoked when the current locale changes.
 */
void
gtk_widget_add_translation_hook (GtkWidget *w, void (*func)(GtkWidget *, void *), gpointer data)
{
  widgets = g_slist_append (widgets, w);

  g_signal_connect (G_OBJECT (w), "destroy", G_CALLBACK (remove_widget), NULL);

  g_object_set_data (G_OBJECT (w), "translate-callback", func);
  g_object_set_data (G_OBJECT (w), "translate-callback-data", data);

  if (! filter_installed)
    {
      Display *dpy = GDK_DISPLAY ();

      XSelectInput (dpy, RootWindow (dpy, DefaultScreen (dpy)),
		    PropertyChangeMask);

      locale_atom = gdk_atom_intern ("_GPE_LOCALE", FALSE);

      gdk_window_add_filter (GDK_ROOT_PARENT (), filter, NULL);

      filter_installed = TRUE;
    }
  
  func (w, data);
}

/**
 * gtk_label_new_with_translation:
 * @domain: Textual domain used for this program.
 * @string: Message string to be displayed.
 * @Returns: a GtkWidget
 * 
 * Creates a translation-aware label widget.  The supplied string is
 * passed through gettext prior to display, and automatically updated
 * if the selected locale is changed.
 */
GtkWidget *
gtk_label_new_with_translation (gchar *domain, gchar *string)
{
  gchar *initial_value = dgettext (domain, string);
  GtkWidget *w = gtk_label_new (initial_value);

  g_object_set_data (G_OBJECT (w), "translate-domain", domain);
  g_object_set_data (G_OBJECT (w), "translate-string", string);

  gtk_widget_add_translation_hook (w, label_translation, NULL);

  return w;
}
