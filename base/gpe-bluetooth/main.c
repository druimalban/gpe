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
#include <pthread.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/gpe-iconlist.h>
#include <gpe/tray.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#define _(x) gettext(x)

#define HCIATTACH "/etc/bluetooth/hciattach"

static pthread_t scan_thread;
static gboolean scan_complete;

struct gpe_icon my_icons[] = {
  { "bt-on", "bluetooth/bt-on" },
  { "bt-off", "bluetooth/bt-off" },
  { "cellphone", "bluetooth/cellphone" },
  { "network", "bluetooth/network" },
  { "bt-logo" },
  { NULL }
};

typedef enum
  {
    BT_UNKNOWN,
    BT_LAP,
    BT_DUN
  } bt_device_type;

struct bt_device
{
  gchar *name;
  guint class;
  bdaddr_t bdaddr;
  GdkPixbuf *pixbuf;
  bt_device_type type;
  guint port;
  pid_t pid;
  GtkWidget *window, *button;
};

static GtkWidget *icon;

static pid_t hcid_pid;
static pid_t hciattach_pid;

static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;
static GtkWidget *devices_window;
static GtkWidget *iconlist;

static gboolean radio_is_on;

static GSList *devices;

static gboolean
do_run_scan (void)
{
  bdaddr_t bdaddr;
  inquiry_info *info = NULL;
  int dev_id = hci_get_route(&bdaddr);
  int num_rsp, length, flags;
  char name[248];
  int i, dd;

  if (dev_id < 0) 
    {
      gpe_perror_box ("Device is not available");
      return FALSE;
    }
  
  length  = 4;  /* ~10 seconds */
  num_rsp = 10;
  flags = 0;

  num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);
  if (num_rsp < 0) 
    {
      gpe_perror_box ("Inquiry failed.");
      close (dev_id);
      return FALSE;
    }

  dd = hci_open_dev(dev_id);
  if (dd < 0) 
    {
      gpe_perror_box ("HCI device open failed");
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
      if (hci_read_remote_name (dd, &(info+i)->bdaddr, sizeof(name), name, 100000) < 0)
	strcpy (name, _("unknown"));

      bd = g_malloc (sizeof (struct bt_device));
      memset (bd, 0, sizeof (*bd));
      bd->name = g_strdup (name);
      memcpy (&bd->bdaddr, &bdaddr, sizeof (bdaddr));
      bd->class = ((info+i)->dev_class[2] << 16) | ((info+i)->dev_class[1] << 8) | (info+i)->dev_class[0];

      switch (bd->class & 0x1f00)
	{
	case 0x200:
	  bd->pixbuf = gpe_find_icon ("cellphone");
	  break;
	case 0x300:
	  bd->pixbuf = gpe_find_icon ("network");
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

  return TRUE;
}

static int
run_scan (void *data)
{
  do_run_scan ();

  scan_complete = TRUE;

  return 0;
}

static void
stop_dun (struct bt_device *bd)
{
  pid_t p = vfork ();
  char bdaddr[40];

  strcpy (bdaddr, batostr (&bd->bdaddr));

  if (p == 0)
    {
      execlp ("dund", "dund", "-k", bdaddr, NULL);
      _exit (1);
    }
}

static pid_t
fork_hcid (void)
{
  pid_t p = vfork ();
  if (p == 0)
    {
      execlp ("hcid", "hcid", "-n", NULL);
      perror ("hcid");
      _exit (1);
    }

  return p;
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

  gtk_widget_hide (menu_radio_on);
  gtk_widget_show (menu_radio_off);
  gtk_widget_set_sensitive (menu_devices, TRUE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-on"));
  radio_is_on = TRUE;
  sigemptyset (&sigs);
  sigaddset (&sigs, SIGCHLD);
  sigprocmask (SIG_BLOCK, &sigs, NULL);
  hcid_pid = fork_hcid ();
  hciattach_pid = fork_hciattach ();
  sigprocmask (SIG_UNBLOCK, &sigs, NULL);
}

static void
do_stop_radio (void)
{
  GSList *iter;

  radio_is_on = FALSE;
  for (iter = devices; iter; iter = iter->next)
    {
      struct bt_device *bd = (struct bt_device *)iter->data;
      if (bd->pid)
	{
	  stop_dun (bd);
	}
    }
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

static gboolean
browse_device (struct bt_device *bd)
{
  GSList *attrid, *search;
  GSList *pSeq = NULL;
  uint32_t range = 0x0000ffff;
  uint16_t count = 0;
  int status = -1, i;
  uuid_t group;
  bdaddr_t bdaddr;
  uuid_t lap_uuid, dun_uuid, rfcomm_uuid;

  baswap (&bdaddr, &bd->bdaddr);

  sdp_create_uuid16 (&group, PUBLIC_BROWSE_GROUP);
  sdp_create_uuid16 (&lap_uuid, LAN_ACCESS_SVCLASS_ID);
  sdp_create_uuid16 (&dun_uuid, DIALUP_NET_SVCLASS_ID);
  sdp_create_uuid16 (&rfcomm_uuid, RFCOMM_UUID);

  attrid = g_slist_append (NULL, &range);
  search = g_slist_append (NULL, &group);
  status = sdp_service_search_attr_req (&bdaddr, search, SDP_ATTR_REQ_RANGE,
					attrid, 65535, &pSeq, &count);

  if (status) 
    return FALSE;

  bd->type = BT_UNKNOWN;

  for (i = 0; i < (int) g_slist_length (pSeq); i++) 
    {
      uint32_t svcrec;
      GSList *pGSList;
      svcrec = *(uint32_t *) g_slist_nth_data(pSeq, i);

      if (sdp_get_svclass_id_list (&bdaddr, svcrec, &pGSList) == 0) 
	{
	  GSList *iter;
	  
	  for (iter = pGSList; iter; iter = iter->next)
	    {
	      uuid_t *u = iter->data;
	      if (sdp_uuid_cmp (u, &lap_uuid) == 0)
		bd->type = BT_LAP;
	      else if (sdp_uuid_cmp (u, &dun_uuid) == 0)
		bd->type = BT_DUN;
	    }
	}

      if (bd->type != BT_UNKNOWN)
	{
	  sdp_access_proto_t *access_proto;
	  if (sdp_get_access_proto (&bdaddr, svcrec, &access_proto) == 0) 
	    {
	      GSList *iter;

	      for (iter = access_proto->seq; iter; iter = iter->next)
		{
		  GSList *list;

		  for (list = iter->data; list; list = list->next)
		    {
		      sdp_proto_desc_t *desc = list->data;
		      if (sdp_uuid_cmp (&desc->uuid, &rfcomm_uuid) == 0)
			bd->port = desc->port;
		    }
		}
	    }
	}
    }
  
  return TRUE;
}

static void
window_destroyed (GtkWidget *window, gpointer data)
{
  struct bt_device *bd = (struct bt_device *)data;

  bd->window = NULL;
  bd->button = NULL;
}

static void
button_clicked (GtkWidget *window, gpointer data)
{
  struct bt_device *bd = (struct bt_device *)data;

  if (bd->pid)
    {
      stop_dun (bd);
      bd->pid = 0;
    }
  else
    {
      /* connect */
      char port[16];
      char address[64];
      pid_t p;

      if (! radio_is_on)
	{
	  gpe_error_box (_("Radio is switched off"));
	  return;
	}
      
      strcpy (address, batostr (&bd->bdaddr));
      sprintf (port, "%d", bd->port);

      p = vfork ();

      if (p == 0)
	{
	  execlp ("dund", "dund", "-n", "-C", port, "-c", address, NULL);
	  _exit (1);
	}

      bd->pid = p;
    }

  if (bd->button)
    {
      GtkWidget *label = gtk_bin_get_child (GTK_BIN (bd->button));
      gtk_label_set_text (GTK_LABEL (label), bd->pid ? _("Disconnect") : _("Connect"));
    }
}

static void
device_clicked (GtkWidget *w, gpointer data)
{
  struct bt_device *bd = (struct bt_device *)data;

  if (bd->window == NULL)
    {
      GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      GtkWidget *vbox1 = gtk_vbox_new (FALSE, 0);
      GtkWidget *hbox1 = gtk_hbox_new (FALSE, 0);
      GtkWidget *labelname = gtk_label_new (bd->name);
      GtkWidget *labeladdr = gtk_label_new (batostr (&bd->bdaddr));
      GtkWidget *image = gtk_image_new_from_pixbuf (bd->pixbuf);
      GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);

      gtk_widget_show (vbox1);
      gtk_widget_show (hbox1);
      gtk_widget_show (vbox2);
      gtk_widget_show (labelname);
      gtk_widget_show (labeladdr);
      gtk_widget_show (image);

      gtk_misc_set_alignment (GTK_MISC (labelname), 0.0, 0.5);
      gtk_misc_set_alignment (GTK_MISC (labeladdr), 0.0, 0.5);
      
      gtk_box_pack_start (GTK_BOX (vbox1), labelname, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (vbox1), labeladdr, TRUE, TRUE, 0);
      
      gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 8);
      gtk_box_pack_start (GTK_BOX (hbox1), image, TRUE, TRUE, 8);
      
      gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 0);

      if (bd->type == 0)
	browse_device (bd);

      if (bd->type != BT_UNKNOWN)
	{
	  GtkWidget *label;
	  char str[80];
	  
	  strcpy (str, _("Service class: "));
	  switch (bd->type)
	    {
	    case BT_DUN:
	      strcat (str, _("Dial-Up Network"));
	      break;
	    case BT_LAP:
	      strcat (str, _("LAN Access"));
	      break;
	    default:
	      strcat (str, _("unknown"));
	      break;
	    }
	  
	  label = gtk_label_new (str);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_widget_show (label);
	  gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 4);
	}
      
      if (bd->type == BT_LAP)
	{
	  GtkWidget *button = gtk_button_new_with_label (bd->pid ? _("Disconnect") : _("Connect"));
	  gtk_widget_show (button);
	  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 4);
	  bd->button = button;
	  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (button_clicked), bd);
	}
      
      gtk_container_add (GTK_CONTAINER (window), vbox2);
      
      gtk_widget_realize (window);
      gdk_window_set_transient_for (window->window, devices_window->window);
      
      gtk_widget_show (window);
      
      bd->window = window;
      
      g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (window_destroyed), bd);
    }
}

static gboolean
check_scan_complete (void)
{
  if (scan_complete)
    {
      GSList *iter;

      pthread_join (scan_thread, NULL);
      
      for (iter = devices; iter; iter = iter->next)
	{
	  struct bt_device *bd = iter->data;
	  gpe_iconlist_add_item_pixbuf (GPE_ICONLIST (iconlist), bd->name, bd->pixbuf, bd);
	}

      return FALSE;
    }

  return TRUE;
}

static void
show_devices (void)
{
  GSList *iter;

  if (devices_window == NULL)
    {
      devices_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_window_set_title (GTK_WINDOW (devices_window), _("Bluetooth devices"));
      gpe_set_window_icon (devices_window, "bt-logo");

      iconlist = gpe_iconlist_new ();
      gtk_widget_show (iconlist);
      gtk_container_add (GTK_CONTAINER (devices_window), iconlist);
      gpe_iconlist_set_embolden (GPE_ICONLIST (iconlist), FALSE);

      g_signal_connect (G_OBJECT (iconlist), "clicked", 
			G_CALLBACK (device_clicked), NULL);
      
      g_signal_connect (G_OBJECT (devices_window), "destroy", 
			G_CALLBACK (devices_window_destroyed), NULL);
    }

  gpe_iconlist_clear (GPE_ICONLIST (iconlist));
  gtk_widget_show (devices_window);

  scan_complete = FALSE;
  if (pthread_create (&scan_thread, 0, run_scan, NULL))
    gpe_perror_box (_("Unable to scan"));
  else
    gtk_timeout_add (100, G_CALLBACK (check_scan_complete), NULL);
}

static void
sigchld_handler (int sig)
{
  int status;
  pid_t p = waitpid (0, &status, WNOHANG);

  if (p == hcid_pid)
    {
      hcid_pid = 0;
      if (radio_is_on)
	{
	  gpe_error_box_nonblocking (_("hcid died unexpectedly"));
	  radio_off ();
	}
    }
  else if (p == hciattach_pid)
    {
      hciattach_pid = 0;
      if (radio_is_on)
	{
	  gpe_error_box_nonblocking (_("hciattach died unexpectedly"));
	  radio_off ();
	}
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

int
main (int argc, char *argv[])
{
  Display *dpy;
  GtkWidget *window;
  GdkBitmap *bitmap;
  GtkWidget *menu_remove;
  GtkWidget *tooltips;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  sdp_init ();

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

  gtk_widget_set_sensitive (menu_devices, FALSE);

  gtk_widget_show (menu_radio_on);
  gtk_widget_show (menu_devices);
  gtk_widget_show (menu_remove);

  gtk_menu_append (GTK_MENU (menu), menu_radio_on);
  gtk_menu_append (GTK_MENU (menu), menu_radio_off);
  gtk_menu_append (GTK_MENU (menu), menu_devices);
  gtk_menu_append (GTK_MENU (menu), menu_remove);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("bt-off"));
  gtk_widget_show (icon);
  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("bt-off"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 2, 0);
  gdk_bitmap_unref (bitmap);

  gpe_set_window_icon (window, "bt-off");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the Bluetooth control.\nTap here to turn the radio on and off, or to see a list of Bluetooth devices."), NULL);

  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  gtk_widget_show (window);

  atexit (do_stop_radio);

  gpe_system_tray_dock (window->window);

  gtk_main ();

  exit (0);
}
