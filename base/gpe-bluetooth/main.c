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
#include <gpe/gpe-iconlist.h>
#include "tray.h"

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define _(x) gettext(x)

#define HCIATTACH "/etc/bluetooth/hciattach"

struct gpe_icon my_icons[] = {
  { "bt-on" },
  { "bt-off" },
  { "cellphone" },
  { "network" },
  { NULL }
};

struct bt_device
{
  gchar *name;
  guint class;
  bdaddr_t bdaddr;
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
run_scan (void)
{
  bdaddr_t bdaddr;
  inquiry_info *info = NULL;
  int dev_id = hci_get_route(&bdaddr);
  int num_rsp, length, flags;
  char name[248];
  int i, opt, dd;

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

      memset(name, 0, sizeof(name));
      if (hci_read_remote_name (dd, &(info+i)->bdaddr, sizeof(name), name, 100000) < 0)
	strcpy (name, "n/a");
      baswap (&bdaddr, &(info+i)->bdaddr);

      bd = g_malloc (sizeof (struct bt_device));
      bd->name = g_strdup (name);
      memcpy (&bd->bdaddr, &bdaddr, sizeof (bdaddr));
      bd->class = ((info+i)->dev_class[2] << 16) | ((info+i)->dev_class[1] << 8) | (info+i)->dev_class[0];
      devices = g_slist_append (devices, bd);
    }
  
  close (dd);
  close (dev_id);
  return TRUE;
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

static gboolean
devices_window_destroyed (void)
{
  devices_window = NULL;

  return FALSE;
}

static gboolean
browse_device (bdaddr_t *bdaddr)
{
  GSList *attrid, *search;
  GSList *pSeq = NULL;
  GSList *pGSList = NULL;
  uint32_t range = 0x0000ffff;
  uint16_t count = 0;
  int status = -1, i;
  sdp_access_proto_t *access_proto = NULL;
  uuid_t group;
  char str[20];

  sdp_create_uuid16 (&group, PUBLIC_BROWSE_GROUP);

  attrid = g_slist_append(NULL, &range);
  search = g_slist_append(NULL, &group);
  status = sdp_service_search_attr_req(bdaddr, search, SDP_ATTR_REQ_RANGE,
				       attrid, 65535, &pSeq, &count);

  if (status) 
    {
      gpe_error_box (_("Service search failed"));
      return FALSE;
    }

  for (i = 0; i < (int) g_slist_length(pSeq); i++) {
    uint32_t svcrec;
    svcrec = *(uint32_t *) g_slist_nth_data(pSeq, i);
    
    sdp_svcrec_print(bdaddr, svcrec);
    
    printf("Service RecHandle: 0x%x\n", svcrec);
    
    if (sdp_get_svclass_id_list(bdaddr, svcrec, &pGSList) == 0) {
      printf("Service Class ID List:\n");
      g_slist_foreach(pGSList, print_service_class, NULL);
    }
    
    if (sdp_get_access_proto(bdaddr, svcrec, &access_proto) == 0) {
      printf("Protocol Descriptor List:\n");
      g_slist_foreach(access_proto->seq, 
		      print_access_proto, NULL);
    }
    
    if (sdp_get_lang_attr(bdaddr, svcrec, &pGSList) == 0) {
      printf("Language Base Atrr List:\n");
      g_slist_foreach(pGSList, print_lang_attr, NULL);
    }
    
    if (sdp_get_profile_desc(bdaddr, svcrec, &pGSList) == 0) {
      printf("Profile Descriptor List:\n");
      g_slist_foreach(pGSList, print_profile_desc, NULL);
    }
    
    printf("\n");
    
    if (sdp_is_group(bdaddr, svcrec)) {
      uuid_t grp;
      sdp_get_group_id(bdaddr, svcrec, &grp);
      if (grp.value.uuid16 != group.value.uuid16)
	do_browse(bdaddr, &grp);
    }
  }

  return status;
}

static void
show_devices (void)
{
  GSList *iter;

  if (devices_window == NULL)
    {
      devices_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_window_set_title (GTK_WINDOW (devices_window), _("Bluetooth devices"));

      iconlist = gpe_iconlist_new ();
      gtk_widget_show (iconlist);
      gtk_container_add (GTK_CONTAINER (devices_window), iconlist);

      gtk_signal_connect (devices_window, "destroy", GTK_SIGNAL_FUNC (devices_window_destroyed), NULL);
    }

  gtk_widget_show (devices_window);

  run_scan ();

  for (iter = devices; iter; iter = iter->next)
    {
      struct bt_device *bd = iter->data;
      gchar *icon = NULL;
      switch (bd->class & 0x1f00)
	{
	case 0x200:
	  icon = PREFIX "/share/gpe/pixmaps/default/cellphone.png";
	  break;
	case 0x300:
	  icon = PREFIX "/share/gpe/pixmaps/default/network.png";
	  break;
	default:
	  printf("Don't know what to do about class %x\n", bd->class & 0x1f00);
	  break;
	}
      fprintf (stderr, "icon is %s\n", icon);
      gpe_iconlist_add_item (iconlist, bd->name, icon, NULL);
    }
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
	  gpe_error_box (_("hcid died unexpectedly"));
	  radio_off ();
	}
    }
  else if (p == hciattach_pid)
    {
      hciattach_pid = 0;
      if (radio_is_on)
	{
	  gpe_error_box (_("hciattach died unexpectedly"));
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
  gtk_signal_connect (GTK_OBJECT (menu_devices), "activate", GTK_SIGNAL_FUNC (show_devices), NULL);

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
  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("bt-off"), NULL, &bitmap, 255);
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
