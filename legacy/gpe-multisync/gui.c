/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "gpe_sync.h"

void
cancel_clicked (GtkWidget *w, GtkWidget *data)
{
  gtk_widget_destroy (data);
}

void
ok_clicked (GtkWidget *w, GtkWidget *data)
{
  gpe_conn *conn;

  conn = g_object_get_data (G_OBJECT (data), "conn");

  g_free (conn->username);
  g_free (conn->device_addr);

  w = g_object_get_data (G_OBJECT (data), "username");
  conn->username = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);

  w = g_object_get_data (G_OBJECT (data), "addr");
  conn->device_addr = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (w)->entry), 0, -1);

  gpe_save_config (conn);

  gtk_widget_destroy (data);
}

void
delete_window (GtkWidget *w)
{
  gpe_conn *conn;

  conn = g_object_get_data (G_OBJECT (w), "conn");
  g_free (conn->username);
  g_free (conn->device_addr);
  g_free (conn);

  sync_plugin_window_closed ();
}

GtkWidget* 
open_option_window (sync_pair *pair, connection_type type)
{
  GladeXML *xml;
  gchar *filename;
  GtkWidget *config_window;
  GtkWidget *w;
  gpe_conn *conn;

  filename = g_build_filename (PREFIX, "share", PACKAGE,
			       "gpe_sync.glade", NULL);

  xml = glade_xml_new (filename, NULL, NULL);

  g_free (filename);

  if (xml == NULL)
    return FALSE;

  conn = g_malloc0 (sizeof (*conn));

  conn->sync_pair = pair;

  config_window = glade_xml_get_widget (xml, "gpe_config");

  gpe_load_config (conn);

  g_object_set_data (G_OBJECT (config_window), "conn", conn);

  g_signal_connect (G_OBJECT (config_window), "destroy", G_CALLBACK (delete_window), NULL);

  w = glade_xml_get_widget (xml, "entry1");
  gtk_entry_set_text (GTK_ENTRY (w), conn->username);
  g_object_set_data (G_OBJECT (config_window), "username", w);

  w = glade_xml_get_widget (xml, "combo1");
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w)->entry), conn->device_addr);
  g_object_set_data (G_OBJECT (config_window), "addr", w);

  w = glade_xml_get_widget (xml, "cancelbutton1");
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (cancel_clicked), config_window);

  w = glade_xml_get_widget (xml, "okbutton1");
  g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (ok_clicked), config_window);

  g_object_unref (G_OBJECT (xml));

  return config_window;
}
