/*
 * This is gpe-beam, a simple IrDa frontend for GPE
 *
 * Copyright (c) 2003, 2004 Florian Boor <florian.boor@kernelconcepts.de>
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

#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>

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
#include "filesel.h"
#include "dbus.h"
#include "ircp/ircp.h"
#include "ircp/ircp_server.h"
#include "ircp/ircp_client.h"

#define _(x) gettext(x)

#define IR_INBOX "/tmp"

#define COMMAND_IR_ON  PREFIX "/bin/irsw on"
#define COMMAND_IR_OFF PREFIX "/bin/irsw off"
#define COMMAND_VCARD_IMPORT PREFIX "/bin/vcard-import < " IR_INBOX "/%s"
#define IR_DISCOVERY "/proc/net/irda/discovery"
#define IR_DISCOVERY_STATUS "/proc/sys/net/irda/discovery"
static const gchar *MY_VCARD;

struct gpe_icon my_icons[] = {
	{"irda-on16", PREFIX "/share/pixmaps/irda-on-16.png"},
	{"irda-off16", PREFIX "/share/pixmaps/irda-16.png"},
	{"irda-conn16", PREFIX "/share/pixmaps/irda-conn-16.png"},
	{"irda-on48", PREFIX "/share/pixmaps/irda-on-48.png"},
	{"irda-conn48", PREFIX "/share/pixmaps/irda-conn-48.png"},
	{"irda", PREFIX "/share/pixmaps/irda.png"},
	{NULL, NULL}
};

GtkWidget *window;
static GtkWidget *icon;
static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_vcard, *menu_control;
static GtkWidget *menu_send, *menu_receive;
static GtkWidget *control_window;
static GtkWidget *lDevice, *lCHint, *lSaddr;
static GThread *scan_thread;

static gboolean radio_is_on;
static gboolean have_peer = FALSE;
GdkWindow *dock_window;
static guint timeout_id = 0;
static GtkWidget *lStatus = NULL;
static GtkWidget *lTStatus = NULL;
static char *str_last_filename = NULL;
static GtkWidget *dlgStatus = NULL;

static void radio_on (void);
static void radio_off (void);
static ircp_client_t *cli = NULL;


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
	
	size = (sx > sy) ? sx : sy;
	if (size < 24)
		sbuf = gpe_find_icon (radio_is_on ? 
	    	                  (have_peer ? "irda-conn16" : "irda-on16") : "irda-off16");
	else
		sbuf = gpe_find_icon (radio_is_on ? 
	    	                  (have_peer ? "irda-conn48" : "irda-on48") : "irda");
	
	dbuf = gdk_pixbuf_scale_simple(sbuf, sx, sy, GDK_INTERP_HYPER);
	gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 60);
	gtk_widget_shape_combine_mask (GTK_WIDGET(window), NULL, 1, 0);
	gtk_widget_shape_combine_mask (GTK_WIDGET(window), bitmap, 1, 0);
	gdk_bitmap_unref (bitmap);
	gtk_image_set_from_pixbuf(GTK_IMAGE(icon), dbuf);
	gtk_widget_set_size_request(GTK_WIDGET(window), size, size);
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
	lStatus = label;

	return window;
}


void
ircp_info_cb (int event, char *param)
{
	char *ts;

	if (lStatus == NULL)
		return;

	gdk_threads_enter ();
	switch (event)
	{
	case IRCP_EV_ERRMSG:
		ts = g_strdup_printf ("%s: %s", _("Error"), param);
		gtk_label_set_text (GTK_LABEL (lStatus), ts);
		free (ts);
		break;
	case IRCP_EV_ERR:
		gtk_label_set_text (GTK_LABEL (lStatus), _("Failed"));
		break;
	case IRCP_EV_OK:
		gtk_label_set_text (GTK_LABEL (lStatus), _("Done"));
		break;
	case IRCP_EV_CONNECTING:
		gtk_label_set_text (GTK_LABEL (lStatus), _("Connecting..."));
		break;
	case IRCP_EV_DISCONNECTING:
		gtk_label_set_text (GTK_LABEL (lStatus),
				    _("Disconnecting..."));
		break;
	case IRCP_EV_SENDING:
		gtk_label_set_text (GTK_LABEL (lStatus), _("Sending..."));
		break;
	case IRCP_EV_RECEIVING:
		ts = g_strdup_printf ("%s: %s", _("Receiving"), param);
		gtk_label_set_text (GTK_LABEL (lStatus), ts);
		if (str_last_filename)
			free (str_last_filename);
		str_last_filename = g_strdup (param);
		free (ts);
		break;
	case IRCP_EV_LISTENING:
		gtk_label_set_text (GTK_LABEL (lStatus),
				    _("Waiting for incoming connection"));
		break;
	case IRCP_EV_CONNECTIND:
		gtk_label_set_text (GTK_LABEL (lStatus), _("Connected"));
		break;
	case IRCP_EV_DISCONNECTIND:
		gtk_label_set_text (GTK_LABEL (lStatus), _("Disconnecting"));
		break;
	}
	gdk_threads_leave ();
}


/*
 *    Function taken from irdadump by Dag Brattli <dagb@cs.uit.no>  
 *
 *    Parse and print the names of the various hint bits if they are set
 *
 */
char *
parse_hints (int hint)
{
	char *str = g_strdup ("");
	char *str1;

	if (hint & HINT_TELEPHONY)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("Telephony"));
		free (str);
		str = str1;
	}
	if (hint & HINT_FILE_SERVER)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("File Server"));
		free (str);
		str = str1;
	}
	if (hint & HINT_COMM)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("IrCOMM"));
		free (str);
		str = str1;
	}
	if (hint & HINT_OBEX)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("IrOBEX"));
		free (str);
		str = str1;
	}

	hint >>= 8;

	if (hint & HINT_PNP)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("PnP"));
		free (str);
		str = str1;
	}
	if (hint & HINT_PDA)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("PDA/Palmtop"));
		free (str);
		str = str1;
	}
	if (hint & HINT_COMPUTER)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("Computer"));
		free (str);
		str = str1;
	}
	if (hint & HINT_PRINTER)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("Printer"));
		free (str);
		str = str1;
	}
	if (hint & HINT_MODEM)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("Modem"));
		free (str);
		str = str1;
	}
	if (hint & HINT_FAX)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("Fax"));
		free (str);
		str = str1;
	}
	if (hint & HINT_LAN)
	{
		str1 = g_strdup_printf ("%s%s\n", str, _("LAN Access"));
		free (str);
		str = str1;
	}

	if (strlen (str))
		str[strlen (str) - 1] = 0;

	return str;
}


static int
vparse (FILE * f, char *format, va_list ap)
{
	char buffer[256];
	int ret = 0;
	fgets (buffer, 255, f);
	while ((feof (f) == 0))
	{
		if ((ret = vsscanf (buffer, format, ap)) > 0)
		{
			return ret;
		}
		fgets (buffer, 255, f);
	}
	return 0;
}


static int
parse_file (char *file, char *format, ...)
{
	va_list ap;
	FILE *f;
	int rv = 1;
	va_start (ap, format);

	f = fopen (file, "r");

	if (f > 0)
	{
		rv = vparse (f, format, ap);
		fclose (f);
	}
	va_end (ap);
	return rv;
}


static int
irda_is_on ()
{
	int result = FALSE;
	parse_file (IR_DISCOVERY_STATUS, "%d", &result);
	return result;
}


static void
exec_file_tx (char *filename)
{
	char *ts;

	if (cli == NULL)
		return;
	gdk_threads_enter ();
	gtk_window_present (GTK_WINDOW (dlgStatus));
	gdk_threads_leave ();
	if (ircp_cli_connect (cli) >= 0)
	{
		ircp_put (cli, filename);
		ircp_cli_disconnect (cli);
		gdk_threads_enter ();
		ts = g_strdup_printf ("%s %s", _("Transmitted file"),
				      filename);
		if (lTStatus) gtk_label_set_text (GTK_LABEL (lTStatus), ts);
		free (ts);
		gdk_threads_leave ();
	}
	else
	{
		gdk_threads_enter ();
		ts = g_strdup_printf ("%s %s", _("Could not transmit file"),
				      filename);
		if (lTStatus) gtk_label_set_text (GTK_LABEL (lTStatus), ts);
		free (ts);
		gdk_threads_leave ();
	}
	ircp_cli_close (cli);
	cli = NULL;

	sleep (3);
	gdk_threads_enter ();
	free (filename);
	gtk_widget_destroy (dlgStatus);
	lStatus = NULL;
	gdk_threads_leave ();
}


void
tx_file_select (const gchar *filename, gpointer data)
{
	scan_thread =
		g_thread_create ((GThreadFunc) exec_file_tx,
				 strdup (filename), FALSE, NULL);

	if (scan_thread == NULL)
		gpe_perror_box (_("Unable to start file transmit."));
}


void
send_data (char *filename, char *data, size_t len)
{
	int tmpfile;
	char *fn = g_strdup_printf("/tmp/%s", filename);

	tmpfile = open(fn, O_CREAT | O_WRONLY);
	if (tmpfile >= 0)
	{
		write(tmpfile, data, len);
		close(tmpfile);
		
		if (!radio_is_on)
			radio_on ();

		cli = ircp_cli_open (ircp_info_cb);

		gdk_threads_enter ();
		dlgStatus =
			bt_progress_dialog (_("IR Receive and Transmit....."),
						gpe_find_icon ("irda"));
		gtk_widget_show_all (dlgStatus);
		gdk_threads_leave ();
	
		if (cli != NULL)
		{
			tx_file_select (fn, NULL);
		}
	}
}


void
exec_file_abort ()
{
	gtk_widget_destroy (dlgStatus);
	lStatus = NULL;
}


void
tx_file_cancel (gpointer data)
{
	scan_thread =
		g_thread_create ((GThreadFunc) exec_file_abort, NULL, FALSE,
				 NULL);
	if (scan_thread == NULL)
		gpe_perror_box (_("Unable to start file transmit."));
	
}


static void
do_send_file (void)
{
	if (!radio_is_on)
		radio_on ();

	cli = ircp_cli_open (ircp_info_cb);

	gdk_threads_enter ();
	dlgStatus =
		bt_progress_dialog (_("IR Receive and Transmit....."),
				    gpe_find_icon ("irda"));
	gtk_widget_show_all (dlgStatus);
	gdk_threads_leave ();

	if (cli == NULL)
	{
		gdk_threads_enter ();
		gtk_label_set_text (GTK_LABEL (lStatus),
				    _("Error opening IR client."));
		gtk_label_set_text (GTK_LABEL (lTStatus),
				    _("Error opening IR client."));
		gdk_threads_leave ();
	}
	else
	{
		gdk_threads_enter();
		ask_user_a_file (getenv ("HOME"),
				 _("Select file to transmit"), tx_file_select,
				 tx_file_cancel, NULL);
		gdk_threads_leave();
	}
}


static void
do_import_file(GtkWidget *dlg,gint response,char *filename)
{
	char *imp;
	
	if (response == GTK_RESPONSE_YES)
	{
		gtk_widget_destroy (dlg);
		imp = g_strdup_printf
			(COMMAND_VCARD_IMPORT, filename);
		switch (system (imp))
		{
		case -1:
			dlg = gtk_message_dialog_new (GTK_WINDOW(control_window),
							  GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_ERROR,
							  GTK_BUTTONS_CLOSE,
							  _("Could not start import tool."));
			break;
		case 0:
			dlg = gtk_message_dialog_new (GTK_WINDOW(control_window),
							  GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_INFO,
							  GTK_BUTTONS_CLOSE,
							  _("vCard was imported successfully."));
			break;
		default:
			dlg = gtk_message_dialog_new
				(GTK_WINDOW(control_window),
				 GTK_DIALOG_DESTROY_WITH_PARENT,
				 GTK_MESSAGE_ERROR,
				 GTK_BUTTONS_CLOSE,
				 _("Import of vCard failed."));
			break;
		}
		gtk_dialog_run (GTK_DIALOG (dlg));
		gtk_widget_destroy (dlg);

		g_free (imp);
	}
	else
	{
		gtk_widget_destroy (dlg);
	}
}


static void
check_file_import (char *filename)
{
	GtkWidget *dlg;

	/* ultra simple check for a vcf file */
	if (strstr (filename, "vcf"))
	{
		gdk_threads_enter ();
		
		dlg = gtk_message_dialog_new (GTK_WINDOW(control_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
					      GTK_MESSAGE_QUESTION,
					      GTK_BUTTONS_YES_NO,
					      _("File '%s' looks like a vCard, "
					       "do you want to import it?"), filename);
		
		g_signal_connect_after (GTK_OBJECT (dlg), "response",
                           G_CALLBACK (do_import_file),
                           filename);
		gtk_widget_show_all(dlg);

		gdk_threads_leave ();
	}
}


static void
send_file (void)
{
	scan_thread =
		g_thread_create ((GThreadFunc)
				 do_send_file, NULL, FALSE, NULL);
	if (scan_thread == NULL)
		gpe_perror_box (_("Unable to start receiver."));
}


static void
do_receive_file (void)
{
	ircp_server_t *srv = NULL;
	char *ts;
	if (!radio_is_on)
		radio_on ();
	gdk_threads_enter ();
	dlgStatus =
		bt_progress_dialog (_
				    ("IR Receive and transmit control"),
				    gpe_find_icon ("irda"));
	gtk_widget_show_all (dlgStatus);
	gtk_window_present (GTK_WINDOW (dlgStatus));
	gdk_threads_leave ();
	srv = ircp_srv_open (ircp_info_cb);
	if (srv == NULL)
	{
		gdk_threads_enter ();
		gtk_label_set_text (GTK_LABEL
				    (lStatus), _("Error opening IR server."));
		gtk_label_set_text (GTK_LABEL
				    (lTStatus),
				    _("Error opening IR server."));
		gdk_threads_leave ();
	}
	else
	{
		ircp_srv_recv (srv, IR_INBOX);
		ircp_srv_close (srv);
		
		if (str_last_filename)
		{
			gdk_threads_enter ();
			ts = g_strdup_printf ("%s %s",
					      _("Received file"),
					      str_last_filename);
			if (lTStatus) gtk_label_set_text (GTK_LABEL (lTStatus), ts);
			free (ts);
			gdk_threads_leave ();
			check_file_import (str_last_filename);
		}
	}
	sleep (3);
	gdk_threads_enter ();
	gtk_widget_destroy (dlgStatus);
	lStatus = NULL;
	gdk_threads_leave ();
}


static void
receive_file (void)
{
	scan_thread =
		g_thread_create ((GThreadFunc)
				 do_receive_file, NULL, FALSE, NULL);
	if (scan_thread == NULL)
		gpe_perror_box (_("Unable to start receiver."));
}


static void
do_send_vcard (void)
{
	if (!radio_is_on)
		radio_on ();

	cli = ircp_cli_open (ircp_info_cb);
	if (cli)
	{
		gdk_threads_enter ();
		dlgStatus =
			bt_progress_dialog (_("Transmitting VCard"),
			                    gpe_find_icon ("irda"));
		gtk_widget_show_all (dlgStatus);
		gdk_threads_leave ();
	
		tx_file_select (MY_VCARD, NULL);
	}
}

static void
send_vcard (void)
{
	scan_thread =
		g_thread_create ((GThreadFunc)
				 do_send_vcard, NULL, FALSE, NULL);
	if (scan_thread == NULL)
		gpe_perror_box (_("Unable to start sender."));
}


static gboolean
get_irstatus (void)
{
	static char nick[32];
	static char nick2[32];
	static unsigned int chint = 0;
	unsigned long long saddr = 0;
	unsigned long long daddr = 0;
	char *ts;
	int success = FALSE;
	if (parse_file (IR_DISCOVERY,
			"nickname: %s hint: 0x%4x, saddr: 0x%8llx, daddr: 0x%8llx",
			nick, &chint, &saddr, &daddr) == 4)
		success = TRUE;
	else if (parse_file (IR_DISCOVERY,
			     "nickname: %s %s hint: 0x%4x, saddr: 0x%8llx, daddr: 0x%8llx",
			     nick, nick2, &chint, &saddr, &daddr) == 5)
	{
		success = TRUE;
		snprintf (nick + strlen (nick),
			  32 - strlen (nick), " %s", nick2);
	}

	if (success)
	{
		if (control_window)
		{
			if (strlen (nick))
				nick[strlen (nick) - 1] = 0;
			gtk_label_set_text (GTK_LABEL (lDevice), nick);
			snprintf (nick, 32, "0x%8llx", saddr);
			gtk_label_set_text (GTK_LABEL (lSaddr), nick);
			ts = parse_hints (chint);
			gtk_label_set_text (GTK_LABEL (lCHint), ts);
			free (ts);
		}
	}
	else
	{
		if (control_window)
		{
			gtk_label_set_text (GTK_LABEL (lDevice), "");
			gtk_label_set_text (GTK_LABEL (lSaddr), "");
			gtk_label_set_text (GTK_LABEL (lCHint), "");
		}
	}
	if (have_peer != success)
	{
		have_peer = success;
		set_image(0, 0);
	}
	return TRUE;
}


static gboolean
control_window_destroyed (void)
{
	control_window = NULL;
	lTStatus = NULL;
	radio_off ();

	return FALSE;
}


static void
show_control (void)
{
	GtkWidget *img;
	GtkWidget *tw, *hb;
	gchar *str;
	
	if (control_window == NULL)
	{
		GtkWidget *hsep = gtk_hseparator_new ();
		GtkWidget *sw = gtk_scrolled_window_new (NULL,
							 NULL);
		GtkWidget *vbox = gtk_vbox_new (FALSE,
						gpe_get_boxspacing ());
		GtkWidget *hbox = gtk_hbutton_box_new ();
		GtkWidget *bSend = gtk_button_new ();
		GtkWidget *bReceive = gtk_button_new ();
		GtkWidget *bClose =
			gtk_button_new_from_stock (GTK_STOCK_CLOSE);
		GtkWidget *table = gtk_table_new (6, 3, FALSE);
		GtkWidget *lcPeer = gtk_label_new (NULL);
		GtkWidget *lcActions = gtk_label_new (NULL);
		GtkWidget *l1 = gtk_label_new (_("Name:"));
		GtkWidget *l2 = gtk_label_new (_("Services:"));
		GtkWidget *l3 = gtk_label_new (_("Address:"));
		lDevice = gtk_label_new (NULL);
		gtk_misc_set_alignment (GTK_MISC (lDevice), 0, 0.5);
		lCHint = gtk_label_new (NULL);
		gtk_misc_set_alignment (GTK_MISC (lCHint), 0, 0.0);
		lSaddr = gtk_label_new (NULL);
		gtk_misc_set_alignment (GTK_MISC (lSaddr), 0, 0.5);
		str = g_strdup_printf("<b>%s</b>",_("Peer Information"));
		gtk_label_set_markup (GTK_LABEL(lcPeer), str);
		g_free(str);			  
		str = g_strdup_printf("<b>%s</b>",_("Peer Actions"));
		gtk_label_set_markup (GTK_LABEL
				      (lcActions), str);
		g_free(str);			  
		hb = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
		img = gtk_image_new_from_stock
			(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
		gtk_container_add (GTK_CONTAINER (hb), img);
		tw = gtk_label_new (_("Send File"));
		gtk_misc_set_alignment (GTK_MISC (tw), 0, 0.5);
		gtk_container_add (GTK_CONTAINER (hb), tw);
		gtk_container_add (GTK_CONTAINER (bSend), hb);
		hb = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
		img = gtk_image_new_from_stock
			(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
		gtk_container_add (GTK_CONTAINER (hb), img);
		tw = gtk_label_new (_("Receive File"));
		gtk_misc_set_alignment (GTK_MISC (tw), 0, 0.5);
		gtk_container_add (GTK_CONTAINER (hb), tw);
		gtk_container_add (GTK_CONTAINER (bReceive), hb);
		gtk_box_pack_start (GTK_BOX (hbox), bSend, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), bReceive, FALSE, TRUE, 0);
		gtk_misc_set_alignment (GTK_MISC (lcPeer), 0, 0.5);
		gtk_misc_set_alignment (GTK_MISC (lcActions), 0, 0.5);
		gtk_misc_set_alignment (GTK_MISC (l1), 0, 0.5);
		gtk_misc_set_alignment (GTK_MISC (l2), 0, 0.0);
		gtk_misc_set_alignment (GTK_MISC (l3), 0, 0.5);
		gtk_table_attach (GTK_TABLE
				  (table),
				  lcPeer, 0, 3,
				  0, 1,
				  GTK_FILL |
				  GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table), l1,
				  0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table),
				  lDevice, 1, 2,
				  1, 2, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table), l3,
				  0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table),
				  lSaddr, 1, 2,
				  2, 3, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table), l2,
				  0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table),
				  lCHint, 1, 2,
				  3, 4,
				  GTK_FILL |
				  GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table),
				  lcActions, 0,
				  3, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE
				  (table), hbox,
				  0, 3, 5, 6,
				  GTK_FILL |
				  GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
		if (radio_is_on)
			lTStatus = gtk_label_new (_("IR transceiver on"));
		else
			lTStatus = gtk_label_new (_("IR transceiver off"));
		gtk_label_set_line_wrap (GTK_LABEL (lTStatus), TRUE);
		gtk_table_attach (GTK_TABLE
				  (table),
				  lTStatus, 0,
				  3, 6, 7,
				  GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
		hbox = gtk_hbutton_box_new ();
		control_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW
				      (control_window),
				      _("Infrared Control"));
		gpe_set_window_icon (control_window, "irda");
		gtk_container_set_border_width
			(GTK_CONTAINER (vbox), gpe_get_border ());
		gtk_table_set_row_spacings
			(GTK_TABLE (table), gpe_get_boxspacing ());
		gtk_table_set_col_spacings
			(GTK_TABLE (table), gpe_get_boxspacing ());
		gtk_scrolled_window_set_policy
			(GTK_SCROLLED_WINDOW (sw),
			 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport
			(GTK_SCROLLED_WINDOW (sw), vbox);
		gtk_container_add (GTK_CONTAINER (control_window), sw);
		gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX
				    (vbox),
				    hsep, FALSE, TRUE, gpe_get_boxspacing ());
		gtk_box_pack_start (GTK_BOX (hbox), bClose, FALSE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
		gtk_widget_realize (control_window);
		gtk_widget_show_all (control_window);
		g_signal_connect (G_OBJECT
				  (bSend),
				  "clicked",
				  G_CALLBACK (send_file), control_window);
		g_signal_connect (G_OBJECT
				  (bReceive),
				  "clicked",
				  G_CALLBACK (receive_file), control_window);
		g_signal_connect_swapped
			(G_OBJECT (bClose), "clicked",
			 G_CALLBACK (gtk_widget_destroy), control_window);
		g_signal_connect (G_OBJECT
				  (control_window),
				  "destroy",
				  G_CALLBACK
				  (control_window_destroyed), NULL);
	}
}


static void
radio_on (void)
{
	gtk_widget_hide (menu_radio_on);
	gtk_widget_show (menu_radio_off);
	gtk_widget_set_sensitive (menu_vcard, TRUE);
	system (COMMAND_IR_ON);
	radio_is_on = TRUE;
	set_image(0, 0);
	if (lTStatus)
		gtk_label_set_text (GTK_LABEL(lTStatus), _("IR transceiver on"));
	timeout_id = gtk_timeout_add (1000,
	                              (GtkFunction) get_irstatus, NULL);
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
	do_stop_radio ();
	set_image(0, 0);
	if (lTStatus)
		gtk_label_set_text (GTK_LABEL(lTStatus), 
	                        _("IR transceiver off"));
	/* stop updates from scanner */
	if (timeout_id)
	{
		gtk_timeout_remove (timeout_id);
		timeout_id = 0;
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
	g_timeout_add (time,
		       (GSourceFunc) cancel_dock_message, (gpointer) id);
}


static void
clicked (GtkWidget * w, GdkEventButton * ev)
{
	gtk_menu_popup (GTK_MENU (menu),
			NULL, NULL,
			gpe_popup_menu_position, w, ev->button, ev->time);
}


/* handle resizing */
gboolean 
external_event(GtkWindow *window, GdkEventConfigure *event, gpointer user_data)
{
	if (event->type == GDK_CONFIGURE)
	{
		set_image(event->width, event->height);
	}
	return FALSE;
}

int
main (int argc, char *argv[])
{
	Display *dpy;
	GtkWidget *menu_remove;
	GtkTooltips *tooltips;
	GdkBitmap *bitmap;
	GtkWidget *s1, *s2;
	
	MY_VCARD = g_strdup_printf("%s/.gpe/user.vcf", g_get_home_dir());
	
	g_thread_init (NULL);
	gdk_threads_init ();
	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);
	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);
	
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
	
	window = gtk_plug_new (0);
	gtk_widget_set_usize (window, 16, 16);
	gtk_widget_realize (window);
	gtk_window_set_title (GTK_WINDOW (window), _("IrDa tool"));
	menu = gtk_menu_new ();
	s1 = gtk_separator_menu_item_new();
	s2 = gtk_separator_menu_item_new();
	
	menu_radio_on = gtk_menu_item_new_with_label (_("Switch IR on"));
	menu_radio_off = gtk_menu_item_new_with_label (_("Switch IR off"));
	menu_vcard = gtk_menu_item_new_with_label (_("Send vCard"));
	menu_send = gtk_menu_item_new_with_label (_("Send File"));
	menu_receive = gtk_menu_item_new_with_label (_("Receive File"));
	menu_control = gtk_menu_item_new_with_label (_("Controls..."));
	menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));
	g_signal_connect (G_OBJECT
			  (menu_radio_on),
			  "activate", G_CALLBACK (radio_on), NULL);
	g_signal_connect (G_OBJECT
			  (menu_radio_off),
			  "activate", G_CALLBACK (radio_off), NULL);
	g_signal_connect (G_OBJECT
			  (menu_vcard),
			  "activate", G_CALLBACK (send_vcard), NULL);
	g_signal_connect (G_OBJECT
			  (menu_send),
			  "activate", G_CALLBACK (send_file), NULL);
	g_signal_connect (G_OBJECT
			  (menu_receive),
			  "activate", G_CALLBACK (receive_file), NULL);
	g_signal_connect (G_OBJECT
			  (menu_control),
			  "activate", G_CALLBACK (show_control), NULL);
	g_signal_connect (G_OBJECT
			  (menu_remove),
			  "activate", G_CALLBACK (gtk_main_quit), NULL);
	radio_is_on = irda_is_on ();
	
	if (access(MY_VCARD, R_OK))
		gtk_widget_set_sensitive (menu_vcard, FALSE);
	else
		gtk_widget_set_sensitive (menu_vcard, TRUE);

	if (radio_is_on)
		gtk_widget_show (menu_radio_off);
	else
		gtk_widget_show (menu_radio_on);
	
	gtk_widget_show (menu_control);
	gtk_widget_show (menu_vcard);
	gtk_widget_show (menu_remove);
	gtk_widget_show (menu_send);
	gtk_widget_show (menu_receive);
    gtk_widget_show (s1);
    gtk_widget_show (s2);

	gtk_menu_append (GTK_MENU (menu), menu_radio_on);
	gtk_menu_append (GTK_MENU (menu), menu_radio_off);

	gtk_menu_append (GTK_MENU (menu), s1);
	
	gtk_menu_append (GTK_MENU (menu), menu_vcard);
	gtk_menu_append (GTK_MENU (menu), menu_send);
	gtk_menu_append (GTK_MENU (menu), menu_receive);

	gtk_menu_append (GTK_MENU (menu), s2);
	
	gtk_menu_append (GTK_MENU (menu), menu_control);
	gtk_menu_append (GTK_MENU (menu), menu_remove);
	
	icon = gtk_image_new_from_pixbuf
		(gpe_find_icon (radio_is_on ? "irda-on16" : "irda-off16"));
	gdk_pixbuf_render_pixmap_and_mask
		(gpe_find_icon ("irda-on16"), NULL, &bitmap, 255);
	gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
	gdk_bitmap_unref (bitmap);
	gtk_widget_show (icon);
	gpe_set_window_icon (window, "irda");
	tooltips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips),
		                  window,
		                  _("This is the infrared communications control."), 
	                      NULL);
	gtk_object_set_data(GTK_OBJECT(window),"tooltips",tooltips);
	g_signal_connect (G_OBJECT(window),
			          "button-press-event", G_CALLBACK (clicked), NULL);
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);
    g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (external_event), NULL);
	gtk_container_add (GTK_CONTAINER (window), icon);
	dpy = GDK_WINDOW_XDISPLAY (window->window);
	gtk_widget_show (window);
	atexit (do_stop_radio);
	dock_window = window->window;
	gpe_system_tray_dock (window->window);
	
	gdk_threads_enter ();
	gpe_beam_init_dbus();
	gtk_main ();
	gdk_threads_leave ();
	
	exit (0);
}
