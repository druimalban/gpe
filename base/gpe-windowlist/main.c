/*
 * gpe-windowlist - a window list and switcher
 *
 * Some code taken from minilite and gpe-buttonbox.
 * Copyright 2006 Florian Boor <florian@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

/* libgpewidget */
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/tray.h>
#include <gpe/popup.h>
#include <gpe/gpewindowlist.h>
#include <gpe/errorbox.h>

/* libgpelaunch */
#include <gpe/launch.h>
#include <gpe/desktop_file.h>

#include "config.h"
#include "globals.h"

#define _(_x)  gettext (_x)

struct gpe_icon my_icons[] = {
  { "windowlist", PREFIX "/share/gpe/pixmaps/default/windowlist.png" },
  { "other", PREFIX "/share/gpe/pixmaps/default/other-app.png" },
  { NULL, NULL }
};

struct timeout_record
{
  GPEWindowList *list;
  Window w;
};

static GList *windows;
static Display *dpy;
static GtkWidget *icon;
static GdkPixbuf *other_icon;

Atom *atoms;

struct window_record
{
  Window w;
  Window leader;

  gchar *class;
  gchar *icon_name;
  GdkPixbuf *icon;
};

void
raise_window (struct window_record *r)
{
  gpe_launch_activate_window (dpy, r->w);
}

static gchar *
gpe_get_wm_icon_name (Display *dpy, Window w)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gchar *name = NULL;
  int rc;

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, XA_WM_ICON_NAME,
			  0, G_MAXLONG, False, XA_STRING, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop);

  if (gdk_error_trap_pop () || rc != Success)
    return NULL;

  if (nitems)
    {
      name = g_strdup (prop);
      XFree (prop);
    }

  return name;
}


static void
window_added (GPEWindowList *list, Window w)
{
  struct window_record *r;
  Atom type;
  Window leader;
  gchar *class;
  
  type = gpe_get_window_property (dpy, w, atoms[_NET_WM_WINDOW_TYPE]);
  if (type == atoms[_NET_WM_WINDOW_TYPE_DOCK])
    return;
 
  if (type == atoms[_NET_WM_WINDOW_TYPE_DESKTOP])
    return;
  
  if (type == atoms[_NET_WM_WINDOW_TYPE_TOOLBAR])
    return;
  
  leader = gpe_get_wm_leader (dpy, w);
  if (leader == None)
    leader = w;

  r = g_malloc0 (sizeof (*r));
  r->w = w;
  r->leader = leader;
  r->icon_name = gpe_get_wm_icon_name (dpy, w);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_wm_icon_name (dpy, leader);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_window_name (dpy, leader);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_window_name (dpy, w);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_window_name (dpy, leader);
  if (r->icon_name == NULL)
    r->icon_name = g_strdup ("?");
  r->icon = gpe_get_window_icon (dpy, w);
  if (r->icon == NULL)
    r->icon = other_icon;
  r->class = class;

  windows = g_list_append (windows, r);
}

static gboolean
timeout_func (void *p)
{
  struct timeout_record *d = (struct timeout_record *)p;
  window_added (d->list, d->w);
  g_free (d);
  return FALSE;
}

static void
window_added_callback (GPEWindowList *list, Window w)
{
  struct timeout_record *d;
  d = g_malloc (sizeof (*d));
  d->list = list;
  d->w = w;
  g_timeout_add (250, timeout_func, d);
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
      gchar *name;

      r = l->data;
      g_assert (r != NULL);

      name = gpe_get_window_name (dpy, r->w);
      if (name == NULL && r->leader != r->w)
        name = gpe_get_window_name (dpy, r->leader);  
      if (name == NULL)
        name = g_strdup ("?");

      item = gtk_image_menu_item_new_with_label (name);
      
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

  gtk_widget_show_all (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                  gpe_popup_menu_position,
                  widget, button->button, button->time);
}


/* handle resizing */
static gboolean 
external_event(GtkWindow *window, GdkEventConfigure *event, gpointer user_data)
{
  GdkBitmap *bitmap;
  GdkPixbuf *sbuf, *dbuf;
  gint size;

  if (event->type == GDK_CONFIGURE)
    {
      size = (event->width > event->height) ? event->height : event->width;
      sbuf = gpe_find_icon ("windowlist");
      dbuf = gdk_pixbuf_scale_simple(sbuf,size, size, GDK_INTERP_HYPER);
      gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 250);
      gtk_widget_shape_combine_mask (GTK_WIDGET(window), NULL, 1, 0);
      gtk_widget_shape_combine_mask (GTK_WIDGET(window), bitmap, 1, 0);
      gdk_bitmap_unref (bitmap);
      gtk_image_set_from_pixbuf(GTK_IMAGE(icon), dbuf);
    }
  return FALSE;
}


int 
main (int argc, char **argv)
{
  GtkWidget *window;
  GdkBitmap *bitmap;
  GtkTooltips *tooltips;
  GObject *list;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE) 
    {
      gpe_error_box (_("Failed to load icons"));
      exit (1);
    }
  
  dpy = gdk_x11_get_default_xdisplay ();
  XSelectInput (dpy, DefaultRootWindow (dpy), PropertyChangeMask | StructureNotifyMask);
  atoms = g_malloc0 (sizeof (Atom) * sizeof (atom_names) / sizeof (atom_names[0]));
  XInternAtoms (dpy, atom_names, sizeof (atom_names) / sizeof (atom_names[0]),
                False, atoms);
  
  window = gtk_plug_new (0);
  gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    
  gtk_widget_realize (window);


  gtk_window_set_title (GTK_WINDOW (window), _("Window list"));
  gpe_set_window_icon (GTK_WIDGET (window), "windowlist");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the list of open windows.  You can select a window to activate here."), NULL);

  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("windowlist"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
  gdk_bitmap_unref (bitmap);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("windowlist"));
  gtk_misc_set_alignment(GTK_MISC(icon), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (window), icon);

  other_icon = gpe_find_icon ("windowlist");
  
  gtk_widget_show_all (window);

  gpe_system_tray_dock (window->window);
  
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  list = gpe_window_list_new (gdk_screen_get_default ());

  gpe_launch_monitor_display (dpy);
  add_initial_windows (GPE_WINDOW_LIST (list));

  gpe_launch_install_filter ();
  
  g_signal_connect (G_OBJECT (list), "window-added", G_CALLBACK (window_added_callback), NULL);
  g_signal_connect (G_OBJECT (list), "window-removed", G_CALLBACK (window_removed), NULL);

  g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (external_event), NULL);
  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (popup_window_list), windows);

  gtk_main ();

  exit (0);
}
