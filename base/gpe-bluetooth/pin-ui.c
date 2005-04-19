/*
 * Copyright (C) 2002, 2003, 2004, 2005 Philip Blundell <philb@gnu.org>
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
#include <glade/glade.h>

#ifndef GPE_BLUETOOTH
#include <gconf/gconf-client.h>
#endif

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#ifdef GPE_BLUETOOTH
#include <gpe/pixmaps.h>
#endif

#include "pin-ui.h"

#define BT_ICON PREFIX "/share/pixmaps/bt-logo.png"

#define _(x) gettext(x)

struct pin_request_context;

static guint my_signals[1];

static const char *
bluez_pin_guess_name (const char *address, const char *name)
{
  if (name != NULL && strlen (name) > 0)
    return name;
#ifndef GPE_BLUETOOTH
  GConfClient *gc;
  char *path;

  gc = gconf_client_get_default ();
  path = g_strdup_printf ("/system/bluetooth/device/%s/name", address);
  name = gconf_client_get_string (gc, path, NULL);
  g_free (path);
  if (name != NULL)
    return name;
#endif
  return address;
}

struct _BluetoothPinRequest
{
  GObject object;

  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *entry;
};

struct _BluetoothPinRequestClass
{
  GObjectClass parent_class;

  void (*result)(BluetoothPinRequest *req, const gchar *text);
};

static void
click_cancel (GtkWidget *widget, BluetoothPinRequest *req)
{
  g_signal_emit (G_OBJECT (req), my_signals[0], 0, NULL);

  gtk_widget_hide (req->window);
}

static void
click_ok (GtkWidget *widget, BluetoothPinRequest *req)
{
  const gchar *pin;

  pin = gtk_entry_get_text (GTK_ENTRY (req->entry));

  g_signal_emit (G_OBJECT (req), my_signals[0], 0, pin);

  gtk_widget_hide (req->window);
}

static void
bluetooth_pin_request_init (BluetoothPinRequest *req)
{
  GladeXML *xml;
  char *filename;
  GtkWidget *window, *logo, *entry, *label;
  GdkPixbuf *pixbuf;
  GtkWidget *buttonok, *buttoncancel;

#ifdef GPE_BLUETOOTH
  filename = g_build_filename (PREFIX, "/share/bluez-pin",
		  "bluez-pin-gpe.glade", NULL);
#else
  filename = g_build_filename (PREFIX, "/share/bluez-pin",
		  "bluez-pin.glade", NULL);
#endif

  xml = glade_xml_new (filename, NULL, NULL);

  g_free (filename);

  if (xml == NULL)
    abort ();
   
  window = glade_xml_get_widget (xml, "dialog1");
  logo = glade_xml_get_widget (xml, "image1");
  entry = glade_xml_get_widget (xml, "entry1");
  label = glade_xml_get_widget (xml, "label1");
  buttonok = glade_xml_get_widget (xml, "okbutton1");
  buttoncancel = glade_xml_get_widget (xml, "cancelbutton1");

#ifdef GPE_BLUETOOTH
  pixbuf = gpe_find_icon ("bt-logo");
#else 
  pixbuf = gdk_pixbuf_new_from_file (BT_ICON, NULL);
#endif
  if (pixbuf)
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (logo), pixbuf);
      g_object_unref (pixbuf);
    }

  g_signal_connect (G_OBJECT (buttonok), "clicked",
		    G_CALLBACK (click_ok), req);
  g_signal_connect (G_OBJECT (buttoncancel), "clicked",
		    G_CALLBACK (click_cancel), req);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (click_ok), req);
  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (click_cancel), req);

  req->label = label;
  req->entry = entry;
  req->window = window;

  gtk_window_set_title (GTK_WINDOW (window), _("Bluetooth PIN"));
}

void
bluetooth_pin_request_set_details (BluetoothPinRequest *req,
				   gboolean outgoing,
				   const gchar *address,
				   const gchar *name)
{
  gchar *text;

  text = g_strdup_printf (outgoing ? _("Outgoing connection to %s")
		  : _("Incoming connection from %s"),
		  bluez_pin_guess_name (address, name));
  gtk_label_set_text (GTK_LABEL (req->label), text);
  g_free (text);
}

static void
bluetooth_pin_request_class_init (BluetoothPinRequestClass * klass)
{
  GObjectClass *oclass;

  oclass = (GObjectClass *) klass;

  my_signals[0] = g_signal_new ("result",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _BluetoothPinRequestClass, result),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1,
				G_TYPE_STRING);
}

GType
bluetooth_pin_request_get_type (void)
{
  static GType bluetooth_pin_request_type = 0;

  if (! bluetooth_pin_request_type)
    {
      static const GTypeInfo bluetooth_pin_request_info =
      {
	sizeof (BluetoothPinRequestClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) bluetooth_pin_request_class_init,
	(GClassFinalizeFunc) NULL,
	NULL /* class_data */,
	sizeof (BluetoothPinRequest),
	0 /* n_preallocs */,
	(GInstanceInitFunc) bluetooth_pin_request_init,
      };

      bluetooth_pin_request_type = g_type_register_static (G_TYPE_OBJECT,
							   "BluetoothPinRequest", 
							   &bluetooth_pin_request_info, 
							   (GTypeFlags)0);

    }

  return bluetooth_pin_request_type;
}

BluetoothPinRequest *
bluetooth_pin_request_new (gboolean outgoing, const gchar *address, const gchar *name)
{
  BluetoothPinRequest *req;

  req = g_object_new (bluetooth_pin_request_get_type (), NULL);

  bluetooth_pin_request_set_details (req, outgoing, address, name);

  gtk_widget_show_all (req->window);

  gtk_widget_grab_focus (req->entry);

  return req;
}
