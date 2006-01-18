/*
 * gpe-mininet (c) 2004 - 2006 Florian Boor <florian.boor@kernelconcepts.de>
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/tray.h>
#include <gpe/infoprint.h>
#include <gpe/popup.h>
#include <gpe/spacing.h>

#include "main.h"
#include "netlink.h"

#define _(x) gettext(x)

#define BIN_INFO PREFIX "/bin/gpe-info"
#define BIN_CONFIG PREFIX "/bin/gpe-conf"
#define BIN_PING "/bin/ping"

#define PARAM_INFO "network"
#define PARAM_CONFIG "network"

struct gpe_icon my_icons[] = {
	{"net-on", "net-on"},
	{"net-off", "net-off"},
	{"net-on-48", "net-on-48"},
	{"net-off-48", "net-off-48"},
	{"gpe-mininet", "gpe-mininet"},
	{NULL}
};

typedef enum
{
	ICON_NET_OFF,
	ICON_NET_ON,
	NUM_ICONS
}
n_images;


static GtkWidget *window;
static GtkWidget *menu;
static GtkWidget *icon;

gboolean net_is_on = FALSE;

static int netlink_fd;

/* Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask   MTU      Window  IRTT
   eth0    0105A8C0        0102A8C0        0006    0       0       0       FFFFFFF0                     */

#define RTF_UP  0x1

static void
update_icon(gint size)
{
	GdkBitmap *bitmap;
	GdkPixbuf *sbuf, *dbuf;
	
	if (size <= 0)
		size = gdk_pixbuf_get_width(gtk_image_get_pixbuf(GTK_IMAGE(icon)));
	
	if (size > 16)
		sbuf = gpe_find_icon ( net_is_on ? "net-on-48" : "net-off-48" );
	else
		sbuf = gpe_find_icon ( net_is_on ? "net-on" : "net-off" );
	dbuf = gdk_pixbuf_scale_simple(sbuf, size, size, GDK_INTERP_HYPER);
	gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 64);
	gtk_widget_shape_combine_mask (GTK_WIDGET(window), NULL, 1, 0);
	gtk_widget_shape_combine_mask (GTK_WIDGET(window), bitmap, 1, 0);
	gdk_bitmap_unref (bitmap);
	gtk_image_set_from_pixbuf (GTK_IMAGE(icon), dbuf);
}

static gboolean
net_get_status()
{
	FILE *pipe;
	char buffer[256];
	int result = FALSE;
	char gw[9];
	char *ip = NULL;
	int ipdigit[4];
	
	pipe = fopen ("/proc/net/route", "r");

	if (pipe)
	{
  		while ((feof(pipe) == 0))
		{
			if (fgets (buffer, 255, pipe))
			{
				char *p, *q;
				int flags;

				p = strchr (buffer, '\t');
				if (!p)
					continue;
				while (isspace (*p))
					p++;
				q = strchr (p, '\t');
				if (!q)
					continue;
				*q++ = 0;
				if (strcmp (p, "00000000"))
					continue;	// Not default route
				while (isspace (*q))
					q++;
				strncpy(gw,q,8); // copy ip of gw
				gw[8] = 0;
				p = strchr (q, '\t');	// Skip over gateway
				if (!p)
					continue;
				while (isspace (*p))
					p++;
				q = strchr (p, '\t');
				if (!q)
					continue;
				*q = 0;
				if (!sscanf (p, "%x", &flags))
					continue;
				if (flags & RTF_UP)
				{
					result = TRUE;
					break;
				}
			}
		}
		pclose(pipe);		
	}
	/* check if we can ping our default gw */
	if (result)
	{
		sscanf(gw,"%02x%02x%02x%02x",
			&ipdigit[0],&ipdigit[1],&ipdigit[2],&ipdigit[3]);
		ip = g_strdup_printf("%s -c 1 -q %i.%i.%i.%i",
			BIN_PING,ipdigit[3],ipdigit[2],ipdigit[1],ipdigit[0]);

		pipe = popen (ip, "r");
		if (pipe > 0)
		{
			while ((feof(pipe) == 0))
			{
				fgets (buffer, 255, pipe);
				if (strstr(buffer,"100%"))
				  result = FALSE;
			}
			pclose(pipe);		
		}		
	}
	return result;
}

gboolean
update_netstatus (void)
{
	gboolean oldstatus = net_is_on;
	
	net_is_on = net_get_status();
	
	if (net_is_on != oldstatus)
	{
		gpe_popup_infoprint(GDK_DISPLAY(),
			net_is_on ? _("Network connection established.") 
				: _("Network connection lost."));
		update_icon(0);
	}
	return TRUE;
}


static void
app_shutdown ()
{
	gtk_main_quit ();
}


static void
do_network_info (void)
{
	GError *err = NULL;

	if (!g_spawn_command_line_async (BIN_INFO " " PARAM_INFO, &err))
	{
		gpe_error_box (_("Could not start network info tool!"));
		g_printerr("err gpe-mininet: %s\n", err->message);
		g_error_free(err);
	}
}


static void
do_network_config (void)
{
	GError *err = NULL;

	if (!g_spawn_command_line_async (BIN_CONFIG " " PARAM_CONFIG, &err))
	{
		gpe_error_box (_("Could not start network configuration tool!"));
		g_printerr("err gpe-mininet: %s\n", err->message);
		g_error_free(err);
	}
}

static void
sigterm_handler (int sig)
{
	app_shutdown ();
}


static void
clicked (GtkWidget *w, GdkEventButton *ev, gpointer data)
{
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gpe_popup_menu_position,
	                w, ev->button, ev->time);
}


static gboolean
rtnl_callback (GIOChannel *source, GIOCondition cond, gpointer data)
{
	rtnl_process ((int)data);

	return TRUE;
}


void
rtnl_connect_glib (int fd)
{
	GIOChannel *chan;

	chan = g_io_channel_unix_new (fd);

	g_io_add_watch (chan, G_IO_IN, rtnl_callback, (gpointer)fd);
}

/* handle resizing */
gboolean 
external_event(GtkWindow *window, GdkEventConfigure *event, gpointer user_data)
{
	gint size;

	if (event->type == GDK_CONFIGURE)
	{
		size = (event->width < event->height) ? event->height : event->width;
		update_icon(size);
	}
	return FALSE;
}


int
main (int argc, char *argv[])
{
	Display *dpy;
	GtkWidget *menu_remove;
	GtkWidget *menu_config;
	GtkWidget *menu_info;
	GtkTooltips *tooltips;

	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	setlocale (LC_ALL, "");

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	netlink_fd = rtnl_open ();
	if (netlink_fd < 0)
	  {
	    gpe_error_box (_("Couldn't open netlink device"));
	    exit (1);
	  }

	rtnl_connect_glib (netlink_fd);

	window = gtk_plug_new (0);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
	gtk_widget_realize (window);

	gtk_window_set_title (GTK_WINDOW (window), _("Network Control"));

	signal (SIGTERM, sigterm_handler);

	menu = gtk_menu_new ();
	menu_info = gtk_menu_item_new_with_label (_("Network Status"));
	menu_config =
		gtk_menu_item_new_with_label (_("Configure Network"));
	menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));

	g_signal_connect (G_OBJECT (menu_info), "activate",
			  G_CALLBACK (do_network_info), NULL);
	g_signal_connect (G_OBJECT (menu_config), "activate",
			  G_CALLBACK (do_network_config), NULL);
	g_signal_connect (G_OBJECT (menu_remove), "activate",
			  G_CALLBACK (app_shutdown), NULL);

	gtk_widget_set_sensitive(menu_info, !access(BIN_INFO, X_OK));
	gtk_widget_set_sensitive(menu_config, !access(BIN_CONFIG, X_OK));

	gtk_widget_show (menu_info);
	gtk_widget_show (menu_config);
	gtk_widget_show (menu_remove);

	gtk_menu_append (GTK_MENU (menu), menu_info);
	gtk_menu_append (GTK_MENU (menu), menu_config);
	gtk_menu_append (GTK_MENU (menu), menu_remove);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	net_is_on = net_get_status();

	icon = gtk_image_new_from_pixbuf (gpe_find_icon(net_is_on ? "net-on" :
	                                                "net-off")); 
	gtk_misc_set_alignment (GTK_MISC(icon), 0, 0);
	gtk_widget_show (icon);	
	gpe_set_window_icon (window, "gpe-mininet");

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window,
			      _("This is gpe-mininet - the network control applet."), NULL);

	g_signal_connect (G_OBJECT (window), "button-press-event",
	                  G_CALLBACK (clicked), NULL);
	g_signal_connect (G_OBJECT (window), "configure-event", 
	                  G_CALLBACK (external_event), NULL);
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

	gtk_container_add (GTK_CONTAINER (window), icon);
	
	dpy = GDK_WINDOW_XDISPLAY (window->window);

	gtk_widget_show (window);

	gpe_system_tray_dock (window->window);

	gtk_main ();
	
	exit (0);
}
