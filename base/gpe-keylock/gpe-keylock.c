/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <locale.h>
#include <libintl.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

GtkWidget *w;
GtkWidget *info_w;

#define _(x) gettext(x)

int keys_down;

int left_key, right_key;

gboolean window_visible;

#define DELAY 2500

GdkWindow *grab_window;

gboolean
hide_window (gpointer data)
{
  window_visible = FALSE;

  gtk_widget_hide (w);

  return FALSE;
}

void
show_window (void)
{
  window_visible = TRUE;

  gtk_widget_show_all (w);

  g_timeout_add (DELAY, hide_window, NULL);
}

gboolean
hide_info_window (gpointer data)
{
  gtk_main_quit ();

  return FALSE;
}

void
create_info_widgets (GtkWidget *parent)
{
  GtkWidget *lock;
  GtkWidget *hbox;
  GtkWidget *label;

  lock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);

  hbox = gtk_vbox_new (FALSE, 4);

  label = gtk_label_new ("");

  gtk_label_set_markup (GTK_LABEL (label), _("Screen unlocked"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gtk_box_pack_start (GTK_BOX (hbox), lock, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (parent), 4);

  gtk_container_add (GTK_CONTAINER (parent), hbox);
}

void
do_unlock (void)
{
  hide_window (NULL);

  gdk_window_hide (grab_window);

  info_w = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_window_set_position (GTK_WINDOW (info_w), GTK_WIN_POS_CENTER_ALWAYS);

  create_info_widgets (info_w);

  gtk_widget_show_all (info_w);

  g_timeout_add (DELAY, hide_info_window, NULL);
}

GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;

  if (xev->type == KeyPress || xev->type == KeyRelease)
    {
      int key = 0;
      XKeyEvent *xkev = (XKeyEvent *)xev;

      if (! window_visible)
	show_window ();

      if (xkev->keycode == left_key)
	key = 1;
      else if (xkev->keycode == right_key)
	key = 2;

      if (key)
	{
	  if (xev->type == KeyPress)
	    {
	      keys_down |= key;
	      if (keys_down == 3)
		do_unlock ();
	    }
	  else
	    keys_down &= ~key;
	}
    }
  else if (xev->type == ButtonPress)
    {
      if (! window_visible)
	show_window ();
    }

  return GDK_FILTER_CONTINUE;
}

void
create_widgets (GtkWidget *parent)
{
  GtkWidget *lock;
  GtkWidget *hbox;
  GtkWidget *label;

  lock = gtk_image_new_from_file (PREFIX "/share/pixmaps/gpe-keylock.png");

  hbox = gtk_vbox_new (FALSE, 4);

  label = gtk_label_new ("");

  gtk_label_set_markup (GTK_LABEL (label), _("To unlock the screen, press the leftmost and rightmost buttons together.\n"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gtk_box_pack_start (GTK_BOX (hbox), lock, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (parent), 4);

  gtk_container_add (GTK_CONTAINER (parent), hbox);
}

GdkWindow *
create_grab_window (void)
{
  GdkWindowAttr attr;

  attr.wclass = GDK_INPUT_ONLY;
  attr.window_type = GDK_WINDOW_TEMP;
  attr.width = 1;
  attr.height = 1;
  attr.override_redirect = TRUE;

  return gdk_window_new (NULL, &attr, GDK_WA_NOREDIR);
}

int
main (int argc, char *argv[])
{
  Display *dpy;

  gtk_init (&argc, &argv);

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");  

  w = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_window_set_position (GTK_WINDOW (w), GTK_WIN_POS_CENTER_ALWAYS);

  create_widgets (w);

  gtk_widget_realize (w);

  grab_window = create_grab_window ();

  dpy = GDK_WINDOW_XDISPLAY (w->window);

  left_key = XKeysymToKeycode (dpy, XF86XK_Calendar);
  right_key = XKeysymToKeycode (dpy, XF86XK_Start);

  show_window ();
  gdk_window_show (grab_window);

  gdk_window_add_filter (grab_window, filter, 0);

  gdk_window_set_events (grab_window, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gdk_keyboard_grab (grab_window, TRUE, GDK_CURRENT_TIME);
  gdk_pointer_grab (grab_window, TRUE, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK, NULL, NULL, GDK_CURRENT_TIME);

  gtk_main ();

  exit (EXIT_SUCCESS);
}

