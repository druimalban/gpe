/*
 * gpe-conf
 *
 * Copyright (C) 2005  pHilipp Zabel <pzabel@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE usb settings module.
 *
 */

/*
  ToDo
  - Support for kernel 2.4?
  - Support for USB host?
  - file requestor
  - no instant apply
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>

enum {
	USB_MODE_ETHER,
	USB_MODE_STORAGE,
	USB_MODE_SERIAL
};

enum {
	STORAGE_MODE_CARD_READER,
	STORAGE_MODE_FILE_BACKED
};

char *gadget_module_name[] = { "g_ether", "g_file_storage", "g_serial", NULL };

struct usb_settings {
	int usb_mode;
	int storage_mode;
	int storage_allow_write;
	int serial_cdc_acm;
};

struct
{
	GtkWidget *storage_label;
	GtkWidget *storage_card_reader;
	GtkWidget *storage_file_backed;
	GtkWidget *storage_backing_file;
	GtkWidget *storage_allow_write;
	GtkWidget *serial_label;
	GtkWidget *serial_cdc_acm;

	struct usb_settings old;
	struct usb_settings new;
}
self;

/* gtk callbacks */

void
on_g_ether_toggled (GtkRadioButton *w, gpointer data)
{
	int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	if (active)
		self.new.usb_mode = USB_MODE_ETHER;
}

void
on_g_file_storage_toggled (GtkRadioButton *w, gpointer data)
{
	int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	gtk_widget_set_sensitive (self.storage_label, active);
	gtk_widget_set_sensitive (self.storage_card_reader, active);
	gtk_widget_set_sensitive (self.storage_file_backed, active);
	gtk_widget_set_sensitive (self.storage_backing_file, active &&
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
						(self.storage_file_backed)));

	if (active)
		self.new.usb_mode = USB_MODE_STORAGE;
}

void
on_g_serial_toggled (GtkRadioButton *w, gpointer data)
{
	int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	gtk_widget_set_sensitive (self.serial_label, active);
	gtk_widget_set_sensitive (self.serial_cdc_acm, active);

	if (active)
		self.new.usb_mode = USB_MODE_SERIAL;
}

void
on_storage_card_reader_toggled (GtkRadioButton *w, gpointer data)
{
	int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	if (active)
		self.new.storage_mode = STORAGE_MODE_CARD_READER;
}

void
on_storage_file_backed_toggled (GtkRadioButton *w, gpointer data)
{
	int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	gtk_widget_set_sensitive (self.storage_backing_file, active);

	if (active)
		self.new.storage_mode = STORAGE_MODE_FILE_BACKED;
}

void
on_serial_cdc_acm_toggled (GtkCheckButton *w, gpointer data)
{
	self.new.serial_cdc_acm = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
}

void
on_storage_allow_write_toggled (GtkCheckButton *w, gpointer data)
{
	self.new.storage_allow_write = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
}

/* gpe-conf applet interface */

void
USB_Save ()
{
	int do_rmmod_insmod = (self.new.usb_mode != self.old.usb_mode);
	gchar *buf, *file;

	if (!do_rmmod_insmod) {
		switch (self.new.usb_mode) {
		case USB_MODE_STORAGE:
			do_rmmod_insmod = (self.new.storage_mode != self.old.storage_mode);
			break;
		case USB_MODE_SERIAL:
			do_rmmod_insmod = (self.new.serial_cdc_acm != self.old.serial_cdc_acm);
			break;
		}
	}

	if (do_rmmod_insmod) {
		int ro = !self.new.storage_allow_write;
		suid_exec("RMOD", gadget_module_name[self.old.usb_mode]);
		switch (self.new.usb_mode) {
		case USB_MODE_ETHER:
			suid_exec("MODP", "g_ether");
			break;
		case USB_MODE_STORAGE:
			switch (self.new.storage_mode) {
			case STORAGE_MODE_CARD_READER:
				/* FIXME: this must not be hardcoded */
				buf = g_strdup_printf ("g_file_storage removable=1 "
					"file=/dev/hda,/dev/mmcblk0 ro=%d,%d", ro, ro);
				break;
			case STORAGE_MODE_FILE_BACKED:
				file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER
								(self.storage_backing_file));
				if (!file) return;
				buf = g_strdup_printf ("g_file_storage file=%s ro=%d", file, ro);
				break;
			}
			suid_exec("MODP", buf);
			g_free (buf);
			break;
		case USB_MODE_SERIAL:
			if (self.new.serial_cdc_acm)
				suid_exec("MODP", "g_serial use_acm=1");
			else
				suid_exec("MODP", "g_serial use_acm=0");
			break;
		}
	}
}

void
USB_Restore ()
{
}

GtkWidget *
USB_Build_Objects (void)
{
	GtkWidget *table;
	GtkWidget *tw;
	gchar *ts = NULL;
	gchar *err = NULL;
	
	/* init something */

	/*	- lsmod and check active usb mode */

	/* build gui */

	table = gtk_table_new(9, 3, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), gpe_get_border());
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());

	/* USB mode section */
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("USB Peripheral Mode"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 0, 1, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);

	tw = gtk_radio_button_new_with_label(NULL, _("Ethernet Networking"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 1, 2, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(tw), "toggled", 
	                       G_CALLBACK(on_g_ether_toggled), USB_MODE_ETHER);
	tw = gtk_radio_button_new_with_label_from_widget(tw, _("Storage"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 2, 3, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(tw), "toggled",
	                       G_CALLBACK(on_g_file_storage_toggled), USB_MODE_STORAGE);
	tw = gtk_radio_button_new_with_label_from_widget(tw, _("Serial Port"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 3, 4, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(tw), "toggled",
	                       G_CALLBACK(on_g_serial_toggled), USB_MODE_SERIAL);

	/* storage options section */ 
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Storage Behaviour"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 4, 5, 
	                 GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	self.storage_label = tw;
	tw = gtk_radio_button_new_with_label(NULL, _("CF/SD Card Reader"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 5, 6, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(tw), "toggled", 
	                       G_CALLBACK(on_storage_card_reader_toggled), NULL);
	self.storage_card_reader = tw;
	tw = gtk_radio_button_new_with_label_from_widget(tw, _("File-backed"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 1, 6, 7, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(tw), "toggled", 
	                       G_CALLBACK(on_storage_file_backed_toggled), NULL);
	self.storage_file_backed = tw;
	tw = gtk_file_chooser_button_new(_("backing file"), GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_table_attach(GTK_TABLE(table), tw, 1, 3, 6, 7, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	self.storage_backing_file = tw;
	tw = gtk_check_button_new_with_label(_("Allow write access"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 7, 8, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(tw), "toggled",
	                       G_CALLBACK(on_storage_allow_write_toggled), NULL);
	self.storage_allow_write = tw;

	/* serial options section */ 
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Serial Settings"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 8, 9, 
	                 GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	self.serial_label = tw;
	tw = gtk_check_button_new_with_label(_("Use CDC ACM"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 9, 10, GTK_FILL,
                     GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(tw), "toggled",
	                       G_CALLBACK(on_serial_cdc_acm_toggled), NULL);
	self.serial_cdc_acm = tw;

	gtk_widget_set_sensitive (self.storage_label, FALSE);
	gtk_widget_set_sensitive (self.storage_card_reader, FALSE);
	gtk_widget_set_sensitive (self.storage_file_backed, FALSE);
	gtk_widget_set_sensitive (self.storage_backing_file, FALSE);
	gtk_widget_set_sensitive (self.storage_allow_write, FALSE);
	gtk_widget_set_sensitive (self.serial_label, FALSE);
	gtk_widget_set_sensitive (self.serial_cdc_acm, FALSE);

	return table;
}
