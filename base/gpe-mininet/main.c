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
//#include <gpe/tray.h>
#include <libmb/mb.h>
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


MBPixbuf      *Pixbuf;
MBPixbufImage *AppImage;
MBTrayApp *app = NULL;

typedef enum
{
	ICON_NET_OFF,
	ICON_NET_ON,
	NUM_ICONS
}
n_images;

MBPixbufImage *net_status_icons[NUM_ICONS];

static GtkWidget *menu;

gboolean net_is_on = FALSE;

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
//	gpe_system_tray_cancel_message (GDK_WINDOW(), id);
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
		mb_tray_app_repaint (app);
/*		msg_id = gpe_system_tray_send_message (GDK_WINDOW(), 
			net_is_on ? _("Network connection established.") 
				: _("Network connection lost.")
			, 0);
		
		g_timeout_add (10000, (GSourceFunc) remove_dock_message,
		       (gpointer) msg_id);
*/	}
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
clicked (MBTrayApp *app, int x, int y, Bool is_released )
//clicked (GtkWidget * w, GdkEventButton * ev)
{
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL/*gpe_popup_menu_position*/,
		NULL,0, gtk_get_current_event_time());
			//w, ev->button, ev->time);
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


void 
paint_callback ( MBTrayApp *app, Drawable drw ) 
{ 
	MBPixbufImage *img_scaled;
	MBPixbufImage *img_backing = NULL;
	int use_index = net_is_on ? ICON_NET_ON : ICON_NET_OFF;
  
	img_backing = mb_tray_app_get_background (app, Pixbuf);

	mb_pixbuf_img_composite(Pixbuf, img_backing, 
			  net_status_icons[use_index], 
			  0, 0);
	mb_pixbuf_img_render_to_drawable(Pixbuf, img_backing, drw, 0, 0);
	mb_pixbuf_img_free( Pixbuf, img_backing );
}


GdkFilterReturn
event_filter (GdkXEvent *xev, GdkEvent *gev, gpointer data)
{
	XEvent    *ev  = (XEvent *)xev;
	MBTrayApp *app = (MBTrayApp*)data;
	Display *dpy = ev->xany.display;
	mb_tray_handle_xevent (app, ev); 
	return GDK_FILTER_CONTINUE;
}


int
main (int argc, char *argv[])
{
//	Display *dpy;
//	GtkWidget *window;
//	GdkBitmap *bitmap;
	GtkWidget *menu_remove;
	GtkWidget *menu_config;
	GtkWidget *menu_info;
	GtkTooltips *tooltips;
	GdkPixbuf *pixmap;

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

	app = mb_tray_app_new_with_display (_("Network Control"), NULL, paint_callback, &argc, &argv, GDK_DISPLAY() );
	  
/*	window = gtk_plug_new (0);
	gtk_widget_set_usize (window, 16, 16);
	gtk_widget_realize (window);

	gtk_window_set_title (GTK_WINDOW (window), _("Network Control"));
*/
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


	Pixbuf = mb_pixbuf_new(mb_tray_app_xdisplay(app), mb_tray_app_xscreen(app));
    pixmap = gpe_find_icon("net-on");	
	net_status_icons[ICON_NET_ON] = mb_pixbuf_img_new_from_data (Pixbuf, gdk_pixbuf_get_pixels(pixmap), 
		gdk_pixbuf_get_width(pixmap), gdk_pixbuf_get_height(pixmap), gdk_pixbuf_get_has_alpha(pixmap));
    pixmap = gpe_find_icon("net-off");	
	net_status_icons[ICON_NET_OFF] = mb_pixbuf_img_new_from_data (Pixbuf, gdk_pixbuf_get_pixels(pixmap), 
		gdk_pixbuf_get_width(pixmap), gdk_pixbuf_get_height(pixmap), gdk_pixbuf_get_has_alpha(pixmap));

	mb_tray_app_set_button_callback (app, clicked);
	mb_tray_app_main_init(app);

	gdk_window_add_filter (NULL, event_filter, (gpointer)app );

/*	mb_tray_app_main(app);	
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
*/
	gtk_main ();
	
	exit (0);
}
