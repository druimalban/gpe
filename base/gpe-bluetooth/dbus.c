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

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define REQUEST_NAME "org.handhelds.gpe.bluez.pin-request"

#define _(x) gettext(x)

#include "../bluez-pin/dbus.c"

DBusHandlerResult
dbus_handler_func (DBusMessageHandler *handler,
 	      DBusConnection     *connection,
	      DBusMessage        *message,
	      void               *user_data)
{
  if (dbus_message_has_name (message, REQUEST_NAME))
    bluez_pin_handle_dbus_request (connection, message);
  
  if (dbus_message_has_name (message, DBUS_MESSAGE_LOCAL_DISCONNECT))
    exit (0);
  
  return DBUS_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

void
gpe_bluetooth_init_dbus (void)
{
  DBusConnection *connection;
  DBusError error;
  DBusMessageHandler *handler;

  dbus_error_init (&error);
  connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection == NULL)
    {
      fprintf (stderr, "Failed to open connection to system message bus: %s\n",
               error.message);
      dbus_error_free (&error);
      exit (1);
    }

  dbus_connection_setup_with_g_main (connection, NULL);

  handler = dbus_message_handler_new (dbus_handler_func, NULL, NULL);
  dbus_connection_add_filter (connection, handler);
}
