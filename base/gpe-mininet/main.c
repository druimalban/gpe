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

#include "main.h"

#define _(x) gettext(x)

#define PIXMAP_SIZE  GTK_ICON_SIZE_DIALOG
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

static gboolean
net_get_status()
{
	FILE *pipe;
	char buffer[256];
	int result = FALSE;
	
	pipe = popen ("/bin/netstat -rn", "r");

	if (pipe > 0)
    {
  		while ((feof(pipe) == 0))
    	{
      		fgets (buffer, 255, pipe);
			if (g_str_has_prefix(buffer,"0.0.0.0") || g_str_has_prefix(buffer,"default"))
			  result = TRUE;
		}
		pclose(pipe);		
	}
	return result;
}


static void
update_netstatus (void)
{
	GdkBitmap *bitmap;
	gboolean oldstatus = net_is_on;
	
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
	}
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
cancel_dock_message (guint id)
{
	gdk_threads_enter ();
	gpe_system_tray_cancel_message (dock_window->window, id);
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
	GtkWidget *menu_config;
	GtkWidget *menu_info;
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

	gtk_timeout_add (2000, (GtkFunction) update_netstatus, NULL);

	gdk_threads_enter ();
	gtk_main ();
	gdk_threads_leave ();
	
	exit (0);
}
