/*
 * gpe-aerial (c) 2003 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Base applet taken from gpe-bluetooth (see below)
 *
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
//#include <time.h>
#include <libintl.h>

#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <poll.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/gpe-iconlist.h>
#include <gpe/tray.h>
#include <gpe/popup.h>
#include <gpe/spacing.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "main.h"
#include "prismstumbler.h"

#define _(x) gettext(x)

/* we use the prismstumbler scan engine */
#define SCANNER_EXEC "/tmp/prismstumbler"

static GThread *scan_thread;

struct gpe_icon my_icons[] = {
	{"scan-on", "scan-on-16"},
	{"scan-off", "scan-off-16"},
	{"network", PREFIX "/share/pixmaps/pccard-network.png"},
	{"gpe-aerial"},
	{NULL}
};

static GtkWidget *icon;

static pid_t scanner_pid;
static int sock;
static psconfig_t cfg = { 1, "eth0", DT_ORINOCO, 40000, 0, 0, FALSE, "\0" };
static netinfo_t **netlist = NULL;
static int netcount = 0;
static guint timeout_id = 0;

static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;
static GtkWidget *devices_window;
static GtkWidget *iconlist;

gboolean radio_is_on = FALSE;
GdkWindow *dock_window;
static GSList *devices = NULL;


static gboolean run_scan (void);
static void radio_off (void);
static void list_add_net (netinfo_t * ni);



static void
check_connection (GtkWidget * w, netinfo_t * this_net)
{
	/* we need to switch the scanner off */
	radio_off ();
}


void
send_config ()
{
	psmessage_t msg;
	msg.type = msg_config;
	msg.content.cfg = cfg;

	if (write (sock, (void *) &msg, sizeof (psmessage_t)) < 0)
	{
		perror ("err sending config data");
	}
}


void
send_command (command_t cmd)
{
	psmessage_t msg;
	msg.type = msg_command;
	msg.content.command.command = cmd;

	if (write (sock, (void *) &msg, sizeof (psmessage_t)) < 0)
	{
		perror ("err sending command");
	}
}


static int
fork_scanner ()
{
	if (access (SCANNER_EXEC, X_OK) == 0)
	{
		pid_t p = vfork ();
		if (p == 0)
		{
			execl (SCANNER_EXEC, SCANNER_EXEC, NULL);
			perror (SCANNER_EXEC);
			_exit (1);
		}

		return p;
	}

	return 0;
}


static gboolean
devices_window_destroyed (void)
{
	devices_window = NULL;
	
	/* stop updates from scanner */
	if (timeout_id) 
	{
		gtk_timeout_remove(timeout_id);
		timeout_id = 0;
	}
	cfg.autosend = FALSE;
	send_config ();

	return FALSE;
}


GtkWidget *
bt_progress_dialog (gchar * text, GdkPixbuf * pixbuf)
{
	GtkWidget *window;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	hbox = gtk_hbox_new (FALSE, 0);
	image = gtk_image_new_from_pixbuf (pixbuf);
	label = gtk_label_new (text);

	gtk_window_set_type_hint (GTK_WINDOW (window),
				  GDK_WINDOW_TYPE_HINT_DIALOG);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

	gtk_container_set_border_width (GTK_CONTAINER (hbox),
					gpe_get_border ());

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (window), hbox);

	return window;
}


static void
show_networks (void)
{
	if (devices_window == NULL)
	{
		devices_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

		gtk_window_set_title (GTK_WINDOW (devices_window),
				      _("Wireless networks"));
		gpe_set_window_icon (devices_window, "gpe-aerial");

		iconlist = gpe_iconlist_new ();
		gtk_container_add (GTK_CONTAINER (devices_window), iconlist);
		gpe_iconlist_set_embolden (GPE_ICONLIST (iconlist), FALSE);
		gpe_iconlist_set_icon_size (GPE_ICONLIST (iconlist), 42);

		g_signal_connect (G_OBJECT (devices_window), "destroy",
				  G_CALLBACK (devices_window_destroyed),
				  NULL);
	}

	gpe_iconlist_clear (GPE_ICONLIST (iconlist));

	scan_thread =
		g_thread_create ((GThreadFunc) run_scan, NULL, FALSE, NULL);

	if (scan_thread == NULL)
		gpe_perror_box (_("Unable to scan for devices"));

}


static void
do_stop_radio (void)
{
	radio_is_on = FALSE;

	if (scanner_pid)
	{
		kill (scanner_pid, 15);
		scanner_pid = 0;
	}

}


static void
radio_off (void)
{
	if (timeout_id) 
	{
		gtk_timeout_remove(timeout_id);
		timeout_id = 0;
	}
	gtk_widget_hide (menu_radio_off);
	gtk_widget_show (menu_radio_on);
	gtk_widget_set_sensitive (menu_devices, FALSE);

	gtk_image_set_from_pixbuf (GTK_IMAGE (icon),
				   gpe_find_icon ("scan-off"));

	if (sock >= 0)
		close (sock);

	do_stop_radio ();
}


static void
update_netlist (psnetinfo_t * anet)
{
	int i;
	int found = FALSE;

	for (i = 0; i < netcount; i++)
		if (!strncmp (anet->bssid, netlist[i]->net.bssid, 17))
		{
			found = TRUE;
			memcpy (&netlist[i]->net, anet, sizeof (psnetinfo_t));
		}
	if (!found)
	{
		netcount++;
		netlist = realloc (netlist, netcount * sizeof (netinfo_t*));
		netlist[netcount - 1] = malloc (sizeof (netinfo_t));
		memcpy (&netlist[netcount - 1]->net, anet,
			sizeof (psnetinfo_t));
		list_add_net(netlist[netcount - 1]);
	}
}


static int
get_networks (void)
{
	static psmessage_t msg;
	struct pollfd pfd[1];
	pfd[0].fd = sock;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
	while (poll (pfd, 1, 300) > 0)
	{
		if (read (sock, (void *) &msg, sizeof (psmessage_t)) < 0)
		{
			perror ("err receiving data packet");
			radio_off();
			return FALSE;
		}
		else
		{
			switch (msg.type)
			{
			case (msg_network):
				printf ("got net: %s\n",
					msg.content.net.bssid);
				update_netlist (&msg.content.net);
				break;
			case (msg_config):
				memcpy (&cfg, &msg.content.cfg,
					sizeof (psconfig_t));
				break;
			default:
				break;
			}
		}
	}
	return TRUE;
}


static void
network_info (netinfo_t * ni)
{
	char *tmp;
	GtkWidget *window = gtk_dialog_new ();
	GtkWidget *vbox1 = gtk_vbox_new (FALSE, 0);
	GtkWidget *hbox1 = gtk_hbox_new (FALSE, 0);
	GtkWidget *labelname = gtk_label_new (NULL);
	GtkWidget *labeladdr = gtk_label_new (ni->net.bssid);
	GtkWidget *image = gtk_image_new_from_pixbuf (ni->pix);
	GtkWidget *dismiss = gtk_button_new_from_stock (GTK_STOCK_OK);
	GtkWidget *lType = gtk_label_new (NULL);
	GtkWidget *lSignal = gtk_label_new (NULL);
	GtkWidget *lSpeed = gtk_label_new (NULL);
	GtkWidget *lChannel = gtk_label_new (NULL);
	GtkWidget *lWEP = gtk_label_new (NULL);
	GtkWidget *lSubnet = gtk_label_new (NULL);
	GtkWidget *lIPSec = gtk_label_new (NULL);
	GtkWidget *lDhcp = gtk_label_new (NULL);

	gtk_window_set_title (GTK_WINDOW (window), _("Network information"));
	gpe_set_window_icon (GTK_WIDGET (window), "gpe-aerial");
	gtk_box_set_spacing(GTK_BOX(vbox1),gpe_get_boxspacing());

	tmp = g_strdup_printf ("<b>BSSID: %s</b>", ni->net.ssid);
	gtk_label_set_markup (GTK_LABEL (labelname), tmp);
	g_free (tmp);

	if (ni->net.isadhoc)
		tmp = g_strdup_printf ("%s: %s", _("Mode"), "Ad-Hoc (IBSS)");
	else
		tmp = g_strdup_printf ("%s: %s", _("Mode"), "Managed (BSS)");

	gtk_label_set_text (GTK_LABEL (lType), tmp);

	g_free (tmp);
	tmp = g_strdup_printf ("%s: %d", _("Max. signal"),
			       ni->net.maxsiglevel);
	gtk_label_set_text (GTK_LABEL (lSignal), tmp);
	g_free (tmp);
	if (ni->net.speed) 
		tmp = g_strdup_printf ("%s: %3.1f Mb/s", _("Speed"), (float)ni->net.speed/1000.0);
	else
		tmp = g_strdup_printf ("%s: %s", _("Speed"), _("<i>unknown</i>"));
	gtk_label_set_markup (GTK_LABEL (lSpeed), tmp);
	g_free (tmp);
	tmp = g_strdup_printf ("%s: %d", _("Channel"), ni->net.channel);
	gtk_label_set_text (GTK_LABEL (lChannel), tmp);
	g_free (tmp);
	if (ni->net.wep)
		tmp = g_strdup_printf ("%s: %s", _("WEP enabled"), _("yes"));
	else
		tmp = g_strdup_printf ("%s: %s", _("WEP enabled"), _("no"));
	gtk_label_set_text (GTK_LABEL (lWEP), tmp);
	g_free (tmp);
	if (ni->net.ipsec)
		tmp = g_strdup_printf ("%s: %s", _("IPSec detected"), _("yes"));
	else
		tmp = g_strdup_printf ("%s: %s", _("IPSec detected"), _("no"));
	gtk_label_set_text (GTK_LABEL (lIPSec), tmp);
	g_free (tmp);
	if (ni->net.dhcp)
		tmp = g_strdup_printf ("%s: %s", _("DHCP detected"), _("yes"));
	else
		tmp = g_strdup_printf ("%s: %s", _("DHCP detected"), _("no"));
	gtk_label_set_text (GTK_LABEL (lDhcp), tmp);
	g_free (tmp);
	if (ni->net.ip_range[0])
		tmp = g_strdup_printf ("%s: %d.%d.%d.%d", _("Subnet"),
			       ni->net.ip_range[0], ni->net.ip_range[1],
			       ni->net.ip_range[2], ni->net.ip_range[3]);
	else
		tmp = g_strdup_printf ("%s: %s", _("Subnet"), _("<i>unknown</i>"));
	gtk_label_set_markup (GTK_LABEL (lSubnet), tmp);
	g_free (tmp);

	gtk_misc_set_alignment (GTK_MISC (labelname), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (labeladdr), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lType), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lSignal), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lSpeed), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lChannel), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lWEP), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lIPSec), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lSubnet), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lDhcp), 0.0, 0.5);

	gtk_misc_set_alignment (GTK_MISC (image), 0.0, 0.0);

	gtk_box_pack_start (GTK_BOX (vbox1), labelname, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), labeladdr, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lType, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lSignal, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lSpeed, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lChannel, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lWEP, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lIPSec, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lDhcp, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), lSubnet, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 8);
	gtk_box_pack_start (GTK_BOX (hbox1), image, TRUE, TRUE, 8);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox1, FALSE,
			    FALSE, 0);

	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (window)->action_area), dismiss,
			  FALSE, FALSE, 0);

	gtk_widget_realize (window);
	gdk_window_set_transient_for (window->window, devices_window->window);

	gtk_widget_show_all (window);

	g_signal_connect_swapped (G_OBJECT (dismiss), "clicked",
				  G_CALLBACK (gtk_widget_destroy), window);
}


static void
show_network_info (GtkWidget * w, netinfo_t * this_net)
{
	network_info (this_net);
}


static void
device_clicked (GtkWidget * widget, GdkEventButton * e, gpointer data)
{
	GtkWidget *device_menu;
	GtkWidget *details;
	netinfo_t *ni = (netinfo_t *) data;

	device_menu = gtk_menu_new ();

	details = gtk_menu_item_new_with_label (_("Details ..."));
	g_signal_connect (G_OBJECT (details), "activate",
			  G_CALLBACK (show_network_info), ni);
	gtk_widget_show (details);
	gtk_menu_append (GTK_MENU (device_menu), details);

	details = gtk_menu_item_new_with_label (_("Try connect ..."));
	g_signal_connect (G_OBJECT (details), "activate",
			  G_CALLBACK (check_connection), ni);
	gtk_widget_show (details);
	gtk_menu_append (GTK_MENU (device_menu), details);
	gtk_widget_set_sensitive (details, strlen (ni->net.ssid));

	gtk_menu_popup (GTK_MENU (device_menu), NULL, NULL, NULL, widget, 1,
			GDK_CURRENT_TIME);
}

static void
list_add_net (netinfo_t * ni)
{
	GObject *item;

printf("adding %s %s\n",ni->net.ssid,ni->net.bssid);
	if (ni->net.isadhoc)
		ni->pix = gpe_find_icon ("network");
	else
		ni->pix = gpe_find_icon ("gpe-aerial");
	gdk_pixbuf_ref (ni->pix);

	devices = g_slist_append (devices, ni);
	if (strlen (ni->net.ssid))
		item = gpe_iconlist_add_item_pixbuf (GPE_ICONLIST
						     (iconlist),
						     ni->net.ssid,
						     ni->pix, ni);
	else
		item = gpe_iconlist_add_item_pixbuf (GPE_ICONLIST
						     (iconlist),
						     _("<hidden>"),
						     ni->pix, ni);
	gtk_widget_show_all(GTK_WIDGET(devices_window));
	g_signal_connect (G_OBJECT (item), "button-release",
			  G_CALLBACK (device_clicked), ni);
}


static gboolean
run_scan (void)
{
	GtkWidget *w;

	gdk_threads_enter ();
	w = bt_progress_dialog (_("Scanning for networks..."),
				gpe_find_icon ("gpe-aerial"));
	gtk_widget_show_all (w);
	gdk_threads_leave ();

	sleep (9);
	send_command (C_SENDLIST);
	sleep (1);
	gdk_threads_enter ();
	get_networks ();
	gdk_threads_leave ();
	cfg.autosend = TRUE;
	send_config ();

	if (netcount <= 0)
	{
		gdk_threads_enter ();
		gtk_widget_destroy (w);
		gpe_perror_box_nonblocking (_("No nets available...\nScanning will continue."));
		gtk_widget_show_all (devices_window);
		timeout_id = gtk_timeout_add(1000,(GtkFunction)get_networks,NULL);
		gdk_threads_leave ();
		return FALSE;
	}

	gdk_threads_enter ();
	gtk_widget_destroy (w);
	gtk_widget_show_all (devices_window);

	timeout_id = gtk_timeout_add(1000,(GtkFunction)get_networks,NULL);
	gdk_threads_leave ();

	return TRUE;
}


static void
radio_on (void)
{
	sigset_t sigs;
	struct sockaddr_un name;

	gtk_widget_hide (menu_radio_on);
	gtk_widget_show (menu_radio_off);
	gtk_widget_set_sensitive (menu_devices, TRUE);

	gtk_image_set_from_pixbuf (GTK_IMAGE (icon),
				   gpe_find_icon ("scan-on"));
	radio_is_on = TRUE;
	sigemptyset (&sigs);
	sigaddset (&sigs, SIGCHLD);
	sigprocmask (SIG_BLOCK, &sigs, NULL);
	scanner_pid = fork_scanner ();
	sigprocmask (SIG_UNBLOCK, &sigs, NULL);

	usleep (200000);
	sock = socket (AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("opening datagram socket");
		radio_off ();
		return;
	}

	name.sun_family = AF_UNIX;
	strcpy (name.sun_path, PS_SOCKET);
	if (connect (sock, (struct sockaddr *) &name, SUN_LEN (&name)))
	{
		perror ("connecting to socket");
		radio_off ();
		return;
	}

	cfg.autosend = FALSE;
	send_config ();
	printf ("socket -->%s\n", PS_SOCKET);
}


static void
sigchld_handler (int sig)
{
	int status;
	pid_t p = waitpid (0, &status, WNOHANG);

	if (p == scanner_pid)
	{
		scanner_pid = 0;
		if (radio_is_on)
		{
			gpe_error_box_nonblocking (_
						   ("scanner died unexpectedly"));
			radio_off ();
		}
	}
	else if (p > 0)
	{
		fprintf (stderr, "unknown pid %d exited\n", p);
	}
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
	g_timeout_add (time, (GSourceFunc) cancel_dock_message,
		       (gpointer) id);
}

static void
clicked (GtkWidget * w, GdkEventButton * ev)
{
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gpe_popup_menu_position,
			w, ev->button, ev->time);
}

int
main (int argc, char *argv[])
{
	Display *dpy;
	GtkWidget *window;
	GdkBitmap *bitmap;
	GtkWidget *menu_remove;
	GtkTooltips *tooltips;

	g_thread_init (NULL);
	gdk_threads_init ();

	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	setlocale (LC_ALL, "");

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	window = gtk_plug_new (0);
	gtk_widget_set_usize (window, 22, 16);
	gtk_widget_realize (window);

	gtk_window_set_title (GTK_WINDOW (window), _("Wireless LAN control"));

	signal (SIGCHLD, sigchld_handler);

	menu = gtk_menu_new ();
	menu_radio_on = gtk_menu_item_new_with_label (_("Switch scanner on"));
	menu_radio_off =
		gtk_menu_item_new_with_label (_("Switch scanner off"));
	menu_devices = gtk_menu_item_new_with_label (_("Networks..."));
	menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));

	g_signal_connect (G_OBJECT (menu_radio_on), "activate",
			  G_CALLBACK (radio_on), NULL);
	g_signal_connect (G_OBJECT (menu_radio_off), "activate",
			  G_CALLBACK (radio_off), NULL);
	g_signal_connect (G_OBJECT (menu_devices), "activate",
			  G_CALLBACK (show_networks), NULL);
	g_signal_connect (G_OBJECT (menu_remove), "activate",
			  G_CALLBACK (gtk_main_quit), NULL);

	if (!radio_is_on)
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

	icon = gtk_image_new_from_pixbuf (gpe_find_icon
					  (radio_is_on ? "scan-on" :
					   "scan-off"));
	gtk_widget_show (icon);
	gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("scan-off"), NULL,
					   &bitmap, 255);
	gtk_widget_shape_combine_mask (window, bitmap, 2, 2);
	gdk_bitmap_unref (bitmap);

	gpe_set_window_icon (window, "gpe-aerial");

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window,
			      _("This is the wireless LAN selector."), NULL);

	g_signal_connect (G_OBJECT (window), "button-press-event",
			  G_CALLBACK (clicked), NULL);
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

	gtk_container_add (GTK_CONTAINER (window), icon);

	dpy = GDK_WINDOW_XDISPLAY (window->window);

	gtk_widget_show (window);

	atexit (do_stop_radio);

	dock_window = window->window;
	gpe_system_tray_dock (window->window);

	gdk_threads_enter ();
	gtk_main ();
	gdk_threads_leave ();

	exit (0);
}
