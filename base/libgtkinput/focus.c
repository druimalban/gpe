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
static GdkAtom gpe_input_manager;

struct display_record
{
  Display *dpy;
  Window input_manager;
  Atom atom;
};

static gboolean
send_focus_request (GdkWindow *gdkw, gboolean in)
{
  GdkDisplay *gdpy;
  Display *dpy;
  Window manager;
  Atom atom;
  GSList *l;
  struct display_record *r;

#ifdef DEBUG
  fprintf (stderr, "sending request %d\n", in);
#endif

  gdpy = gdk_drawable_get_display (gdkw);
  dpy = gdk_x11_display_get_xdisplay (gdpy);

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
      r->atom = gdk_x11_atom_to_xatom_for_display (gdpy, gpe_input_manager);
      r->input_manager = XGetSelectionOwner (dpy, r->atom);
      displays = g_slist_prepend (displays, r);

      if (!gdk_display_request_selection_notification (gdpy, gpe_input_manager))
	fprintf (stderr, "couldn't register for selection notifications\n");
    }

  manager = r->input_manager;
  atom = r->atom;

  if (manager)
    {
      Window w;
      XClientMessageEvent ev;

      w = gdk_x11_drawable_get_xid (gdkw);

      memset (&ev, 0, sizeof (ev));

      gdk_error_trap_push ();
      ev.type = ClientMessage;
      ev.format = 32;
      ev.message_type = atom;
      ev.data.l[0] = in ? 1 : 2;
      ev.data.l[1] = w;
      XSendEvent (dpy, manager, False, SubstructureNotifyMask, (XEvent *)&ev);
      XFlush (dpy);
      gdk_error_trap_pop ();

      return TRUE;
    }

  return FALSE;
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

  if (event->type == GDK_FOCUS_CHANGE)
    {
#ifdef DEBUG
      fprintf (stderr, "focus event %d %x\n", event->focus_change.in, widget);
#endif
      if (GTK_IS_EDITABLE (widget) || GTK_IS_TEXT_VIEW (widget))
	send_focus_request (widget->window, event->focus_change.in);
    }
  else if (event->type == GDK_OWNER_CHANGE)
    {
      if (event->owner_change.selection == gpe_input_manager)
	{
	  printf ("selection owner has changed\n");
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

  gpe_input_manager = gdk_atom_intern ("_GPE_INPUT_MANAGER", FALSE);
}
