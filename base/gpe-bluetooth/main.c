/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
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
#include <errno.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/gpe-iconlist.h>
#include <gpe/tray.h>
#include <gpe/popup.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>

#include "main.h"
#include "sdp.h"

#include "dun.h"
#include "lap.h"
#include "pan.h"

#define _(x) gettext(x)

#define HCIATTACH "/etc/bluetooth/hciattach"

static GThread *scan_thread;

bdaddr_t src_addr = *BDADDR_ANY;

struct gpe_icon my_icons[] = {
  { "bt-on", "bluetooth/bt-on" },
  { "bt-off", "bluetooth/bt-off" },
  { "cellphone", "bluetooth/cellphone" },
  { "network", "bluetooth/network" },
  { "computer", "bluetooth/Computer" },
  { "printer", "bluetooth/Printer" },
  { "bt-logo" },
  { NULL }
};

static GtkWidget *icon;

static pid_t hciattach_pid;

static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;
static GtkWidget *devices_window;
static GtkWidget *iconlist;

static GSList *devices;

gboolean radio_is_on;
GdkWindow *dock_window;
GSList *service_desc_list;

extern void bluez_pin_dbus_server_run (void);

GtkWidget *
bt_progress_dialog (gchar *text, GdkPixbuf *pixbuf)
{
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *hbox;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  hbox = gtk_hbox_new (FALSE, 0);
  image = gtk_image_new_from_pixbuf (pixbuf);
  label = gtk_label_new (text);

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
  
  gtk_container_set_border_width (GTK_CONTAINER (hbox),
					gpe_get_border ());
	
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  return window;
}

static pid_t
fork_hciattach (void)
{
  if (access (HCIATTACH, X_OK) == 0)
    {
      pid_t p = vfork ();
      if (p == 0)
	{
	  execl (HCIATTACH, HCIATTACH, NULL);
	  perror (HCIATTACH);
	  _exit (1);
	}

      return p;
    }

  return 0;
}

static void
radio_on (void)
{
  sigset_t sigs;
  int fd;

  fd = socket (PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP);
  if (fd < 0)
    {
      gpe_error_box (_("No kernel support for Bluetooth"));
      return;
    }
  close (fd);

  gtk_widget_hide (menu_radio_on);
  gtk_widget_show (menu_radio_off);
  gtk_widget_set_sensitive (menu_devices, TRUE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-on"));
  radio_is_on = TRUE;
  sigemptyset (&sigs);
  sigaddset (&sigs, SIGCHLD);
  sigprocmask (SIG_BLOCK, &sigs, NULL);
  hciattach_pid = fork_hciattach ();
  sigprocmask (SIG_UNBLOCK, &sigs, NULL);
}

static void
do_stop_radio (void)
{
  radio_is_on = FALSE;

  if (hciattach_pid)
    {
      kill (hciattach_pid, 15);
      hciattach_pid = 0;
    }

  system ("hciconfig hci0 down");
}

static void
radio_off (void)
{
  gtk_widget_hide (menu_radio_off);
  gtk_widget_show (menu_radio_on);
  gtk_widget_set_sensitive (menu_devices, FALSE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-off"));

  do_stop_radio ();
}

static gboolean
devices_window_destroyed (void)
{
  devices_window = NULL;

  return FALSE;
}

static void
device_info (struct bt_device *bd)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *vbox1 = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox1 = gtk_hbox_new (FALSE, 0);
  GtkWidget *labelname = gtk_label_new (bd->name);
  GtkWidget *labeladdr = gtk_label_new (batostr (&bd->bdaddr));
  GtkWidget *image = gtk_image_new_from_pixbuf (bd->pixbuf);
  GtkWidget *dismiss = gtk_button_new_from_stock (GTK_STOCK_OK);

  gtk_window_set_title (GTK_WINDOW (window), _("Device information"));
  gpe_set_window_icon (GTK_WIDGET (window), "bt-logo");

  gtk_misc_set_alignment (GTK_MISC (labelname), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (labeladdr), 0.0, 0.5);
      
  gtk_box_pack_start (GTK_BOX (vbox1), labelname, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), labeladdr, TRUE, TRUE, 0);
      
  gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 8);
  gtk_box_pack_start (GTK_BOX (hbox1), image, TRUE, TRUE, 8);
      
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox1, FALSE, FALSE, 0);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (window)->action_area), dismiss, FALSE, FALSE, 0);
  
  gtk_widget_realize (window);
  gdk_window_set_transient_for (window->window, devices_window->window);
  
  gtk_widget_show_all (window);
  
  g_signal_connect_swapped (G_OBJECT (dismiss), "clicked", G_CALLBACK (gtk_widget_destroy), window);
}

static void
show_device_info (GtkWidget *w, struct bt_device *this_device)
{
  device_info (this_device);
}

static void
device_clicked (GtkWidget *widget, GdkEventButton *e, gpointer data)
{
  GSList *iter;
  GtkWidget *device_menu;
  GtkWidget *details;
  struct bt_device *bd = (struct bt_device *)data;

  device_menu = gtk_menu_new ();

  details = gtk_menu_item_new_with_label (_("Details ..."));
  g_signal_connect (G_OBJECT (details), "activate", G_CALLBACK (show_device_info), bd);
  gtk_widget_show (details);
  gtk_menu_append (GTK_MENU (device_menu), details);

  for (iter = bd->services; iter; iter = iter->next)
    {
      struct bt_service *sv = iter->data;

      if (sv->desc->popup_menu)
	sv->desc->popup_menu (sv, device_menu);
    }

#if 0
  g_signal_connect (G_OBJECT (device_menu), "hide", G_CALLBACK (gtk_widget_destroy), NULL);
#endif

  gtk_menu_popup (GTK_MENU (device_menu), NULL, NULL, NULL, widget, 1, GDK_CURRENT_TIME);
}

static gboolean
run_scan (void)
{
  bdaddr_t bdaddr;
  inquiry_info *info = NULL;
  int dev_id = -1;
  int num_rsp, length, flags;
  char name[248];
  int i, dd;
  GtkWidget *w;
  GSList *iter;

  gdk_threads_enter ();
  w = bt_progress_dialog (_("Scanning for devices"), gpe_find_icon ("bt-logo"));
  gtk_widget_show_all (w);
  gdk_threads_leave ();

  length  = 4;  /* ~10 seconds */
  num_rsp = 10;
  flags = 0;

  num_rsp = hci_inquiry (dev_id, length, num_rsp, NULL, &info, flags);
  if (num_rsp < 0) 
    {
      gdk_threads_enter ();
      gtk_widget_destroy (w);
      gpe_perror_box_nonblocking (_("Inquiry failed"));
      gtk_widget_show_all (devices_window);
      gdk_threads_leave ();
      close (dev_id);
      return FALSE;
    }

  dd = hci_open_dev(0/*dev_id*/);
  if (dd < 0) 
    {
      gdk_threads_enter ();
      gtk_widget_destroy (w);
      gpe_perror_box_nonblocking (_("HCI device open failed"));
      gtk_widget_show_all (devices_window);
      gdk_threads_leave ();
      close (dev_id);
      free(info);
      return FALSE;
    }

  for (i = 0; i < num_rsp; i++) 
    {
      struct bt_device *bd;
      GSList *iter;
      gboolean found = FALSE;

      baswap (&bdaddr, &(info+i)->bdaddr);

      for (iter = devices; iter; iter = iter->next)
	{
	  struct bt_device *d = (struct bt_device *)iter->data;
	  if (memcmp (&d->bdaddr, &bdaddr, sizeof (bdaddr)) == 0)
	    {
	      found = TRUE;
	      break;
	    }
	}

      if (found)
	continue;

      memset(name, 0, sizeof(name));
      if (hci_read_remote_name (dd, &(info+i)->bdaddr, sizeof(name), name, 25000) < 0)
	strcpy (name, _("unknown"));

      bd = g_malloc (sizeof (struct bt_device));
      memset (bd, 0, sizeof (*bd));
      bd->name = g_strdup (name);
      memcpy (&bd->bdaddr, &bdaddr, sizeof (bdaddr));
      bd->class = ((info+i)->dev_class[2] << 16) | ((info+i)->dev_class[1] << 8) | (info+i)->dev_class[0];

      switch (bd->class & 0x1f00)
	{
	case 0x100:
	  bd->pixbuf = gpe_find_icon ("computer");
	  break;
	case 0x200:
	  bd->pixbuf = gpe_find_icon ("cellphone");
	  break;
	case 0x300:
	  bd->pixbuf = gpe_find_icon ("network");
	  break;
	case 0x600:
	  bd->pixbuf = gpe_find_icon ("printer");
	  break;
	default:
	  bd->pixbuf = gpe_find_icon ("bt-logo");
	  break;
	}

      gdk_pixbuf_ref (bd->pixbuf);

      devices = g_slist_append (devices, bd);
    }
  
  close (dd);
  close (dev_id);

  gdk_threads_enter ();

  for (iter = devices; iter; iter = iter->next)
    {
      struct bt_device *bd = iter->data;
      GObject *item;
      item = gpe_iconlist_add_item_pixbuf (GPE_ICONLIST (iconlist), bd->name, bd->pixbuf, bd);
      g_signal_connect (G_OBJECT (item), "button-release", G_CALLBACK (device_clicked), bd);
      
      if (bd->sdp == FALSE)
	{
	  bd->sdp = TRUE;
	  sdp_browse_device (bd, PUBLIC_BROWSE_GROUP);
	}
    }

  gtk_widget_destroy (w);
  gtk_widget_show_all (devices_window);

  gdk_threads_leave ();

  return TRUE;
}

static void
show_devices (void)
{
  if (devices_window == NULL)
    {
      devices_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_window_set_title (GTK_WINDOW (devices_window), _("Bluetooth devices"));
      gpe_set_window_icon (devices_window, "bt-logo");

      iconlist = gpe_iconlist_new ();
      gtk_container_add (GTK_CONTAINER (devices_window), iconlist);
      gpe_iconlist_set_embolden (GPE_ICONLIST (iconlist), FALSE);

      g_signal_connect (G_OBJECT (devices_window), "destroy", 
			G_CALLBACK (devices_window_destroyed), NULL);
    }

  gpe_iconlist_clear (GPE_ICONLIST (iconlist));

  scan_thread = g_thread_create ((GThreadFunc) run_scan, NULL, FALSE, NULL);

  if (scan_thread == NULL)
    gpe_perror_box (_("Unable to scan for devices"));
}

static void
sigchld_handler (int sig)
{
  int status;
  pid_t p = waitpid (0, &status, WNOHANG);

  if (p == hciattach_pid)
    {
      hciattach_pid = 0;
      if (radio_is_on)
	{
	  gpe_error_box_nonblocking (_("hciattach died unexpectedly"));
	  radio_off ();
	}
    }
  else if (p > 0)
    {
      fprintf (stderr, "unknown pid %d exited\n", p);
    }
}

static void
clicked (GtkWidget *w, GdkEventButton *ev)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gpe_popup_menu_position, w, ev->button, ev->time);
}

static void
cancel_dock_message (guint id)
{
  gdk_threads_enter ();
  gpe_system_tray_cancel_message (dock_window, id);
  gdk_threads_leave ();
}

void
schedule_message_delete (guint id, guint time)
{
  g_timeout_add (time, (GSourceFunc) cancel_dock_message, (gpointer)id);
}

int
main (int argc, char *argv[])
{
  Display *dpy;
  GtkWidget *window;
  GdkBitmap *bitmap;
  GtkWidget *menu_remove;
  GtkTooltips *tooltips;
  int dd;

  g_thread_init (NULL);
  gdk_threads_init ();

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  dd = hci_open_dev (0);
  if (dd != -1)
    {
      radio_is_on = TRUE;
      hci_close_dev (dd);
    }

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  bluez_pin_dbus_server_run ();

  window = gtk_plug_new (0);
  gtk_widget_set_usize (window, 16, 16);
  gtk_widget_realize (window);

  gtk_window_set_title (GTK_WINDOW (window), _("Bluetooth control"));

  signal (SIGCHLD, sigchld_handler);

  menu = gtk_menu_new ();
  menu_radio_on = gtk_menu_item_new_with_label (_("Switch radio on"));
  menu_radio_off = gtk_menu_item_new_with_label (_("Switch radio off"));
  menu_devices = gtk_menu_item_new_with_label (_("Devices..."));
  menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));

  g_signal_connect (G_OBJECT (menu_radio_on), "activate", G_CALLBACK (radio_on), NULL);
  g_signal_connect (G_OBJECT (menu_radio_off), "activate", G_CALLBACK (radio_off), NULL);
  g_signal_connect (G_OBJECT (menu_devices), "activate", G_CALLBACK (show_devices), NULL);
  g_signal_connect (G_OBJECT (menu_remove), "activate", G_CALLBACK (gtk_main_quit), NULL);

  if (! radio_is_on)
    {
      gtk_widget_set_sensitive (menu_devices, FALSE);
      gtk_widget_show (menu_radio_on);
    }

  gtk_widget_show (menu_devices);
  gtk_widget_show (menu_remove);

  gtk_menu_append (GTK_MENU (menu), menu_radio_on);
  gtk_menu_append (GTK_MENU (menu), menu_radio_off);
  gtk_menu_append (GTK_MENU (menu), menu_devices);
  gtk_menu_append (GTK_MENU (menu), menu_remove);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon (radio_is_on ? "bt-on" : "bt-off"));
  gtk_widget_show (icon);
  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("bt-off"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 2, 2);
  gdk_bitmap_unref (bitmap);

  gpe_set_window_icon (window, "bt-on");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the Bluetooth control.\nTap here to turn the radio on and off, or to see a list of Bluetooth devices."), NULL);

  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  gtk_widget_show (window);

  atexit (do_stop_radio);

  dun_init ();
  lap_init ();
  pan_init ();

  dock_window = window->window;
  gpe_system_tray_dock (window->window);

  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();

  exit (0);
}
