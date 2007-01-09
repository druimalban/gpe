/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <sys/dir.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/tray.h>

#include "xsettings-common.h"
#include "xsettings-client.h"

#define _(x) gettext(x)

struct gpe_icon my_icons[] = {
  { "icon", PREFIX "/share/pixmaps/minitheme.png" },
  { NULL }
};

static GtkWidget *icon;

static GtkWidget *menu;

static XSettingsClient *client;
static Display *dpy;
static Window mywindow;

struct _XSettingsClient
{
  Display *display;
  int screen;
  XSettingsNotifyFunc notify;
  XSettingsWatchFunc watch;
  void *cb_data;

  Window manager_window;
  Atom manager_atom;
  Atom selection_atom;
  Atom xsettings_atom;

  XSettingsList *settings;
};

Window
xsettings_client_manager_window (XSettingsClient *client)
{
  return ((struct _XSettingsClient *)client)->manager_window;
}

static void
clicked (GtkWidget *w, GdkEventButton *ev)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, ev->button, ev->time);
}

static void
set_theme (GtkWidget *w, gchar *value)
{
  Atom gpe_settings_update_atom = XInternAtom (dpy, "_GPE_SETTINGS_UPDATE", 0);
  Window win = XGetSelectionOwner (dpy, gpe_settings_update_atom);
  XSettingsType type;
  size_t length, name_len;
  gchar *buffer;
  XClientMessageEvent ev;
  gchar *prop = "/MATCHBOX/THEME";

  if (win == None)
    {
      gpe_error_box (_("Could not set theme.  Is gpe-confd running?"));
      return;
    }

  type = XSETTINGS_TYPE_STRING;
  length = 4 + ((strlen (value) + 3) & ~3);

  name_len = strlen (prop);
  name_len = (name_len + 3) & ~3;
  buffer = g_malloc (length + 4 + name_len);
  *buffer = type;
  buffer[1] = 0;
  buffer[2] = name_len & 0xff;
  buffer[3] = (name_len >> 8) & 0xff;
  memcpy (buffer + 4, prop, name_len);
  
  *((unsigned long *)(buffer + 4 + name_len)) = strlen (value);
  memcpy (buffer + 8 + name_len, value, strlen (value));

  XChangeProperty (dpy, mywindow, gpe_settings_update_atom, gpe_settings_update_atom,
		   8, PropModeReplace, buffer, length + 4 + name_len);
  
  ev.type = ClientMessage;
  ev.window = mywindow;
  ev.message_type = gpe_settings_update_atom;
  ev.format = 32;
  ev.data.l[0] = gpe_settings_update_atom;
  XSendEvent (dpy, win, FALSE, 0, (XEvent *)&ev);
}

static void
read_themes (GtkWidget *menu)
{
  static char *theme_dir = PREFIX "/share/matchbox/themes";
  DIR *dir = opendir (theme_dir);
  if (dir)
    {
      struct dirent *d;
      while (d = readdir (dir), d != NULL)
	{
	  if (d->d_name[0] != '.')
	    {
	      gchar *theme = g_strdup_printf ("%s/%s", theme_dir, d->d_name);
	      GtkWidget *w = gtk_menu_item_new_with_label (d->d_name);
	      gtk_widget_show (w);
	      gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (set_theme), theme);
	      gtk_menu_append (GTK_MENU (menu), w);
	    }
	}
      closedir (dir);
    }
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GdkBitmap *bitmap;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_plug_new (0);
  gtk_widget_set_usize (window, 16, 16);
  gtk_widget_realize (window);

  gtk_window_set_title (GTK_WINDOW (window), _("Theme switcher"));

  menu = gtk_menu_new ();

  read_themes (menu);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("icon"));
  gtk_widget_show (icon);
  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("icon"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 2, 0);
  gdk_bitmap_unref (bitmap);

  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  client = xsettings_client_new (dpy, DefaultScreen (dpy), NULL, NULL, NULL);
  if (client == NULL)
    {
      fprintf (stderr, "Cannot create XSettings client\n");
      exit (1);
    }

  gtk_widget_show (window);

  mywindow = GDK_WINDOW_XWINDOW (window->window);
  gpe_system_tray_dock (window->window);

  gtk_main ();

  exit (0);
}
