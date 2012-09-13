/*
 * Copyright (C) 2002, 2003, 2004 Philip Blundell <philb@gnu.org>
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

#define SERVICE_NAME   "org.handhelds.gpe.irda"
#define OBEX_INTERFACE_NAME SERVICE_NAME ".OBEX"

extern DBusHandlerResult obex_client_handle_dbus_request (DBusConnection *connection, DBusMessage *message);

DBusHandlerResult
obex_handler_func (DBusConnection     *connection,
		   DBusMessage        *message,
		   void               *user_data)
{
  if (dbus_message_is_method_call (message, OBEX_INTERFACE_NAME, "ObjectPush"))
    return obex_client_handle_dbus_request (connection, message);
  
  if (dbus_message_is_signal (message,
                              DBUS_INTERFACE_LOCAL,
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
gpe_beam_init_dbus (void)
{
  DBusConnection *connection;
  DBusError error;
  static const char *object_path = "/org/handhelds/gpe/irda/OBEX";

  dbus_g_thread_init ();

  dbus_error_init (&error);
  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (connection)
    {
      dbus_connection_setup_with_g_main (connection, NULL);

      dbus_connection_register_object_path (connection, object_path, &dbus_obex_vtable, NULL);

      dbus_bus_request_name (connection, SERVICE_NAME, 0, &error);
      if (dbus_error_is_set (&error))
	{
	  gpe_error_box_fmt (_("Failed to request name: %s"), error.message);
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
