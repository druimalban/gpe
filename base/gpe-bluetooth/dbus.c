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

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <gpe/errorbox.h>

#define _(x) gettext(x)

#define PIN_SERVICE_NAME   "org.bluez.PinAgent"
#define PIN_INTERFACE_NAME PIN_SERVICE_NAME

#define OBEX_SERVICE_NAME   "org.handhelds.gpe.bluez"
#define OBEX_INTERFACE_NAME OBEX_SERVICE_NAME ".OBEX"

extern DBusHandlerResult bluez_pin_handle_dbus_request (DBusConnection *connection, DBusMessage *message);
extern DBusHandlerResult obex_client_handle_dbus_request (DBusConnection *connection, DBusMessage *message);

DBusHandlerResult
pin_handler_func (DBusConnection     *connection,
	      DBusMessage        *message,
	      void               *user_data)
{
  if (dbus_message_is_method_call (message, PIN_INTERFACE_NAME, "PinRequest"))
    return bluez_pin_handle_dbus_request (connection, message);
  
  if (dbus_message_is_signal (message,
                              DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL,
                              "Disconnected"))
    exit (0);
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusObjectPathVTable dbus_pin_vtable = {
  NULL,
  pin_handler_func,
  NULL
};

DBusHandlerResult
obex_handler_func (DBusConnection     *connection,
		   DBusMessage        *message,
		   void               *user_data)
{
  if (dbus_message_is_method_call (message, OBEX_INTERFACE_NAME, "ObjectPush"))
    return obex_client_handle_dbus_request (connection, message);
  
  if (dbus_message_is_signal (message,
                              DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL,
                              "Disconnected"))
    exit (0);
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusObjectPathVTable dbus_obex_vtable = {
  NULL,
  obex_handler_func,
  NULL
};

void
gpe_bluetooth_init_dbus (void)
{
  DBusConnection *connection;
  DBusError error;
  static const char *object_path1 = "/org/bluez/PinAgent"; 
  static const char *object_path2 = "/org/handhelds/gpe/bluez/OBEX";

  dbus_g_thread_init ();

  dbus_error_init (&error);
  connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection)
    {
      dbus_connection_setup_with_g_main (connection, NULL);

      dbus_connection_register_object_path (connection, object_path1, &dbus_pin_vtable, NULL);

      dbus_bus_acquire_service (connection, PIN_SERVICE_NAME, 0, &error);
      if (dbus_error_is_set (&error))
	{
	  gpe_error_box_fmt (_("Failed to acquire service: %s"), error.message);
	  dbus_error_free (&error);
	}
    }
  else
    {
      gpe_error_box_fmt (_("Failed to open connection to system message bus: %s"),
			 error.message);
      dbus_error_free (&error);
    }

  dbus_error_init (&error);
  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (connection)
    {
      dbus_connection_setup_with_g_main (connection, NULL);

      dbus_connection_register_object_path (connection, object_path2, &dbus_obex_vtable, NULL);

      dbus_bus_acquire_service (connection, OBEX_SERVICE_NAME, 0, &error);
      if (dbus_error_is_set (&error))
	{
	  gpe_error_box_fmt (_("Failed to acquire service: %s"), error.message);
	  dbus_error_free (&error);
	}
    }
  else
    {
      gpe_error_box_fmt (_("Failed to open connection to session message bus: %s"),
			 error.message);
      dbus_error_free (&error);
    }
}
