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
#include <glade/glade.h>

#ifndef GPE_BLUETOOTH
#include <gconf/gconf-client.h>
#endif

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#ifdef GPE_BLUETOOTH
#include <gpe/pixmaps.h>
#endif

#define BT_ICON PREFIX "/share/pixmaps/bt-logo.png"

#define _(x) gettext(x)

struct pin_request_context;

extern void bluez_pin_response (struct pin_request_context *ctx, const char *pin);

static void
click_cancel (GtkWidget *widget, GtkWidget *window)
{
  struct pin_request_context *req;

  req = g_object_get_data (G_OBJECT (window), "context");

  if (req)
    bluez_pin_response (req, NULL);
  else
    {
      printf ("ERR\n");
      gtk_main_quit ();
    }

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

  if (req)
    bluez_pin_response (req, pin);
  else
    {
      printf ("PIN:%s\n", pin);
      gtk_main_quit ();
    }

  gtk_widget_destroy (window);
}

static const char *
bluez_pin_guess_name (const char *address)
{
#ifndef GPE_BLUETOOTH
  const char *name;
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

void
bluez_pin_request (struct pin_request_context *ctx, gboolean outgoing, const gchar *address, const gchar *name)
{
  GladeXML *xml;
  char *filename;
  GtkWidget *window, *logo, *entry, *label;
  GdkPixbuf *pixbuf;
  GtkWidget *buttonok, *buttoncancel;
  char *text;

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
    {
      printf ("ERR\n");
      gtk_main_quit ();
      exit (1);
    }

  window = glade_xml_get_widget (xml, "dialog1");
  logo = glade_xml_get_widget (xml, "image1");
  entry = glade_xml_get_widget (xml, "entry1");
  label = glade_xml_get_widget (xml, "label1");
  buttonok = glade_xml_get_widget (xml, "okbutton1");
  buttoncancel = glade_xml_get_widget (xml, "cancelbutton1");

  text = g_strdup_printf (outgoing ? _("Outgoing connection to %s")
		  : _("Incoming connection from %s"),
		  bluez_pin_guess_name (address));
  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);

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
		    G_CALLBACK (click_ok), window);
  g_signal_connect (G_OBJECT (buttoncancel), "clicked",
		    G_CALLBACK (click_cancel), window);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (click_ok), window);
  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (click_cancel), window);

  g_object_set_data (G_OBJECT (window), "entry", entry);
  g_object_set_data (G_OBJECT (window), "context", ctx);

  gtk_window_set_title (GTK_WINDOW (window), _("Bluetooth PIN"));

  gtk_widget_show_all (window);

  gtk_widget_grab_focus (entry);
}

