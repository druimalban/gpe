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
#include <stdlib.h>
#include <libintl.h>
#include <string.h>
#include <sys/stat.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/windows.h>

#include "displays.h"

#define _(x) gettext(x)

Atom migrate_atom;
Atom string_atom;
Atom challenge_atom;
GdkWindow *grab_window;
GtkListStore *list_store;

gboolean grabbed;

struct display *selected_dpy;

extern gchar *sign_challenge (gchar *text, int length, gchar *target);

Atom atoms[8];

char *atom_names[] = 
  {
    "_NET_CLIENT_LIST",
    "UTF8_STRING",
    "WM_NAME",
    "_NET_WM_NAME",
    "_NET_WM_ICON"
  };

#define _NET_CLIENT_LIST 0
#define UTF8_STRING 1
#define WM_NAME 2
#define _NET_WM_NAME 3
#define _NET_WM_ICON 4

struct client_window
{
  Window w;
  gchar *name;
  GdkPixbuf *icon;
};

struct gpe_icon my_icons[] = 
  {
    { "icon", PREFIX "/share/pixmaps/teleport.png" },
    { NULL }
  };

static void
send_message (Display *dpy, Window w, char *host, gchar *method, gchar *data)
{
  gchar *buf = g_strdup_printf ("%s %s %s", host, method, data);
  
  XChangeProperty (dpy, w, migrate_atom, string_atom, 8, PropModeReplace, buf, strlen (buf));
  XFlush (dpy);

  g_free (buf);
}

static void
migrate_to (Display *dpy, Window w, char *host, int display, int screen)
{
  gchar *auth = "NONE";
  gchar *data = NULL;
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gchar *target;

  target = g_strdup_printf ("%s:%d.%d", host, display, screen);

  if (XGetWindowProperty (dpy, w, challenge_atom, 0, 8192, False, string_atom,
			  &type, &format, &nitems, &bytes_after, &prop) == Success
      && type == string_atom && nitems != 0)
    {
      auth = "RSA-SIG";
      data = sign_challenge (prop, nitems, target);
      if (data == NULL)
	{
	  g_free (target);
	  if (prop)
	    XFree (prop);
	  return;
	}
    }

  if (prop)
    XFree (prop);

  send_message (dpy, w, target, auth, data ? data : "");

  g_free (target);

  if (data)
    g_free (data);
}

GSList *
get_clients (Display *dpy)
{
  Window *windows;
  guint nwin, i;
  GSList *clients = NULL;

  gpe_get_client_window_list (dpy, &windows, &nwin);
  for (i = 0; i < nwin; i++)
    {
      Window w = windows[i];
      gchar *name;
      struct client_window *cw;
      GdkPixbuf *icon;
 
#if 0
      if (! can_migrate (dpy, w))
	continue;
#endif

      name = gpe_get_window_name (dpy, w);

      icon = gpe_get_window_icon (dpy, w);

      cw = g_malloc (sizeof (*cw));
      cw->w = w;
      cw->name = name;
      if (icon)
	{
	  cw->icon = gdk_pixbuf_scale_simple (icon, 16, 16, GDK_INTERP_BILINEAR);
	  gdk_pixbuf_unref (icon);
	}
      else
	cw->icon = NULL;

      clients = g_slist_append (clients, cw);
    }

  return clients;
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
open_window (GSList *clients)
{
  GtkWidget *window;
  GtkWidget *quit_button;
  GtkWidget *go_button;
  GtkWidget *hbox1, *hbox2;
  GtkWidget *option_menu;
  GtkWidget *display_label;
  GtkWidget *display_combo;
  GtkWidget *client_label;
  GList *strings = NULL;
  GSList *i;
  GtkWidget *menu;

  for (i = displays; i; i = i->next)
    {
      struct display *d = i->data;
      strings = g_list_append (strings, d->str);
    }

  window = gtk_dialog_new ();

  hbox1 = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
  display_label = gtk_label_new (_("Display"));
  display_combo = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (display_combo), strings);
  gtk_box_pack_start (GTK_BOX (hbox1), display_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), display_combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox1, FALSE, FALSE, gpe_get_boxspacing ());

  hbox2 = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
  client_label = gtk_label_new (_("Window"));
  option_menu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_box_pack_start (GTK_BOX (hbox2), client_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), option_menu, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox2, FALSE, FALSE, gpe_get_boxspacing ());

  for (i = clients; i; i = i->next)
    {
      struct client_window *cw = i->data;
      GtkWidget *item = gtk_image_menu_item_new_with_label (cw->name);
      if (cw->icon)
	{
	  GtkWidget *image = gtk_image_new_from_pixbuf (cw->icon);
	  gtk_widget_show (image);
	  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	}

      gtk_widget_show (item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    }

  quit_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  go_button = gtk_button_new_from_stock (GTK_STOCK_EXECUTE);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (window)->action_area), go_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (window)->action_area), quit_button, FALSE, FALSE, 0);

  gtk_window_set_title (GTK_WINDOW (window), _("Teleport"));
  gpe_set_window_icon (GTK_WIDGET (window), "icon");

  g_signal_connect (G_OBJECT (quit_button), "clicked", G_CALLBACK (g_main_loop_quit), NULL);
  g_signal_connect (G_OBJECT (go_button), "clicked", G_CALLBACK (go_callback), NULL);

  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (g_main_loop_quit), NULL);

  gtk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  Display *dpy;
  gchar *home_dir, *d;
  GSList *clients;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  dpy = GDK_DISPLAY ();
  string_atom = XInternAtom (dpy, "STRING", False);
  migrate_atom = XInternAtom (dpy, "_GPE_DISPLAY_CHANGE", False);
  challenge_atom = XInternAtom (dpy, "_GPE_DISPLAY_CHANGE_RSA_CHALLENGE", False);

  home_dir = g_get_home_dir ();

  d = g_strdup_printf ("%s/.gpe", home_dir);
  mkdir (d, 0700);
  g_free (d);
  d = g_strdup_printf ("%s/.gpe/migrate", home_dir);
  mkdir (d, 0700);
  g_free (d);

  crypt_init ();

  displays_init ();

  XInternAtoms (dpy, atom_names, sizeof (atom_names) / sizeof (atom_names[0]),
		False, atoms);

  clients = get_clients (dpy);

  open_window (clients);

  gtk_main ();

  exit (0);
}
