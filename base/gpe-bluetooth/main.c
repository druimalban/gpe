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
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include "tray.h"

#define _(x) gettext(x)

struct gpe_icon my_icons[] = {
  { "bt-on" },
  { "bt-off" },
  { NULL }
};

static GtkWidget *icon;
static gchar *hciattach_string;

static pid_t hcid_pid;
static pid_t hciattach_pid;

static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;

static gboolean radio_is_on;

static pid_t
fork_hcid (void)
{
  pid_t p = vfork ();
  if (p == 0)
    {
      execlp ("hcid", "hcid", "-n");
      perror ("hcid");
      _exit (1);
    }

  return p;
}

static pid_t
fork_hciattach (char *str)
{
}

static void
radio_on (void)
{
  sigset_t sigs;

  gtk_widget_hide (menu_radio_on);
  gtk_widget_show (menu_radio_off);
  gtk_widget_set_sensitive (menu_devices, TRUE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-on"));
  radio_is_on = TRUE;
  sigemptyset (&sigs);
  sigaddset (&sigs, SIGCHLD);
  sigprocmask (SIG_BLOCK, &sigs, NULL);
  hcid_pid = fork_hcid ();
  if (hciattach_string)
    hciattach_pid = fork_hciattach (hciattach_string);
  sigprocmask (SIG_UNBLOCK, &sigs, NULL);
}

static void
radio_off (void)
{
  gtk_widget_hide (menu_radio_off);
  gtk_widget_show (menu_radio_on);
  gtk_widget_set_sensitive (menu_devices, FALSE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-off"));
  radio_is_on = FALSE;
  if (hcid_pid)
    {
      kill (hcid_pid, 15);
      hcid_pid = 0;
    }
  if (hciattach_pid)
    {
      kill (hciattach_pid, 15);
      hciattach_pid = 0;
    }
  system ("hciconfig hci0 down");
}

static void
sigchld_handler (int sig)
{
  int status;
  pid_t p = wait (&status);

  if (p == hcid_pid)
    {
      hcid_pid = 0;
      gpe_error_box (_("hcid died unexpectedly"));
      radio_off ();
    }
  else if (p == hciattach_pid)
    {
      hciattach_pid = 0;
      gpe_error_box (_("hciattach died unexpectedly"));
      radio_off ();
    }
  else if (p != -1)
    {
      fprintf (stderr, "unknown pid %d exited\n", p);
    }
}

static void
clicked (GtkWidget *w, GdkEventButton *ev)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, ev->button, ev->time);
}

static GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  
  if (xev->type == ClientMessage || xev->type == ReparentNotify)
    {
      XAnyEvent *any = (XAnyEvent *)xev;
      tray_handle_event (any->display, any->window, xev);
      if (xev->type == ReparentNotify)
	gtk_widget_show (GTK_WIDGET (p));
    }
  return GDK_FILTER_CONTINUE;
}

int
main (int argc, char *argv[])
{
  Display *dpy;
  GtkWidget *window;
  GdkBitmap *bitmap;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window, 16, 16);
  gtk_widget_realize (window);

  signal (SIGCHLD, sigchld_handler);

  menu = gtk_menu_new ();
  menu_radio_on = gtk_menu_item_new_with_label (_("Switch radio on"));
  menu_radio_off = gtk_menu_item_new_with_label (_("Switch radio off"));
  menu_devices = gtk_menu_item_new_with_label (_("Devices..."));

  gtk_signal_connect (GTK_OBJECT (menu_radio_on), "activate", GTK_SIGNAL_FUNC (radio_on), NULL);
  gtk_signal_connect (GTK_OBJECT (menu_radio_off), "activate", GTK_SIGNAL_FUNC (radio_off), NULL);
  gtk_signal_connect (GTK_OBJECT (menu_radio_off), "activate", GTK_SIGNAL_FUNC (show_devices), NULL);

  gtk_widget_set_sensitive (menu_devices, FALSE);

  gtk_widget_show (menu_radio_on);
  gtk_widget_show (menu_devices);

  gtk_menu_append (GTK_MENU (menu), menu_radio_on);
  gtk_menu_append (GTK_MENU (menu), menu_radio_off);
  gtk_menu_append (GTK_MENU (menu), menu_devices);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new ();
  gtk_widget_show (icon);
  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-off"));
  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("bt-off"), NULL, &bitmap, 127);
  gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
  gdk_bitmap_unref (bitmap);

  gtk_signal_connect (GTK_OBJECT (window), "button-press-event", GTK_SIGNAL_FUNC (clicked), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  tray_init (dpy, GDK_WINDOW_XWINDOW (window->window));
  gdk_window_add_filter (window->window, filter, window);

  gtk_main ();

  exit (0);
}
