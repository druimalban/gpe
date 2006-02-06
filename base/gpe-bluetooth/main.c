/*
 * Copyright (C) 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
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
#include <gpe/gpeiconlistview.h>
#include <gpe/tray.h>
#include <gpe/popup.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"
#include "sdp.h"
#include "dbus.h"
#include "progress.h"

#include "dun.h"
#include "lap.h"
#include "pan.h"
#include "headset.h"

#include "obexserver.h"
#include "obexclient.h"

#define _(x) gettext(x)

#define HCIATTACH "/etc/bluetooth/hciattach"

static GThread *scan_thread;

bdaddr_t src_addr = *BDADDR_ANY;
sdp_session_t *sdp_session;

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

static GtkWidget *window;
static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;
static GtkWidget *devices_window;
static GtkWidget *iconlist;
static GtkWidget *radio_on_progress;

static GSList *devices;

gboolean radio_is_on;
GdkWindow *dock_window;
GSList *service_desc_list;

struct callback_record
{
  GCallback callback;
  gpointer data;
};

typedef void (*radio_start_callback)(gpointer);
static GSList *radio_start_callbacks;
static gboolean radio_starting;
static int radio_use_count;

extern void bluez_pin_dbus_server_run (void);

void
set_image(int sx, int sy)
{
  GdkBitmap *bitmap;
  GdkPixbuf *sbuf, *dbuf;
  int size;
	
  if (!sx)
    {
      sy = gdk_pixbuf_get_height(gtk_image_get_pixbuf(GTK_IMAGE(icon)));
      sx = gdk_pixbuf_get_width(gtk_image_get_pixbuf(GTK_IMAGE(icon)));
    }
	
  size = (sx > sy) ? sy : sx;
  sbuf = gpe_find_icon (radio_is_on ? "bt-on" : "bt-off");
	
  dbuf = gdk_pixbuf_scale_simple(sbuf, size, size, GDK_INTERP_HYPER);
  gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 60);
  gtk_widget_shape_combine_mask (GTK_WIDGET(window), NULL, 0, 0);
  gtk_widget_shape_combine_mask (GTK_WIDGET(window), bitmap, 0, 0);
  gdk_bitmap_unref (bitmap);
  gtk_image_set_from_pixbuf(GTK_IMAGE(icon), dbuf);
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

#define RADIO_ON_POLL_TIME	500

gboolean
check_radio_startup (guint id)
{
  int dd;

  dd = hci_open_dev (0);
  if (dd != -1)
    {
      GSList *l;

      hci_close_dev (dd);

      if (radio_on_progress)
	{
	  gtk_widget_destroy (radio_on_progress);
	  radio_on_progress = NULL;
	}

      radio_is_on = TRUE;
      radio_starting = FALSE;
      set_image (0, 0);

      if (radio_start_callbacks)
	{
	  for (l = radio_start_callbacks; l; l = l->next)
	    {
	      struct callback_record *c = l->data;
	      ((radio_start_callback)c->callback) (c->data);
	    }

	  g_slist_free (radio_start_callbacks);

	  radio_start_callbacks = NULL;
	}

      return FALSE;
    }

  return TRUE;
}

static gboolean
do_start_radio (void)
{
  sigset_t sigs;
  int fd;

  fd = socket (PF_BLUETOOTH, SOCK_DGRAM, BTPROTO_L2CAP);
  if (fd < 0)
    {
      gpe_error_box (_("No kernel support for Bluetooth"));
      return FALSE;
    }
  close (fd);

  gtk_widget_hide (menu_radio_on);
  gtk_widget_show (menu_radio_off);
  gtk_widget_set_sensitive (menu_devices, TRUE);

  sigemptyset (&sigs);
  sigaddset (&sigs, SIGCHLD);
  sigprocmask (SIG_BLOCK, &sigs, NULL);
  hciattach_pid = fork_hciattach ();
  sigprocmask (SIG_UNBLOCK, &sigs, NULL);

  if (hciattach_pid == 0)
    {
      gpe_perror_box (_("Couldn't exec " HCIATTACH));
      return FALSE;
    }

  radio_on_progress = bt_progress_dialog (_("Energising radio"), gpe_find_icon ("bt-logo"));
  gtk_widget_show_all (radio_on_progress);
  radio_starting = TRUE;

  g_timeout_add (RADIO_ON_POLL_TIME, (GSourceFunc) check_radio_startup, NULL);
  return TRUE;
}

static void
do_stop_radio (void)
{
  radio_is_on = FALSE;
  radio_starting = FALSE;

  if (hciattach_pid)
    {
      kill (hciattach_pid, 15);
      hciattach_pid = 0;
    }

  system ("/sbin/hciconfig hci0 down");
  set_image (0, 0);

  gtk_widget_hide (menu_radio_off);
  gtk_widget_show (menu_radio_on);
  gtk_widget_set_sensitive (menu_devices, FALSE);
}

void
radio_on (void)
{
  if (!radio_is_on)
    do_start_radio ();
  radio_use_count++;
}

void
radio_off (void)
{
  radio_use_count--;

  if (radio_use_count == 0)
    do_stop_radio ();
}

gboolean
radio_on_then (GCallback callback, gpointer data)
{
  struct callback_record *c;

  radio_use_count++;

  if (radio_is_on)
    {
      ((radio_start_callback)callback) (data);
      return TRUE;
    }

  c = g_new (struct callback_record, 1);
  c->callback = callback;
  c->data = data;
  radio_start_callbacks = g_slist_prepend (radio_start_callbacks, c);
  if (!radio_starting)
    {
      if (!do_start_radio ())
	{
	  radio_start_callbacks = g_slist_remove (radio_start_callbacks, c);
	  g_free (c);
	  radio_use_count--;
	  return FALSE;
	}
    }

  return TRUE;
}

static void
radio_on_menu (void)
{
  radio_on ();

  gtk_widget_hide (menu_radio_on);
  gtk_widget_show (menu_radio_off);
  gtk_widget_set_sensitive (menu_devices, TRUE);
}

static void
radio_off_menu (void)
{
  radio_off ();

  gtk_widget_hide (menu_radio_off);
  gtk_widget_show (menu_radio_on);
  gtk_widget_set_sensitive (menu_devices, FALSE);
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

const gchar *
icon_name_for_class (int class)
{
  const gchar *pixbuf_name;

  switch (class & 0x1f00)
    {
    case 0x100:
      pixbuf_name = "computer";
      break;
    case 0x200:
      pixbuf_name = "cellphone";
      break;
    case 0x300:
      pixbuf_name = "network";
      break;
    case 0x600:
      pixbuf_name = "printer";
      break;
    default:
      pixbuf_name = "bt-logo";
      break;
    }

  return pixbuf_name;
}

static gboolean
run_scan (gpointer data)
{
  bdaddr_t bdaddr;
  inquiry_info *info = NULL;
  int dev_id = -1;
  int num_rsp, length, flags;
  char name[248];
  int i, dd;
  GtkWidget *w;
  GSList *iter;
  const char *text;

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
      text = _("Inquiry failed");
      goto error;
    }

  dd = hci_open_dev (0/*dev_id*/);
  if (dd < 0) 
    {
      free (info);
      text = _("HCI device open failed");
      goto error;
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

      bd->pixbuf = gpe_find_icon (icon_name_for_class (bd->class));

      gdk_pixbuf_ref (bd->pixbuf);

      devices = g_slist_append (devices, bd);
    }
  
  close (dd);
  free (info);

  gdk_threads_enter ();

  gtk_widget_show_all (devices_window);

  bt_progress_dialog_update (w, _("Retrieving service information"));
  gdk_flush ();

  for (iter = devices; iter; iter = iter->next)
    {
      struct bt_device *bd = iter->data;
      GObject *item;
      item = gpe_icon_list_view_add_item_pixbuf (GPE_ICON_LIST_VIEW (iconlist), bd->name, bd->pixbuf, bd);
      g_signal_connect (G_OBJECT (item), "button-release", G_CALLBACK (device_clicked), bd);
      
      if (bd->sdp == FALSE)
	{
	  gdk_flush ();
	  gdk_threads_leave ();

	  bd->sdp = sdp_browse_device (bd, PUBLIC_BROWSE_GROUP);

	  gdk_threads_enter ();
	}
    }

  gtk_widget_destroy (w);
  gdk_flush ();

  gdk_threads_leave ();

  return TRUE;

 error:
  gdk_threads_enter ();
  gtk_widget_destroy (w);
  gpe_perror_box_nonblocking (text);
  gtk_widget_show_all (devices_window);
  gdk_threads_leave ();
  return FALSE;
}

static void
show_devices (void)
{
  if (devices_window == NULL)
    {
      devices_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_window_set_title (GTK_WINDOW (devices_window), _("Bluetooth devices"));
      gpe_set_window_icon (devices_window, "bt-logo");

      gtk_window_set_default_size (GTK_WINDOW (devices_window), 240, 240);

      iconlist = gpe_icon_list_view_new ();
      gtk_container_add (GTK_CONTAINER (devices_window), iconlist);
      gpe_icon_list_view_set_embolden (GPE_ICON_LIST_VIEW (iconlist), FALSE);

      g_signal_connect (G_OBJECT (devices_window), "destroy", 
			G_CALLBACK (devices_window_destroyed), NULL);
    }

  gpe_icon_list_view_clear (GPE_ICON_LIST_VIEW (iconlist));

  scan_thread = g_thread_create ((GThreadFunc) run_scan, NULL, FALSE, NULL);

  if (scan_thread == NULL)
    gpe_perror_box (_("Unable to scan for devices"));
}

static gboolean sigchld_signalled;

static void 
sigchld_handler (int signo)
{
  sigchld_signalled = TRUE;
}

static gboolean
sigchld_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  int status;
  pid_t p;

  sigchld_signalled = FALSE;
  
  do
    {
      p = waitpid (0, &status, WNOHANG);
      if (p < 0)
	perror ("waitpid");

      if (p == hciattach_pid)
	{
	  if (WIFSIGNALED (status) && (WTERMSIG (status) == SIGHUP))
	    {
	      /* restart hciattach after hangup */
	      hciattach_pid = fork_hciattach ();
	    }
	  else
	    {
	      hciattach_pid = 0;
	      if (radio_is_on)
		{
		  gpe_error_box_nonblocking (_("hciattach died unexpectedly"));
		  radio_off ();
		}
	      else
		{
		  if (radio_on_progress)
		    {
		      gtk_widget_destroy (radio_on_progress);
		      radio_on_progress = NULL;
		    }
		  
		  gpe_perror_box_nonblocking (_("Radio startup failed"));
		  radio_starting = FALSE;
		}
	    }
	}
      else if (p > 0)
	{
	  fprintf (stderr, "unknown pid %d exited\n", p);
	}
    } while (p > 0);

  return TRUE;
}

static gboolean
sigchld_source_prepare (GSource *source, gint *timeout)
{
  *timeout = -1;
  return sigchld_signalled;
}

static gboolean
sigchld_source_check (GSource *source)
{
  return sigchld_signalled;
}

static GSourceFuncs
sigchld_source_funcs = 
  {
    sigchld_source_prepare,
    sigchld_source_check,
    sigchld_source_dispatch,
    NULL
  };

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

gboolean 
configure_event (GtkWidget *window, GdkEventConfigure *event, GdkBitmap *bitmap_)
{
  if (event->type == GDK_CONFIGURE)
    set_image(event->width, event->height);
	
  return FALSE;
}

int
main (int argc, char *argv[])
{
  GdkBitmap *bitmap;
#ifdef REMOVE_FROM_PANEL
  GtkWidget *menu_remove;
#endif
  GtkTooltips *tooltips;
  int dd;
  GSource *sigchld_source;
  bdaddr_t interface;

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

  gpe_bluetooth_init_dbus ();

  window = gtk_plug_new (0);
  gtk_widget_realize (window);

  gtk_window_set_title (GTK_WINDOW (window), _("Bluetooth control"));

  signal (SIGCHLD, sigchld_handler);
  sigchld_source = g_source_new (&sigchld_source_funcs, sizeof (GSource));
  g_source_attach (sigchld_source, NULL);

  menu = gtk_menu_new ();
  menu_radio_on = gtk_menu_item_new_with_label (_("Switch radio on"));
  menu_radio_off = gtk_menu_item_new_with_label (_("Switch radio off"));
  menu_devices = gtk_menu_item_new_with_label (_("Devices..."));

  g_signal_connect (G_OBJECT (menu_radio_on), "activate", G_CALLBACK (radio_on_menu), NULL);
  g_signal_connect (G_OBJECT (menu_radio_off), "activate", G_CALLBACK (radio_off_menu), NULL);
  g_signal_connect (G_OBJECT (menu_devices), "activate", G_CALLBACK (show_devices), NULL);

  if (! radio_is_on)
    {
      gtk_widget_set_sensitive (menu_devices, FALSE);
      gtk_widget_show (menu_radio_on);
    }

  gtk_widget_show (menu_devices);

  gtk_menu_append (GTK_MENU (menu), menu_radio_on);
  gtk_menu_append (GTK_MENU (menu), menu_radio_off);
  gtk_menu_append (GTK_MENU (menu), menu_devices);

#ifdef REMOVE_FROM_PANEL  
  menu_remove = gtk_menu_item_new_with_label (_("Remove from panel"));
  g_signal_connect (G_OBJECT (menu_remove), "activate", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show (menu_remove);
  gtk_menu_append (GTK_MENU (menu), menu_remove);
#endif

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon (radio_is_on ? "bt-on" : "bt-off"));
  gtk_widget_show (icon);

  gpe_set_window_icon (window, "bt-on");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the Bluetooth control.\nTap here to turn the radio on and off, or to see a list of Bluetooth devices."), NULL);

  g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (configure_event), bitmap);
  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  gtk_widget_show (window);

  atexit (do_stop_radio);

  sdp_session = sdp_connect (&interface, BDADDR_LOCAL, 0);

  dun_init ();
  lap_init ();
  pan_init ();
  headset_init ();
  obex_init ();
  obex_client_init ();

  dock_window = window->window;
  gpe_system_tray_dock (window->window);

  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();

  exit (0);
}
