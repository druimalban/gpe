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

#include <stdio.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>

#define _(x) gettext(x)

Atom migrate_ok_atom;
Atom migrate_atom;
Atom string_atom;
GdkWindow *grab_window;
GtkListStore *list_store;

gboolean grabbed;

struct gpe_icon
my_icons[] = 
  {
    { "ok" },
    { NULL }
  };

struct display
{
  gchar *host;
  guint dpy;
  guint screen;
  gchar *str;
};

static GSList *displays;
struct display *selected_dpy;

void
send_message (Display *dpy, Window w, char *host, int display, int screen)
{
  char buf[256];
  memset (buf, 0, 8);
  sprintf (buf + 8, "%s:%d.%d", host, display, screen);
  
  XChangeProperty (dpy, w, migrate_atom, migrate_atom, 8, PropModeReplace, buf, 8 + strlen (buf + 8));

  XFlush (dpy);
}

static int
handle_click (Display *dpy, Window w)
{
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty (dpy, w, migrate_atom, 0, 0, False,
			  migrate_atom, &type, &format, &nitems, &bytes_after, &prop) != Success
      || type != migrate_atom)
    {
      Window root, parent, *children;
      unsigned int nchildren;  
  
      XQueryTree (dpy, w, &root, &parent, &children, &nchildren);
      if (children)
	XFree (children);

      if (root != w && root != parent)
	return handle_click (dpy, parent);

      return None;
    }

  if (nitems)
    return None;

  if (prop)
    XFree (prop);

  return w;
}

static Window 
find_deepest_window (Display *dpy, Window grandfather, Window parent,
		     int x, int y)
{
  int dest_x, dest_y;
  Window child;
  
  XTranslateCoordinates (dpy, grandfather, parent, x, y,
			 &dest_x, &dest_y, &child);

  if (child == None)
    return parent;
  
  return find_deepest_window(dpy, parent, child, dest_x, dest_y);
}

void
add_cancel (GtkWidget *w, GtkWidget *window)
{
  gtk_widget_destroy (window);
}

void
add_ok (GtkWidget *w, GtkWidget *window)
{
  GtkWidget *host_entry = g_object_get_data (G_OBJECT (window), "entry");
  GtkWidget *dpy_spin = g_object_get_data (G_OBJECT (window), "dpy_spin");
  GtkWidget *screen_spin = g_object_get_data (G_OBJECT (window), "screen_spin");
  GtkAdjustment *dpy_adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (dpy_spin));
  GtkAdjustment *screen_adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (screen_spin));
  const gchar *host = gtk_entry_get_text (GTK_ENTRY (host_entry));
  guint dpy = (guint)gtk_adjustment_get_value (dpy_adjustment);
  guint screen = (guint)gtk_adjustment_get_value (screen_adjustment);
  struct display *d = g_malloc (sizeof (struct display));
  GtkTreeIter iter;

  if (host[0] == 0)
    {
      gpe_error_box (_("Must specify hostname"));
      return;
    }

  d->host = g_strdup (host);
  d->dpy = dpy;
  d->screen = screen;
  d->str = g_strdup_printf ("%s:%d.%d", host, dpy, screen);

  displays = g_slist_append (displays, d);

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, d->str, 1, d, -1);
 
  gtk_widget_destroy (window);
}

void
remove_callback (GtkWidget *button, GtkWidget *list_view)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      struct display *d;
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &d, -1);
      displays = g_slist_remove (displays, d);
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      g_free (d->host);
      g_free (d->str);
      g_free (d);
    }
  else
    gpe_error_box (_("No display selected"));
}

void
go_callback (GtkWidget *button, GtkWidget *list_view)
{
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      Display *dpy = GDK_DISPLAY ();

      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &selected_dpy, -1);

      grabbed = TRUE;

      XGrabPointer (dpy, RootWindow (dpy, 0), False, ButtonReleaseMask,
		    GrabModeAsync, GrabModeAsync,
		    None, None, CurrentTime);
    }
  else
    gpe_error_box (_("No display selected"));
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

  g_object_set_data (G_OBJECT (window), "entry", host_entry);
  g_object_set_data (G_OBJECT (window), "dpy_spin", dpy_spin);
  g_object_set_data (G_OBJECT (window), "screen_spin", screen_spin);

  g_signal_connect (G_OBJECT (ok), "clicked", G_CALLBACK (add_ok), window);
  g_signal_connect (G_OBJECT (cancel), "clicked", G_CALLBACK (add_cancel), window);

  gtk_window_set_title (GTK_WINDOW (window), _("New display"));

  gtk_widget_realize (window);
  gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_DIALOG);

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
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_widget_realize (window);

  add_button = gpe_button_new_from_stock (GTK_STOCK_ADD, GPE_BUTTON_TYPE_ICON);
  remove_button = gpe_button_new_from_stock (GTK_STOCK_REMOVE, GPE_BUTTON_TYPE_ICON);
  go_button = gpe_button_new_from_stock (GTK_STOCK_YES, GPE_BUTTON_TYPE_ICON);

  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Displays"), renderer,
						     "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  gtk_container_add (GTK_CONTAINER (scrolled_window), list_view);

  gtk_box_pack_start (GTK_BOX (vbox), add_button, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), remove_button, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), go_button, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_TOOLBAR);

  gtk_window_set_title (GTK_WINDOW (window), _("GPE Teleport"));

  gtk_window_set_default_size (GTK_WINDOW (window), 240, 64);

  g_signal_connect (G_OBJECT (add_button), "clicked", G_CALLBACK (add_callback), NULL);
  g_signal_connect (G_OBJECT (remove_button), "clicked", G_CALLBACK (remove_callback), list_view);
  g_signal_connect (G_OBJECT (go_button), "clicked", G_CALLBACK (go_callback), list_view);

  gtk_widget_show_all (window);
}

GdkFilterReturn
window_filter (GdkXEvent *xev, GdkEvent *gev, gpointer d)
{
  XEvent *ev = (XEvent *)xev;
  Display *dpy = ev->xany.display;

  if (ev->xany.type == ButtonRelease
      && ev->xbutton.window == RootWindow (dpy, 0))
    {
      Window w;

      XUngrabPointer (dpy, ev->xbutton.time);

      w = find_deepest_window (dpy, RootWindow (dpy, 0), RootWindow (dpy, 0),
			       ev->xbutton.x, ev->xbutton.y);

      XGrabServer (dpy);
      w = handle_click (dpy, w);
      if (w)
	send_message (dpy, w, selected_dpy->host, selected_dpy->dpy, selected_dpy->screen);
      XUngrabServer (dpy);

      if (w == None)
	gpe_error_box (_("Cannot migrate this application"));

      return GDK_FILTER_REMOVE;
    }

  return GDK_FILTER_CONTINUE;
}

int
main (int argc, char *argv[])
{
  Display *dpy;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  dpy = GDK_DISPLAY ();
  string_atom = XInternAtom (dpy, "STRING", False);
  migrate_atom = XInternAtom (dpy, "_GPE_DISPLAY_CHANGE", False);

  list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);

  gdk_window_add_filter (NULL, window_filter, NULL);

  open_window ();

  gtk_main ();

  exit (0);
}
