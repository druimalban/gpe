/*
 * gpe-mininet (c) 2004 Florian Boor <florian.boor@kernelconcepts.de>
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/tray.h>
#include <gpe/popup.h>
#include <gpe/spacing.h>

#include "main.h"
#include "netlink.h"

#define _(x) gettext(x)

#define BIN_INFO PREFIX "/bin/gpe-info"
#define BIN_CONFIG PREFIX "/bin/gpe-conf"
#define PARAM_INFO "network"
#define PARAM_CONFIG "network"

struct gpe_icon my_icons[] = {
	{"net-on", "net-on"},
	{"net-off", "net-off"},
	{"gpe-mininet", "gpe-mininet"},
	{NULL}
};


static GtkWidget *icon;

static GtkWidget *menu;

gboolean net_is_on = FALSE;
GtkWidget *dock_window;

static int netlink_fd;

/* Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask   MTU      Window  IRTT
   eth0    0105A8C0        0102A8C0        0006    0       0       0       FFFFFFF0                     */

#define RTF_UP  0x1

static gboolean
net_get_status()
{
	FILE *pipe;
	char buffer[256];
	int result = FALSE;
	
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
	return result;
}


static gboolean
remove_dock_message (guint id)
{
	gpe_system_tray_cancel_message (dock_window->window, id);
	return FALSE;
}


gboolean
update_netstatus (void)
{
	GdkBitmap *bitmap;
	gboolean oldstatus = net_is_on;
	guint msg_id;
	
	net_is_on = net_get_status();
	
	if (net_is_on != oldstatus)
	{
		if (net_is_on)
			gtk_image_set_from_pixbuf (GTK_IMAGE (icon),
				   gpe_find_icon ("net-on"));
		else
			gtk_image_set_from_pixbuf (GTK_IMAGE (icon),
				   gpe_find_icon ("net-off"));
		gdk_pixbuf_render_pixmap_and_mask (
			gpe_find_icon (net_is_on ? "net-on" : "net-off"), NULL,
					   &bitmap, 128);
		gtk_widget_shape_combine_mask (dock_window, bitmap, 0, 0);
		gdk_bitmap_unref (bitmap);
		msg_id = gpe_system_tray_send_message (dock_window->window, 
			net_is_on ? _("Network connection established.") 
				: _("Network connection lost.")
			, 0);
		g_timeout_add (10000, (GSourceFunc) remove_dock_message,
		       (gpointer) msg_id);
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
	pid_t pid;
	
	pid = fork();
	switch (pid)
	{
		case -1: 
			gpe_error_box (_("Could not start info tool!\n"));
		break;
		case  0: 
			execlp(BIN_INFO,BIN_INFO,PARAM_INFO,NULL);
		break;
		default: 
		break;
	} 
}

static void
do_network_config (void)
{
	pid_t pid;
	
	pid = fork();
	switch (pid)
	{
		case -1: 
			gpe_error_box (_("Could not start config tool!\n"));
		break;
		case  0: 
			execlp(BIN_CONFIG,BIN_CONFIG,PARAM_CONFIG,NULL);
		break;
		default: 
		break;
	} 
}

static void
sigterm_handler (int sig)
{
	app_shutdown ();
}


static void
clicked (GtkWidget * w, GdkEventButton * ev)
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

int
main (int argc, char *argv[])
{
	Display *dpy;
	GtkWidget *window;
	GdkBitmap *bitmap;
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
	gtk_widget_set_usize (window, 16, 16);
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

	gtk_widget_set_sensitive(menu_info,!access(BIN_INFO,X_OK));
	gtk_widget_set_sensitive(menu_config,!access(BIN_CONFIG,X_OK));

	gtk_widget_show (menu_info);
	gtk_widget_show (menu_config);
	gtk_widget_show (menu_remove);

	gtk_menu_append (GTK_MENU (menu), menu_info);
	gtk_menu_append (GTK_MENU (menu), menu_config);
	gtk_menu_append (GTK_MENU (menu), menu_remove);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	net_is_on = net_get_status();
	
	icon = gtk_image_new_from_pixbuf (
			gpe_find_icon(net_is_on ? "net-on" :
				"net-off")); 

	gtk_widget_show (icon);
	gdk_pixbuf_render_pixmap_and_mask (
		gpe_find_icon (net_is_on ? "net-on" : "net-off"), NULL,
					   &bitmap, 128);
	
	gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
	gdk_bitmap_unref (bitmap);

	gpe_set_window_icon (window, "gpe-mininet");

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window,
			      _("This is gpe-mininet - the network control applet."), NULL);

	g_signal_connect (G_OBJECT (window), "button-press-event",
			  G_CALLBACK (clicked), NULL);
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

	gtk_container_add (GTK_CONTAINER (window), icon);

	dpy = GDK_WINDOW_XDISPLAY (window->window);

	gtk_widget_show (window);

	dock_window = window;
	gpe_system_tray_dock (window->window);

	gtk_main ();
	
	exit (0);
}
