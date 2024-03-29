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

#ifdef USE_HILDON
#define DBUS_API_SUBJECT_TO_CHANGE 1
#endif
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <gpe/errorbox.h>

#include "bluetooth.h"

static DBusConnection *connection;

#define BLUETOOTH_SERVICE_NAME   "org.handhelds.gpe.bluez"
#define IRDA_SERVICE_NAME	 "org.handhelds.gpe.irda"

#define MAX_SIZE	(4 * 1024 * 1024)	/* 4MB */

#define _(x) gettext(x)

static void
do_send_file (const gchar *service, const gchar *path, const gchar *method,
	      const gchar *uri, GnomeVFSFileInfo *info)
{
  DBusMessage *message;
  gchar *data;
  GnomeVFSHandle *handle;
  GnomeVFSResult r;
  GnomeVFSFileSize real_size;
  gchar *filename, *mimetype;

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

  message = dbus_message_new_method_call (service, path, method, "ObjectPush");

  filename = basename (uri);
  mimetype = info->mime_type ? info->mime_type : "application/data";
   
#ifdef DBUS_INTERFACE_LOCAL
  dbus_message_append_args (message, DBUS_TYPE_STRING, &filename,
			    DBUS_TYPE_STRING, &mimetype,
			    DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, 
			    &data, real_size, DBUS_TYPE_INVALID);
#else
  DBusMessageIter iter;

  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, filename);
  dbus_message_iter_append_string (&iter, mimetype);
  dbus_message_iter_append_byte_array (&iter, data, real_size);
#endif

  g_free (data);

  dbus_connection_send (connection, message, NULL);
}

gboolean
bluetooth_available (void)
{
  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

#ifdef DBUS_INTERFACE_LOCAL
  r = dbus_bus_name_has_owner (connection, BLUETOOTH_SERVICE_NAME, NULL);
#else
  r = dbus_bus_service_exists (connection, BLUETOOTH_SERVICE_NAME, NULL);
#endif

  return r ? TRUE : FALSE;
}

void
bluetooth_send_file (const gchar *uri, GnomeVFSFileInfo *info)
{
  do_send_file (BLUETOOTH_SERVICE_NAME,
		"/org/handhelds/gpe/bluez/OBEX",
		BLUETOOTH_SERVICE_NAME ".OBEX",
		uri, info);
}

void
irda_send_file (const gchar *uri, GnomeVFSFileInfo *info)
{
  do_send_file (IRDA_SERVICE_NAME,
		"/org/handhelds/gpe/irda/OBEX",
		IRDA_SERVICE_NAME ".OBEX",
		uri, info);
}

gboolean
irda_available (void)
{
  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

#ifdef DBUS_INTERFACE_LOCAL
  r = dbus_bus_name_has_owner (connection, IRDA_SERVICE_NAME, NULL);
#else
  r = dbus_bus_service_exists (connection, IRDA_SERVICE_NAME, NULL);
#endif

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
