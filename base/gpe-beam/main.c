/*
 * This is gpe-beam, a simple IrDa frontend for GPE
 *
 * Copyright (c) 2003, Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Based on gpe-bluetooth dockapp, see below... 
 *
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

//#include <sys/types.h>
#include <stdlib.h>
#include <libintl.h>

/*#include <locale.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
*/
#include <signal.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/tray.h>
#include <gpe/popup.h>
#include <stdarg.h>

#include <sys/socket.h>
#include "main.h"

#define _(x) gettext(x)

#define COMMAND_IR_ON  "ifconfig irda0 up ; echo 1 > /proc/sys/net/irda/discovery"
#define COMMAND_IR_OFF  "echo 0 > /proc/sys/net/irda/discovery ; ifconfig irda0 down"
#define IR_DISCOVERY "/proc/net/irda/discovery"

struct gpe_icon my_icons[] = {
	{"irda-on", "/usr/share/pixmaps/irda-on-16.png"},
	{"irda-off", "/usr/share/pixmaps/irda-16.png"},
	{"irda", "/usr/share/pixmaps/irda.png"},
	{NULL}
};

static GtkWidget *icon;


static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_vcard, *menu_control;
static GtkWidget *control_window;
static GtkWidget *lDevice, *lCHint;

gboolean radio_is_on;
GdkWindow *dock_window;
static guint timeout_id = 0;


static int vparse(FILE *f, char *format, va_list ap)
{
  char buffer[256];
  fgets (buffer, 255, f);
  while ((feof(f) == 0))
    {
      if ( vsscanf(buffer, format, ap) > 0)
	{
	  return 0;
	}
      fgets (buffer, 255, f);
    }
  return 1;
}


static int parse_file(char *file,char *format,...)
{
  va_list ap;
  FILE *f;
  int rv = 1;
  va_start(ap, format);

  f = fopen (file, "r");

  if (f > 0)
    {
      rv = vparse(f,format,ap);
      fclose (f);
    }
  va_end(ap);
  return rv;
}


static void
send_vcard (void)
{

}


static void
get_irstatus (void)
{
	char nick[32];
	int chint = 0;
	//unsigned long long 
	if (!parse_file(IR_DISCOVERY,"nickname: %s, hint: %ix, %*s %*s %*s %*s",nick,chint))
	{
		gtk_label_set_text(GTK_LABEL(lDevice),nick);
		snprintf(nick,32,"%ix",chint);
		gtk_label_set_text(GTK_LABEL(lCHint),nick);
	}		
}


static gboolean
control_window_destroyed (void)
{
	control_window = NULL;

	/* stop updates from scanner */
	if (timeout_id)
	{
		gtk_timeout_remove (timeout_id);
		timeout_id = 0;
	}

	return FALSE;
}


static void
show_control (void)
{
	if (control_window == NULL)
	{
		GtkWidget *hsep = gtk_hseparator_new ();
		GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
		GtkWidget *vbox = gtk_vbox_new (FALSE, gpe_get_boxspacing ());
		GtkWidget *hbox = gtk_hbutton_box_new ();
		GtkWidget *bOK = gtk_button_new_from_stock (GTK_STOCK_OK);
		GtkWidget *bCancel =
			gtk_button_new_from_stock (GTK_STOCK_CANCEL);
		GtkWidget *table = gtk_table_new (6, 3, FALSE);
		GtkTooltips *tooltips = gtk_tooltips_new ();

		lDevice = gtk_label_new(NULL);
		gtk_misc_set_alignment(GTK_MISC(lDevice),0,0.5);
		
		lCHint = gtk_label_new(NULL);
		gtk_misc_set_alignment(GTK_MISC(lCHint),0,0.5);
		
		gtk_table_attach(GTK_TABLE(table),lDevice,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
		gtk_table_attach(GTK_TABLE(table),lCHint,1,2,0,1,GTK_FILL,GTK_FILL,0,0);
		
		control_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

		gtk_window_set_title (GTK_WINDOW (control_window),
				      _("Infrared Control"));
		gpe_set_window_icon (control_window, "irda");


		gtk_container_set_border_width (GTK_CONTAINER (vbox),
						gpe_get_border ());
		gtk_table_set_row_spacings (GTK_TABLE (table),
					    gpe_get_boxspacing ());
		gtk_table_set_col_spacings (GTK_TABLE (table),
					    gpe_get_boxspacing ());

		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
						GTK_POLICY_NEVER,
						GTK_POLICY_AUTOMATIC);

		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW
						       (sw), vbox);
		gtk_container_add (GTK_CONTAINER (control_window), sw);

		gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, TRUE, 0);

		gtk_box_pack_start (GTK_BOX (hbox), bCancel, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), bOK, FALSE, TRUE, 0);

		gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

		gtk_widget_realize (control_window);
		gtk_widget_show_all (control_window);

/*		
	g_signal_connect (G_OBJECT (bOK), "clicked",
				  G_CALLBACK (ok_clicked), window);
	g_signal_connect_swapped (G_OBJECT (bCancel), "clicked",
				  G_CALLBACK (gtk_widget_destroy), window);
				*/
		g_signal_connect (G_OBJECT (control_window), "destroy",
				  G_CALLBACK (control_window_destroyed),
				  NULL);

		timeout_id =
			gtk_timeout_add (1000, (GtkFunction) get_irstatus,
					 NULL);
	}
}


static void
radio_on (void)
{
	sigset_t sigs;

	gtk_widget_hide (menu_radio_on);
	gtk_widget_show (menu_radio_off);
	gtk_widget_set_sensitive (menu_vcard, TRUE);

	system (COMMAND_IR_ON);

	gtk_image_set_from_pixbuf (GTK_IMAGE (icon),
				   gpe_find_icon ("irda-on"));
	radio_is_on = TRUE;
	sigemptyset (&sigs);
	sigaddset (&sigs, SIGCHLD);
	sigprocmask (SIG_BLOCK, &sigs, NULL);
//  hciattach_pid = fork_hciattach ();
	sigprocmask (SIG_UNBLOCK, &sigs, NULL);
}


static void
do_stop_radio (void)
{
	radio_is_on = FALSE;
	system (COMMAND_IR_OFF);
}


static void
radio_off (void)
{
	gtk_widget_hide (menu_radio_off);
	gtk_widget_show (menu_radio_on);
	gtk_widget_set_sensitive (menu_vcard, FALSE);

	gtk_image_set_from_pixbuf (GTK_IMAGE (icon),
				   gpe_find_icon ("irda-off"));

	do_stop_radio ();
}


static void
sigchld_handler (int sig)
{
/*  int status;
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
*/
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
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			gpe_popup_menu_position, w, ev->button, ev->time);
}


int
main (int argc, char *argv[])
{
	Display *dpy;
	GtkWidget *window;
	GtkWidget *menu_remove;
	GtkTooltips *tooltips;
	GdkBitmap *bitmap;

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

	gtk_window_set_title (GTK_WINDOW (window), _("IrDa control"));

	signal (SIGCHLD, sigchld_handler);

	menu = gtk_menu_new ();
	menu_radio_on = gtk_menu_item_new_with_label (_("Switch IR on"));
	menu_radio_off = gtk_menu_item_new_with_label (_("Switch IR off"));
	menu_vcard = gtk_menu_item_new_with_label (_("Send vcard"));
	menu_control = gtk_menu_item_new_with_label (_("Controls..."));
	menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));

	g_signal_connect (G_OBJECT (menu_radio_on), "activate",
			  G_CALLBACK (radio_on), NULL);
	g_signal_connect (G_OBJECT (menu_radio_off), "activate",
			  G_CALLBACK (radio_off), NULL);
	g_signal_connect (G_OBJECT (menu_vcard), "activate",
			  G_CALLBACK (send_vcard), NULL);
	g_signal_connect (G_OBJECT (menu_control), "activate",
			  G_CALLBACK (show_control), NULL);
	g_signal_connect (G_OBJECT (menu_remove), "activate",
			  G_CALLBACK (gtk_main_quit), NULL);

	if (!radio_is_on)
	{
		gtk_widget_set_sensitive (menu_vcard, FALSE);
		gtk_widget_show (menu_radio_on);
	}

	gtk_widget_show (menu_control);
	gtk_widget_show (menu_vcard);
	gtk_widget_show (menu_remove);

	gtk_menu_append (GTK_MENU (menu), menu_radio_on);
	gtk_menu_append (GTK_MENU (menu), menu_radio_off);
	gtk_menu_append (GTK_MENU (menu), menu_vcard);
	gtk_menu_append (GTK_MENU (menu), menu_control);
	gtk_menu_append (GTK_MENU (menu), menu_remove);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	icon = gtk_image_new_from_pixbuf (gpe_find_icon
					  (radio_is_on ? "irda-on" :
					   "irda-off"));
	gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("irda-on"), NULL,
					   &bitmap, 255);
	gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
	gdk_bitmap_unref (bitmap);
	gtk_widget_show (icon);

	gpe_set_window_icon (window, "irda");

	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window,
			      _
			      ("This is the infrared communications control."),
			      NULL);

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
