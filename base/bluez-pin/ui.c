/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libintl.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define BT_ICON PREFIX "/share/pixmaps/bt-logo.png"

#define _(x) gettext(x)

struct pin_request_context;

static void send_reply (struct pin_request_context *ctx, const char *pin);

static void
click_cancel (GtkWidget *widget, GtkWidget *window)
{
  struct req_context *req = g_object_get_data (G_OBJECT (window), "context");
  
  bluez_pin_response (req, NULL);

  gtk_widget_destroy (window);
}

static void
click_ok (GtkWidget *widget, GtkWidget *window)
{
  struct pin_request_context *req = g_object_get_data (G_OBJECT (window), "context");
  GtkWidget *entry;
  const char *pin;

  entry = g_object_get_data (G_OBJECT (window), "entry");
  pin = gtk_entry_get_text (GTK_ENTRY (entry));

  bluez_pin_response (req, pin);

  gtk_widget_destroy (window);
}

gboolean
bluez_pin_request (struct pin_request_context *ctx, gboolean outgoing, const gchar *address, const gchar *name)
{
  GtkWidget *window;
  GtkWidget *logo = NULL;
  GtkWidget *text1, *text2, *text3, *hbox, *vbox;
  GtkWidget *hbox_pin;
  GtkWidget *pin_label, *entry;
  GtkWidget *buttonok, *buttoncancel;
  GdkPixbuf *pixbuf;

  const char *dir = outgoing ? _("Outgoing connection to") 
    : _("Incoming connection from");

  window = gtk_dialog_new ();
 
  pixbuf = gdk_pixbuf_new_from_file (BT_ICON, NULL);
  if (pixbuf)
    logo = gtk_image_new_from_pixbuf (pixbuf);

  pin_label = gtk_label_new (_("PIN:"));
  entry = gtk_entry_new ();

  hbox = gtk_hbox_new (FALSE, 4);
  vbox = gtk_vbox_new (FALSE, 0);

  hbox_pin = gtk_hbox_new (FALSE, 0);

  text1 = gtk_label_new (dir);
  text2 = gtk_label_new (address);

  gtk_box_pack_start (GTK_BOX (vbox), text1, TRUE, TRUE, 0);
  if (name)
    {
      text3 = gtk_label_new (name);
      gtk_box_pack_start (GTK_BOX (vbox), text3, TRUE, TRUE, 0);
    }
  gtk_box_pack_start (GTK_BOX (vbox), text2, TRUE, TRUE, 0);

  if (logo)
    gtk_box_pack_start (GTK_BOX (hbox), logo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox_pin), pin_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_pin), entry, TRUE, TRUE, 0);

  buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
  buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), buttonok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), buttoncancel, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (buttonok), "clicked",
		    G_CALLBACK (click_ok), window);
  g_signal_connect (G_OBJECT (buttoncancel), "clicked",
		    G_CALLBACK (click_cancel), window);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (click_ok), window);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox_pin, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (window), "entry", entry);
  g_object_set_data (G_OBJECT (window), "context", ctx);

  gtk_window_set_title (GTK_WINDOW (window), _("Bluetooth PIN"));

  gtk_widget_show_all (window);

  gtk_widget_grab_focus (entry);
}
