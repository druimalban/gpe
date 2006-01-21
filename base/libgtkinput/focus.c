/*
 * Copyright (C) 2006 Philip Blundell <philb@gnu.org>
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

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

static GSList *displays;

struct display_record
{
  Display *dpy;
  Window input_manager;
  Atom gpe_input_manager_atom;
};

static gboolean
get_manager_for_display (Display *dpy, Window *manager, Atom *atom)
{
  GSList *l;
  struct display_record *r;

  for (l = displays; l; l = l->next)
    {
      r = l->data;

      if (r->dpy == dpy)
	break;
    }

  if (l == NULL)
    {
      r = g_new (struct display_record, 1);
      r->dpy = dpy;
      r->gpe_input_manager_atom = XInternAtom (dpy, "_GPE_INPUT_MANAGER", False);
      r->input_manager = XGetSelectionOwner (dpy, r->gpe_input_manager_atom);
      displays = g_slist_prepend (displays, r);
    }

  *manager = r->input_manager;
  *atom = r->gpe_input_manager_atom;
  return r->input_manager ? TRUE : FALSE;
}

static gboolean
focus_watcher (GSignalInvocationHint *ihint,
	       guint                  n_param_values,
	       const GValue          *param_values,
	       gpointer               data)
{
  GObject *object;
  GtkWidget *widget;
  GdkEvent *event;

  object = g_value_get_object (param_values + 0);
  if (! GTK_IS_WIDGET (object))
    return FALSE;

  event = g_value_get_boxed (param_values + 1);
  widget = GTK_WIDGET (object);

  if (event->type == GDK_FOCUS_CHANGE && widget->parent == NULL)
    {
      GdkWindow *gdkw;
      Display *dpy;
      Window manager;
      Atom atom;
 
      gdkw = widget->window;
      dpy = gdk_x11_drawable_get_xdisplay (gdkw);

      if (get_manager_for_display (dpy, &manager, &atom))
	{
	  Window w;
	  XClientMessageEvent ev;

	  w = gdk_x11_drawable_get_xid (gdkw);

	  memset (&ev, 0, sizeof (ev));

	  gdk_error_trap_push ();
	  ev.type = ClientMessage;
	  ev.format = 32;
	  ev.message_type = atom;
	  ev.data.l[0] = event->focus_change.in ? 1 : 2;
	  ev.data.l[1] = w;
	  XSendEvent (dpy, manager, False, SubstructureNotifyMask, (XEvent *)&ev);
	  XFlush (dpy);
	  gdk_error_trap_pop ();
	}
    }

  return TRUE;
}

void
gtk_module_init (gint *argc, gchar ***argv)
{
  gtk_type_class (GTK_TYPE_WINDOW);

  g_signal_add_emission_hook (g_signal_lookup ("event-after", GTK_TYPE_WINDOW), 0,
			      focus_watcher, NULL, (GDestroyNotify) NULL);
}
