/*
 * gpe-aerial (c) 2003 - 2005 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Basic applet skeleton taken from gpe-bluetooth (see below)
 *
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#include <sys/types.h>
#include <stdlib.h>
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
#include <gpe/tray.h>
#include <gpe/popup.h>
#include <gpe/spacing.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "main.h"
#include "prismstumbler.h"
#include "netdb.h"
#include "netedit.h"

#define _(x) gettext(x)

/* we use the prismstumbler scan engine */
#define SCANNER_EXEC PREFIX "/bin/prismstumbler"

#define CL_RED 		"#FF6666"
#define CL_GREEN 	"#66FF66"
#define CL_YELLOW 	"#FFFF66"

#define PIXMAP_SIZE  GTK_ICON_SIZE_DIALOG

static GThread *scan_thread;

struct gpe_icon my_icons[] = {
	{"scan-on", PREFIX "/share/pixmaps/scan-on.png"},
	{"scan-off", PREFIX "/share/pixmaps/scan-off.png"},
	{"network", PREFIX "/share/pixmaps/pccard-network.png"},
	{"gpe-aerial", PREFIX "/share/pixmaps/gpe-aerial.png"},
	{NULL}
};

const char *dhcpcommands[] = {
	"/sbin/dhcpcd %s &",
	"/sbin/dhclient %s &",
	"/sbin/udhcpc -i %s &",
	"/usr/sbin/dhcpcd %s &",
	"/usr/sbin/udhcpc -i %s &",
	NULL
};

enum
{
	COL_ICON,
	COL_SSID,
	COL_CHANNEL,
	COL_WEP,
	COL_NET,
	COL_COLOR,
	N_COLUMNS
};


static GtkWidget *icon;

static pid_t scanner_pid;
static int sock;
static psconfig_t cfg = { 0, "eth0", DT_ORINOCO, 40000, 0 ,0, FALSE,
		"/tmp/spotkoord.txt", "/tmp/psdump.pcap", FALSE, 0};
static netinfo_t **netlist = NULL;
static int netcount = 0;
static guint timeout_id = 0;
static int net_request_mode = -1;
static gboolean cfg_changed = FALSE; 
		
static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;
GtkWidget *devices_window;

gboolean radio_is_on = FALSE;
GdkWindow *dock_window;
static GSList *devices = NULL;
GtkWidget *window;


/* some forwards */

static gboolean run_scan (void);
static void radio_off (void);
static void radio_on (void);
static void list_add_net (netinfo_t * ni);
static void send_command (command_t cmd, int par);
static void send_usernet (usernetinfo_t * usernet);
static void device_clicked (GtkWidget * widget, GdkEventButton * e,
			    gpointer data);
void update_display (netinfo_t * ni);
static void aerial_shutdown ();

static GtkTreeStore *store;
static GtkWidget *tree;
static GtkCellRenderer *renderer;

static void
image_set(int size)
{
	GdkBitmap *bitmap;
	GdkPixbuf *sbuf, *dbuf;
	
	if (size == 0)
	{
		GdkPixbuf *pbuf = gtk_image_get_pixbuf(GTK_IMAGE(icon));
		size = gdk_pixbuf_get_width(pbuf);
	}
    sbuf = gpe_find_icon(radio_is_on ? "scan-on" : "scan-off");
    dbuf = gdk_pixbuf_scale_simple(sbuf, size, size, GDK_INTERP_HYPER);
    gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 128);
    gtk_widget_shape_combine_mask (GTK_WIDGET(window), NULL, 1, 0);
    gtk_widget_shape_combine_mask (GTK_WIDGET(window), bitmap, 1, 0);
    gdk_bitmap_unref (bitmap);
    gtk_image_set_from_pixbuf(GTK_IMAGE(icon), dbuf);
}

/* find a valid dhcp command */
static int
check_dhcp(void)
{
	int i = 0;
	char cmd[50];
	
	while (dhcpcommands[i])
	{
		sscanf(dhcpcommands[i], "%50s", cmd);
		if (!access(cmd, F_OK))
			return i;
		i++;		
	}	
	return 0;	
}

/* send command to turn off device */
static void
device_off (GtkWidget * w)
{
	radio_off();
	send_command (C_IFDOWN, 0);
}

static void
draw_signal (int signal)
{
	GdkGC *gc;
	GdkColor sigc = { 0, 0x0000, 0x0000, 0xFFFF };
	GdkRectangle rect = { 0, 0, 16, 16 };
	GdkPixbuf *pbuf = gtk_image_get_pixbuf(GTK_IMAGE(icon));
	int xsize, ysize;
	
	xsize = gdk_pixbuf_get_width(pbuf);
	ysize = gdk_pixbuf_get_height(pbuf);
	rect.width = xsize;
	rect.height = ysize;
	gc = gdk_gc_new (GDK_DRAWABLE (dock_window));
	gdk_gc_set_fill (gc, GDK_SOLID);
	gdk_gc_set_rgb_fg_color (gc, &sigc);

	gdk_window_invalidate_rect (dock_window, &rect, TRUE);
	gdk_window_process_updates (dock_window, TRUE);
	gdk_draw_rectangle (GDK_DRAWABLE (dock_window), gc, TRUE, 0,
			    (xsize + 2) - signal * (ysize*2) / 255, 4,
			    signal * (ysize*2) / 255 + 2);
	g_object_unref(G_OBJECT(gc));
}


/* taken from miniwave, thanks! */

static int
get_linklevel (void)
{
	FILE *wireless;		// File handle for /proc/net/wireless
	char line[255];

	int link = 0;
	int level = 0;

	if ((wireless = fopen ("/proc/net/wireless", "r")) != NULL)
	{
		fgets (line, sizeof (line)-1, wireless);
		fgets (line, sizeof (line)-1, wireless);
		if (fgets (line, sizeof (line)-1, wireless) == NULL)
		{
		}
		else
		{
			sscanf (line, "%*s %*s %d. %d. %*d. %*d %*d %*d",
				&link, &level);
		}
		fclose (wireless);
	}
	return link;
}


static int
update_linklevel (void)
{
	if (!radio_is_on)
	{
		draw_signal (get_linklevel ());
	}
	return TRUE;
}


static void
net_enter_data (GtkWidget * w, netinfo_t * this_net)
{
	radio_off ();
	network_edit (this_net);
	save_network (&this_net->netinfo);
}


static void
check_connection (GtkWidget * w, netinfo_t * this_net)
{
	/* we need to switch the scanner off */
	radio_off ();
	send_usernet (&this_net->netinfo);
	send_command (C_ASSOCIATE, SEQ_USERNET);
	net_request_mode = this_net->netinfo.mode;
}


static void
update_usernet (netinfo_t * ni, gboolean run_display_update)
{
	if (!(ni->netinfo.userset & USET_BSSID))
		sprintf (ni->netinfo.bssid, "%s", ni->net.bssid);
	if (!(ni->netinfo.userset & USET_SSID))
		sprintf (ni->netinfo.ssid, "%s", ni->net.ssid);
	if (!(ni->netinfo.userset & USET_MODE))
	{
		if (ni->netinfo.mode != ni->net.isadhoc)
		{
			ni->netinfo.mode = ni->net.isadhoc;
			if (run_display_update)
				update_display (ni);
		}
	}
	if (!(ni->netinfo.userset & USET_WEP))
	{
		if (ni->netinfo.wep != ni->net.wep)
		{
			ni->netinfo.wep = ni->net.wep;
			if (run_display_update)
				update_display (ni);
		}
	}
	if (!(ni->netinfo.userset & USET_DHCP))
		ni->netinfo.dhcp = ni->net.dhcp;
	if (!(ni->netinfo.userset & USET_CHANNEL))
		ni->netinfo.channel = ni->net.channel;
	if (!(ni->netinfo.userset & USET_NETMASK))
	{
		if (ni->net.ip_range[0])
			ni->netinfo.netmask[0] = 255;
		if (ni->net.ip_range[1])
			ni->netinfo.netmask[1] = 255;
		if (ni->net.ip_range[2])
			ni->netinfo.netmask[2] = 255;
		if (ni->net.ip_range[3])
			ni->netinfo.netmask[3] = 255;
	}
	if (!(ni->netinfo.userset & USET_WEPKEY))
	{
		if (strcmp (ni->netinfo.wep_key, ni->net.wep_key))
		{
			memcpy (&ni->netinfo.wep_key, &ni->net.wep_key, 48);
			if (run_display_update)
				update_display (ni);
		}
	}
	ni->netinfo.inrange = TRUE;
}


static void
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


static void
send_usernet (usernetinfo_t * usernet)
{
	psmessage_t msg;
	msg.type = msg_usernet;
	msg.content.usernet = *usernet;

	if (write (sock, (void *) &msg, sizeof (psmessage_t)) < 0)
	{
		perror ("err sending user netinfo");
	}
}


static void
send_command (command_t cmd, int par)
{
	psmessage_t msg;
	msg.type = msg_command;
	msg.content.command.command = cmd;
	msg.content.command.par = par;

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
			execl (SCANNER_EXEC, SCANNER_EXEC, "-q", NULL);
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
		gtk_timeout_remove (timeout_id);
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
	static GtkTreeViewColumn *column;
	GtkWidget *sw;
	GtkTooltips *tooltips;
	
	
	if (devices_window == NULL)
	{
		devices_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_default_size(GTK_WINDOW(devices_window), 240, 310);
		gtk_window_set_title (GTK_WINDOW (devices_window),
				      _("Wireless networks"));
		gpe_set_window_icon (devices_window, "gpe-aerial");

		/* Create a view */
		sw = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);
		tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
		gtk_container_add (GTK_CONTAINER (sw), tree);
		gtk_container_add (GTK_CONTAINER (devices_window), sw);
		g_signal_connect (G_OBJECT (tree), "button-release-event",
				  G_CALLBACK (device_clicked), NULL);

		/* add help */
		tooltips = gtk_tooltips_new ();
		gtk_tooltips_set_tip(tooltips,tree,_("This window shows all detected networks. "\
											"It will be updated while the scanner is "\
											"running. The colors tell you something "\
											"about the possibility to connect to the "\
											"net. Green=OK, yellow=maybe need some "\
											"configuration, red=not possible."),NULL);
		
		renderer = gtk_cell_renderer_pixbuf_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Mode"),
								   renderer,
								   "pixbuf",
								   COL_ICON,
								   "cell-background",
								   COL_COLOR,
								   NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("ESSID"),
								   renderer,
								   "text",
								   COL_SSID,
								   "background",
								   COL_COLOR,
								   NULL);
		gtk_tree_view_column_set_resizable(column,TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("CH"),
								   renderer,
								   "text",
								   COL_CHANNEL,
								   "background",
								   COL_COLOR,
								   NULL);
		gtk_tree_view_column_set_resizable(column,TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("WEP"),
								   renderer,
								   "text",
								   COL_WEP,
								   "background",
								   COL_COLOR,
								   NULL);
		gtk_tree_view_column_set_resizable(column,TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);


		g_signal_connect (G_OBJECT (devices_window), "destroy",
				  G_CALLBACK (devices_window_destroyed),
				  NULL);
	}


	scan_thread =
		g_thread_create ((GThreadFunc) run_scan, NULL, FALSE, NULL);

	if (scan_thread == NULL)
		gpe_perror_box (_("Unable to scan for devices."));

}


static void
do_stop_radio (void)
{
	radio_is_on = FALSE;

	cfg.scan = FALSE;

	/* inform scanner */
	send_config ();
}


static void
radio_off (void)
{
	gtk_widget_hide (menu_radio_off);
	gtk_widget_show (menu_radio_on);

	do_stop_radio ();
	image_set(0);
	
	draw_signal (0);
}


static char *
get_net_color (usernetinfo_t * n)
{
	if ((n->wep) && (n->wep_key[0] == '<'))
		return CL_RED;
	if (strlen (n->ssid))
	{
		if (n->dhcp)
			return CL_GREEN;
		if ((n->ip[0] != 0) && (n->netmask[0] != 0))
			return CL_GREEN;
	}
	return CL_YELLOW;
}


void
update_display (netinfo_t * ni)
{
	gdk_pixbuf_unref (ni->pix);

	if (ni->netinfo.mode)
		ni->pix = gpe_find_icon_scaled ("network", PIXMAP_SIZE);
	else
		ni->pix = gpe_find_icon_scaled ("gpe-aerial", PIXMAP_SIZE);
	gdk_pixbuf_ref (ni->pix);

	gtk_tree_store_set (store, &ni->iter,
			    COL_ICON, ni->pix,
			    COL_SSID, ni->netinfo.ssid,
			    COL_WEP, ni->netinfo.wep,
			    COL_CHANNEL, ni->netinfo.channel, -1);

	gtk_tree_store_set (store, &ni->iter, COL_COLOR,
			    get_net_color (&ni->netinfo), -1);
}


static void
update_netlist (psnetinfo_t * anet)
{
	int i;
	int found = FALSE;
	usernetinfo_t *snet;
	for (i = 0; i < netcount; i++)
		if (!strncmp (anet->bssid, netlist[i]->net.bssid, 17))
		{
			found = TRUE;
			if (anet->isadhoc != netlist[i]->net.isadhoc)
			{
				netlist[i]->net.isadhoc = anet->isadhoc;
				update_display (netlist[i]);
			}
			memcpy (&netlist[i]->net, anet, sizeof (psnetinfo_t));
			update_usernet (netlist[i], TRUE);
			if (!netlist[i]->visible)
				list_add_net (netlist[i]);
		}
	if (!found)
	{
		netcount++;
		netlist = realloc (netlist, netcount * sizeof (netinfo_t *));
		netlist[netcount - 1] = malloc (sizeof (netinfo_t));
		memset (&netlist[netcount - 1]->netinfo, 0,
			sizeof (usernetinfo_t));

		/* try to load network info from database */
		snet = get_network (anet->bssid);

		if (snet != NULL)
			memcpy (&netlist[netcount - 1]->netinfo, snet,
				sizeof (usernetinfo_t));

		memcpy (&netlist[netcount - 1]->net, anet,
			sizeof (psnetinfo_t));

		update_usernet (netlist[netcount - 1], FALSE);
		netlist[netcount - 1]->visible = FALSE;
		list_add_net (netlist[netcount - 1]);
	}
	if (radio_is_on)
		draw_signal (anet->cursiglevel);
}


static void
do_message_info (psinfo_t * psi)
{
	GtkWidget *dialog;

	switch (psi->info)
	{
	case I_SUCCESS:
		dialog = gtk_message_dialog_new (GTK_WINDOW (devices_window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_OK,
						 _("Successfully connected."));
		g_signal_connect_swapped (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy),
					  G_OBJECT (dialog));
		gtk_widget_show (dialog);
		break;
	case I_FAILED:
		if (net_request_mode)
		{
			dialog = gtk_message_dialog_new (GTK_WINDOW
							 (devices_window),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_INFO,
							 GTK_BUTTONS_OK,
							 _("Ad-Hoc connections setup complete."));
			g_signal_connect_swapped (G_OBJECT (dialog),
						  "response",
						  G_CALLBACK
						  (gtk_widget_destroy),
						  G_OBJECT (dialog));
			gtk_widget_show (dialog);
		}
		else
			gpe_error_box_nonblocking (_("Could not connect to wireless LAN."));
		break;
	case I_ERRCARD:
	case I_NOCARD:
		gpe_error_box_nonblocking (psi->message);
		radio_off ();
		break;
	default:
		break;
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
		if ((pfd[0].revents & POLLERR) || (pfd[0].revents & POLLHUP))
		{
			perror ("Err: connection lost");
			radio_off ();
			return FALSE;
		}
		if (read (sock, (void *) &msg, sizeof (psmessage_t)) < 0)
		{
			perror ("Err receiving data packet");
			radio_off ();
			return FALSE;
		}
		else
		{
			switch (msg.type)
			{
			case msg_network:
				update_netlist (&msg.content.net);
				break;
			case msg_config:
				memcpy (&cfg, &msg.content.cfg,
					sizeof (psconfig_t));
				cfg_changed = TRUE;
				break;
			case msg_info:
				do_message_info (&msg.content.info);
				break;
			default:
				break;
			}
		}
	}
	return TRUE;
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
	netinfo_t *ni;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_get_selected (selection, NULL, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COL_NET, &ni, -1);

	device_menu = gtk_menu_new ();

	details = gtk_menu_item_new_with_label (_("Details..."));
	g_signal_connect (G_OBJECT (details), "activate",
			  G_CALLBACK (show_network_info), ni);
	gtk_widget_show (details);
	gtk_menu_append (GTK_MENU (device_menu), details);

	details = gtk_menu_item_new_with_label (_("Enter additional info"));
	g_signal_connect (G_OBJECT (details), "activate",
			  G_CALLBACK (net_enter_data), ni);
	gtk_widget_show (details);
	gtk_menu_append (GTK_MENU (device_menu), details);
	gtk_widget_set_sensitive (details, TRUE);

	details = gtk_menu_item_new_with_label (_("Try connect..."));
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
	GtkTreeIter iter;

	if (ni->netinfo.mode)
		ni->pix = gpe_find_icon_scaled ("network", PIXMAP_SIZE);
	else
		ni->pix = gpe_find_icon_scaled ("gpe-aerial", PIXMAP_SIZE);
	gdk_pixbuf_ref (ni->pix);

	devices = g_slist_append (devices, ni);

	gtk_tree_store_append (store, &iter, NULL);	/* Acquire an iterator */

	ni->iter = iter;

	gtk_tree_store_set (store, &iter,
			    COL_ICON, ni->pix,
			    COL_SSID, ni->netinfo.ssid,
			    COL_WEP, ni->netinfo.wep,
			    COL_CHANNEL, ni->netinfo.channel, COL_NET, ni,
			    -1);

	gtk_tree_store_set (store, &iter,
			    COL_COLOR, get_net_color (&ni->netinfo), -1);

	gtk_widget_show_all (GTK_WIDGET (devices_window));
	ni->visible = TRUE;
}


static gboolean
run_scan (void)
{
	GtkWidget *w;

	gdk_threads_enter ();
	if (radio_is_on == FALSE)
		radio_on ();
	gdk_threads_leave ();

	gdk_threads_enter ();
	w = bt_progress_dialog (_("Scanning for networks..."),
				gpe_find_icon ("gpe-aerial"));
	gtk_widget_show_all (w);
	gdk_threads_leave ();

	sleep (7);
	send_command (C_SENDLIST, 0);
	sleep (1);
	gdk_threads_enter ();
	get_networks ();
	gdk_threads_leave ();
	cfg.autosend = TRUE;
	send_config ();

	gdk_threads_enter ();
	gtk_widget_destroy (w);
	gtk_widget_show_all (devices_window);

	timeout_id = gtk_timeout_add (1000, (GtkFunction) get_networks, NULL);
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
	
	radio_is_on = TRUE;
	image_set(0);
	
	sigemptyset (&sigs);
	sigaddset (&sigs, SIGCHLD);
	/* start and connect to scanner */
	if (scanner_pid == 0)
	{
		sigprocmask (SIG_BLOCK, &sigs, NULL);
		scanner_pid = fork_scanner ();
		sigprocmask (SIG_UNBLOCK, &sigs, NULL);
		/* wait for scanner to come up */
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
		if (connect
		    (sock, (struct sockaddr *) &name, SUN_LEN (&name)))
		{
			perror ("connecting to socket");
			radio_off ();
			return;
		}
	}
	cfg.autosend = (gboolean) devices_window;
	send_command (C_IFUP, 0); /* turn it on */
	send_command (C_DETECT_CARD, 0);
	
	/* wait for prismstumbler to send config update */
	while (!cfg_changed)
	{
		usleep(200000);
		get_networks();
	}
	cfg.dhcpcommand = check_dhcp();
	cfg_changed = FALSE;
	cfg.scan = TRUE;
	send_config ();
}


static void
sigterm_handler (int sig)
{
	aerial_shutdown ();
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


static void
aerial_shutdown ()
{
	/* inform scanner */
	if (scanner_pid)
	{
		cfg.scan = FALSE;
		send_config ();
		kill (scanner_pid, SIGTERM);
		scanner_pid = 0;
	}

	gtk_main_quit ();
}


/* handle resizing */
gboolean 
external_event(GtkWindow *window, GdkEventConfigure *event, gpointer user_data)
{
  int size;
	
  if (event->type == GDK_CONFIGURE)
  {
    size = (event->width > event->height) ? event->height : event->width;
	image_set(size);
  }
  return FALSE;
}


int
main (int argc, char *argv[])
{
	Display *dpy;
	GdkBitmap *bitmap;
	GtkWidget *menu_remove;
	GtkWidget *menu_off;
	GtkTooltips *tooltips;
	int i;

	g_thread_init (NULL);
	gdk_threads_init ();

	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	setlocale (LC_ALL, "");

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	/* init and open database */
	init_db ();
	if (check_table () != 0)
	{
		if (create_table () != 0)
		{
			fprintf (stderr,"Unable to create database - exiting!\n");
			gtk_exit (-1);
		}
	}

	/* init tree storage stuff */
	store = gtk_tree_store_new (N_COLUMNS,
				    G_TYPE_OBJECT,
				    G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_BOOLEAN, G_TYPE_POINTER,
				    G_TYPE_STRING);

	window = gtk_plug_new (0);
	gtk_window_set_resizable(GTK_WINDOW(window),TRUE);

	gtk_widget_realize (window);

	gtk_window_set_title (GTK_WINDOW (window), _("Wireless LAN control"));

	signal (SIGCHLD, sigchld_handler);
	signal (SIGTERM, sigterm_handler);

	menu = gtk_menu_new ();
	menu_radio_on = gtk_menu_item_new_with_label (_("Switch scanner on"));
	menu_radio_off =
		gtk_menu_item_new_with_label (_("Switch scanner off"));
	menu_devices = gtk_menu_item_new_with_label (_("Networks..."));
	menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));
	menu_off = gtk_menu_item_new_with_label (_("Turn WLAN off"));

	g_signal_connect (G_OBJECT (menu_radio_on), "activate",
			  G_CALLBACK (radio_on), NULL);
	g_signal_connect (G_OBJECT (menu_radio_off), "activate",
			  G_CALLBACK (radio_off), NULL);
	g_signal_connect (G_OBJECT (menu_devices), "activate",
			  G_CALLBACK (show_networks), NULL);
	g_signal_connect (G_OBJECT (menu_remove), "activate",
			  G_CALLBACK (aerial_shutdown), NULL);
	g_signal_connect (G_OBJECT (menu_off), "activate",
			  G_CALLBACK (device_off), NULL);

	if (!radio_is_on)
	{
		gtk_widget_show (menu_radio_on);
	}

	gtk_widget_show (menu_devices);
	gtk_widget_show (menu_remove);
	gtk_widget_show (menu_off);

	gtk_menu_append (GTK_MENU (menu), menu_radio_on);
	gtk_menu_append (GTK_MENU (menu), menu_radio_off);
	gtk_menu_append (GTK_MENU (menu), menu_devices);
	gtk_menu_append (GTK_MENU (menu), menu_off);
	gtk_menu_append (GTK_MENU (menu), menu_remove);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	icon = gtk_image_new_from_pixbuf (gpe_find_icon
					  (radio_is_on ? "scan-on" : "scan-off")); 

	gtk_widget_show (icon);
	gtk_misc_set_alignment(GTK_MISC(icon),0.0,0.5);
	gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("scan-off"), NULL,
					   &bitmap, 255);
	
	gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
	gdk_bitmap_unref (bitmap);

	gpe_set_window_icon (window, "gpe-aerial");

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window,
			      _("This is gpe-aerial - the wireless LAN selector."), NULL);

	g_signal_connect (G_OBJECT (window), "button-press-event",
			  G_CALLBACK (clicked), NULL);
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);
    g_signal_connect (G_OBJECT (window), "configure-event", 
		G_CALLBACK (external_event), NULL);

	gtk_container_add (GTK_CONTAINER (window), icon);

	dpy = GDK_WINDOW_XDISPLAY (window->window);

	gtk_widget_show (window);

	atexit (do_stop_radio);

	dock_window = window->window;
	gpe_system_tray_dock (window->window);

	gtk_timeout_add (1000, (GtkFunction) update_linklevel, NULL);

	gdk_threads_enter ();
	gtk_main ();
	gdk_threads_leave ();

	/* save network data */
	for (i = 0; i < netcount; i++)
		save_network (&netlist[i]->netinfo);

	exit (0);
}
