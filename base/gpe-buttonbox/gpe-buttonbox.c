/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/gpewindowlist.h>
#include <gpe/popup.h>

#include <locale.h>

#include "config.h"

#define _(_x) gettext (_x)

GtkWidget *window;
GtkWidget *box;
Display *dpy;

GList *windows;
GList *classes;

int xpos;

gboolean other_button_visible;
GList *other_windows;
GtkWidget *other_button;

#define NR_SLOTS 4
#define SLOT_WIDTH 72
#define SLOT_MARGIN 8

#define _NET_WM_WINDOW_TYPE_DOCK 0
#define _NET_WM_WINDOW_TYPE 1
#define _NET_WM_STATE_HIDDEN 2
#define _NET_WM_SKIP_PAGER 3
#define _NET_WM_WINDOW_TYPE_DESKTOP 4

Atom atoms[5];

char *atom_names[] =
  {
    "_NET_WM_WINDOW_TYPE_DOCK",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_STATE_HIDDEN",
    "_NET_WM_SKIP_PAGER",
    "_NET_WM_WINDOW_TYPE_DESKTOP"
};

struct class_record *class_slot[NR_SLOTS];

struct window_record
{
  Window w;

  gchar *class;
  gchar *name;
  GdkPixbuf *icon;
};

struct class_record
{
  gint slot;
  gchar *name;
  GList *windows;
  GtkWidget *widget;
};

/* copied from libmatchbox */
static void
activate_window (Display *dpy, Window win)
{
  static Atom atom_net_active = None;
  XEvent ev;

  if (atom_net_active == None)
    atom_net_active = XInternAtom (dpy, "_NET_ACTIVE_WINDOW", False);

  memset (&ev, 0, sizeof ev);
  ev.xclient.type = ClientMessage;
  ev.xclient.window = win;
  ev.xclient.message_type = atom_net_active;
  ev.xclient.format = 32;

  XSendEvent (dpy, RootWindow (dpy, DefaultScreen (dpy)), False, SubstructureRedirectMask, &ev);
}

void
raise_window (struct window_record *r)
{
  activate_window (dpy, r->w);
}

void
popup_window_list (GtkWidget *widget, GdkEventButton *button, GList *windows)
{
  GtkWidget *menu;
  GList *l;
  
  menu = gtk_menu_new ();
  
  for (l = windows; l; l = l->next)
    {
      GtkWidget *item;
      struct window_record *r;
      
      r = l->data;
      item = gtk_image_menu_item_new_with_label (r->name);
      
      gtk_widget_show (item);
      
      if (r->icon)
	{
	  GtkWidget *icon;
	  GdkPixbuf *pix;
	  
	  pix = gdk_pixbuf_scale_simple (r->icon, 16, 16, GDK_INTERP_BILINEAR);
	  icon = gtk_image_new_from_pixbuf (pix);
	  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
	}
      
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (raise_window), r);
      
      gtk_menu_append (GTK_MENU (menu), item);
    }
  
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
		  gpe_popup_menu_position,
		  widget, button->button, button->time);
}

void
button_release_event (GtkWidget *widget, GdkEventButton *button, struct class_record *c)
{
  if (c->windows == NULL)
    abort ();

  if (c->windows->next != NULL)
    popup_window_list (widget, button, c->windows);
  else
    raise_window (c->windows->data);
}

void
other_button_release (GtkWidget *widget, GdkEventButton *button)
{
  popup_window_list (widget, button, other_windows);
}

void
desktop_button_release (GtkWidget *widget, GdkEventButton *button)
{
  static Atom atom_net_showing_desktop = None;
  XEvent ev;

  if (atom_net_showing_desktop == None)
    atom_net_showing_desktop = XInternAtom (dpy, "_NET_SHOWING_DESKTOP", False);

  memset (&ev, 0, sizeof ev);
  ev.xclient.type = ClientMessage;
  ev.xclient.window = None;
  ev.xclient.message_type = atom_net_showing_desktop;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = 1;

  XSendEvent (dpy, RootWindow (dpy, DefaultScreen (dpy)), False, SubstructureRedirectMask, &ev);
}

GtkWidget *
do_build_icon (gchar *name, GdkPixbuf *icon_pix)
{
  GtkWidget *wbox;
  GtkWidget *icon;
  GtkWidget *text;
  GtkWidget *ebox;
  GdkPixbuf *scaled;

  scaled = gdk_pixbuf_scale_simple (icon_pix, 32, 32, GDK_INTERP_BILINEAR);
  icon = gtk_image_new_from_pixbuf (scaled);
  text = gtk_label_new (name);
  wbox = gtk_vbox_new (FALSE, 0);
  ebox = gtk_event_box_new ();

  gtk_box_pack_start (GTK_BOX (wbox), icon, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (wbox), text, FALSE, FALSE, 0);

  gtk_widget_show (icon);
  gtk_widget_show (text);
  gtk_widget_show (wbox);

  gtk_widget_set_usize (GTK_WIDGET (wbox), SLOT_WIDTH - SLOT_MARGIN, 48);
  
  gtk_container_add (GTK_CONTAINER (ebox), wbox);

  return ebox;
}

GtkWidget *
build_icon_for_class (struct class_record *c)
{
  GtkWidget *ebox;
  struct window_record *r;

  r = c->windows->data;

  ebox = do_build_icon (r->name, r->icon);

  g_signal_connect (G_OBJECT (ebox), "button_release_event", G_CALLBACK (button_release_event), c);
  gtk_widget_add_events (GTK_WIDGET (ebox), GDK_BUTTON_RELEASE_MASK);

  return ebox;
}

struct class_record *
find_class_record (const gchar *name)
{
  struct class_record *c;
  GList *l;

  for (l = classes; l; l = l->next)
    {
      c = l->data;

      if (!strcmp (c->name, name))
	return c;
    }

  c = g_malloc0 (sizeof (*c));
  c->name = g_strdup (name);
  c->slot = -1;
  classes = g_list_append (classes, c);
  return c;
}

void
find_slot_for_class (struct class_record *c)
{
  if (c->slot == -1)
    {
      int i;

      for (i = 0; i < NR_SLOTS; i++)
	{
	  if (class_slot[i] == NULL)
	    {
	      class_slot[i] = c;
	      c->slot = i;
	      c->widget = build_icon_for_class (c);
	      gtk_fixed_put (GTK_FIXED (box), c->widget, (i + 1) * SLOT_WIDTH, 0);
	      gtk_widget_show (c->widget);
	      return;
	    }
	}
    }
}

static void
add_window_record (struct window_record *r)
{
  struct class_record *c;
  GList *l;

  if (r->icon)
    g_object_ref (r->icon);

  c = find_class_record (r->class);
  c->windows = g_list_append (c->windows, r);

  find_slot_for_class (c);

  if (c->slot == -1)
    {
      other_windows = g_list_append (other_windows, r);
      if (!other_button_visible)
	gtk_widget_show (other_button);
    }
}

static void
delete_window_record (struct window_record *r)
{
  struct class_record *c;

  c = find_class_record (r->class);

  c->windows = g_list_remove (c->windows, r);
  if (c->windows == NULL)
    {
      int slot;

      gtk_widget_hide (c->widget);
      gtk_widget_destroy (c->widget);
      slot = c->slot;
      class_slot[c->slot] = NULL;
      c->slot = -1;

      if (other_windows)
	{
	  struct window_record *rr;
	  struct class_record *cc;
	  GList *l;

	  rr = other_windows->data;
	  cc = find_class_record (rr->class);

	  for (l = cc->windows; l; l = l->next)
	    other_windows = g_list_remove (other_windows, l->data);

	  cc->widget = build_icon_for_class (cc);
	  gtk_widget_show (cc->widget);
	  gtk_fixed_put (GTK_FIXED (box), cc->widget, (slot + 1) * SLOT_WIDTH, 0);
	  cc->slot = slot;
	  class_slot[slot] = cc;
	}
    }
  
  if (c->slot == -1)
    other_windows = g_list_remove (other_windows, r);

  if (other_windows == NULL)
    gtk_widget_hide (other_button);

  g_free (r->name);
  if (r->icon)
    g_object_unref (r->icon);
  g_free (r);
}

static void
window_added (GPEWindowList *list, Window w)
{
  struct window_record *r;
  GtkWidget *widget;
  Atom type;
  Window leader;
  gchar *class;

  type = gpe_get_window_property (dpy, w, atoms[_NET_WM_WINDOW_TYPE]);
  if (type == atoms[_NET_WM_WINDOW_TYPE_DOCK] || type == atoms[_NET_WM_WINDOW_TYPE_DESKTOP])
    return;

  if (gpe_get_window_property (dpy, w, atoms[_NET_WM_STATE_HIDDEN]) != None)
    return;

  if (gpe_get_window_property (dpy, w, atoms[_NET_WM_SKIP_PAGER]) != None)
    return;

  if (gpe_get_wm_class (dpy, w, NULL, &class) == FALSE)
    return;

  leader = gpe_get_wm_leader (dpy, w);
  if (leader == None)
    leader = w;

  r = g_malloc0 (sizeof (*r));
  r->w = w;
  r->name = gpe_get_window_name (dpy, leader);
  if (r->name == NULL)
    r->name = gpe_get_window_name (dpy, w);
  r->icon = gpe_get_window_icon (dpy, w);
  r->class = class;

  add_window_record (r);

  windows = g_list_append (windows, r);
}

static void
window_removed (GPEWindowList *list, Window w)
{
  GList *l;

  for (l = windows; l; l = l->next)
    {
      struct window_record *r = l->data;

      if (r->w == w)
	{
	  delete_window_record (r);
	  windows = g_list_remove_link (windows, l);
	  g_list_free (l);
	  break;
	}
    }
}

void
add_initial_windows (GPEWindowList *list)
{
  GList *l;

  l = gpe_window_list_get_clients (list);

  while (l)
    {
      window_added (list, (Window)l->data);
      l = l->next;
    }
}

int
main (int argc, char *argv[])
{
  GObject *list;
  GtkWidget *desktop_button;
  GdkPixbuf *desk_icon;
  gboolean flag_panel = FALSE;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  while (1)
    {
      int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      int c;

      static struct option long_options[] = {
	{"panel", 0, 0, 'p'},
	{0, 0, 0, 0}
      };
      
      c = getopt_long (argc, argv, "p", long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 'p':
	  flag_panel = TRUE;
	  break;
	}
    }

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if (flag_panel)
    window = gtk_plug_new (0);
  else
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  box = gtk_fixed_new ();

  list = gpe_window_list_new (gdk_screen_get_default ());
  dpy = gdk_x11_get_default_xdisplay ();

  XInternAtoms (dpy, atom_names, 5, False, atoms);

  g_signal_connect (G_OBJECT (list), "window-added", G_CALLBACK (window_added), NULL);
  g_signal_connect (G_OBJECT (list), "window-removed", G_CALLBACK (window_removed), NULL);

  desk_icon = gdk_pixbuf_new_from_file ("/usr/share/pixmaps/gnome-show-desktop.png", NULL);

  desktop_button = do_build_icon ("Desktop", desk_icon);
  gtk_fixed_put (GTK_FIXED (box), desktop_button, 0, 0);
  g_signal_connect (G_OBJECT (desktop_button), "button_release_event", G_CALLBACK (desktop_button_release), NULL);
  gtk_widget_show (desktop_button);

  other_button = do_build_icon ("Other", NULL);
  gtk_fixed_put (GTK_FIXED (box), other_button, (NR_SLOTS + 1) * SLOT_WIDTH, 0);
  g_signal_connect (G_OBJECT (other_button), "button_release_event", G_CALLBACK (other_button_release), NULL);

  add_initial_windows (GPE_WINDOW_LIST (list));

  gtk_widget_set_usize (box, (NR_SLOTS + 2) * SLOT_WIDTH, -1);

  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show (box);
  gtk_widget_show (window);

  if (flag_panel)
    {
      gtk_widget_realize (window);
      gpe_system_tray_dock (window->window);
    }

  gtk_main ();

  return 0;
}
