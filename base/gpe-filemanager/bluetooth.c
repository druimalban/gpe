/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/*
 * $Id$
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libgnomevfs/gnome-vfs.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <gpe/errorbox.h>

#include "bluetooth.h"

static DBusConnection *connection;

#define BLUETOOTH_SERVICE_NAME   "org.handhelds.gpe.bluez"

#define MAX_SIZE	(4 * 1024 * 1024)	/* 4MB */

#define _(x) gettext(x)

void
bluetooth_send_file (const gchar *uri, GnomeVFSFileInfo *info)
{
  DBusMessage *message;
  DBusMessageIter iter;
  gchar *data;
  GnomeVFSHandle *handle;
  GnomeVFSResult r;
  GnomeVFSFileSize real_size;

  if (info->size > MAX_SIZE)
    {
      gpe_error_box (_("This file is too large to beam"));
      return;
    }

  r = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
  if (r != GNOME_VFS_OK)
    {
      gpe_error_box (gnome_vfs_result_to_string (r));
    }

  data = g_malloc (info->size);

  gnome_vfs_read (handle, data, info->size, &real_size);

  gnome_vfs_close (handle);

  message = dbus_message_new_method_call (BLUETOOTH_SERVICE_NAME,
					  "/org/handhelds/gpe/bluez/OBEX",
					  BLUETOOTH_SERVICE_NAME ".OBEX",
					  "ObjectPush");

  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, basename (uri));
  dbus_message_iter_append_string (&iter, info->mime_type ? info->mime_type : "application/data");
  dbus_message_iter_append_byte_array (&iter, data, real_size);

  g_free (data);

  dbus_connection_send (connection, message, NULL);
}

gboolean
bluetooth_available (void)
{
  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

  r = dbus_bus_service_exists (connection, BLUETOOTH_SERVICE_NAME, NULL);

  return r ? TRUE : FALSE;
}

void
bluetooth_init (void)
{
  DBusError error;

  dbus_error_init (&error);

  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (connection)
    dbus_connection_setup_with_g_main (connection, NULL);
}
