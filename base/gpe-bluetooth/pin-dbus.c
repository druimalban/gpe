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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <bluetooth/bluetooth.h>

#define WRONG_ARGS_ERROR "org.handhelds.gpe.bluez.Error.WrongArgs"

struct pin_request_context
{
  DBusConnection *connection;
  DBusMessage *message;
};

extern void bluez_pin_request (struct pin_request_context *ctx, gboolean outgoing, const gchar *address, const gchar *name);

static void
bluez_pin_send_dbus_reply (DBusConnection *connection, DBusMessage *message, const char *pin)
{
  DBusMessageIter iter;

  dbus_message_append_iter_init (message, &iter);
  if (pin)
    dbus_message_iter_append_string (&iter, pin);

  dbus_connection_send (connection, message, NULL);
}

void
bluez_pin_response (struct pin_request_context *ctx, const char *pin)
{
  bluez_pin_send_dbus_reply (ctx->connection, ctx->message, pin);

  g_free (ctx);
}

void
bluez_pin_handle_dbus_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  gboolean out;
  bdaddr_t bdaddr, sbdaddr;
  int type;
  int i;
  char *address;
  DBusMessage *reply;
  struct pin_request_context *ctx;
  DBusError error;

  dbus_error_init (&error);
  dbus_message_iter_init (message, &iter);
 
  type = dbus_message_iter_get_arg_type (&iter);
  if (type != DBUS_TYPE_BOOLEAN)
  {
    reply = dbus_message_new_error_reply (message, WRONG_ARGS_ERROR,
		    			  "Boolean expected, other type given");
    goto error;
  }

  out = dbus_message_iter_get_boolean (&iter);

  for (i = 0; i < sizeof (bdaddr); i++)
    {
      unsigned char *p = (unsigned char *)&bdaddr;

      if (! dbus_message_iter_next (&iter))
	goto error;

      type = dbus_message_iter_get_arg_type (&iter);
      if (type != DBUS_TYPE_BYTE)
      {
        reply = dbus_message_new_error_reply (message, WRONG_ARGS_ERROR,
					      "Byte type expected, other type given");
	goto error;
      }

      p[i] = dbus_message_iter_get_byte (&iter);
    }

  reply = dbus_message_new_reply (message);

  ctx = g_malloc (sizeof (*ctx));
  ctx->message = reply;
  ctx->connection = connection;

  baswap (&sbdaddr, &bdaddr);
  address = batostr (&sbdaddr);

  bluez_pin_request (ctx, out, address, NULL);

  return;

 error:
  dbus_connection_send (connection, reply, NULL);
}
