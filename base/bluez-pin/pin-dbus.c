/*
 * Copyright (C) 2002, 2003, 2005 Philip Blundell <philb@gnu.org>
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

#include "pin-ui.h"

#define WRONG_ARGS_ERROR "org.bluez.Error.WrongArgs"

struct pin_request_context
{
  DBusConnection *connection;
  DBusMessage *message;
};

extern void bluez_pin_request (struct pin_request_context *ctx, gboolean outgoing, const gchar *address, const gchar *name);

static void
bluez_pin_send_dbus_reply (DBusConnection *connection, DBusMessage *message, const char *pin)
{
#ifndef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  DBusMessageIter iter;

  dbus_message_append_iter_init (message, &iter);
  if (pin)
    dbus_message_iter_append_string (&iter, pin);
#else
  dbus_message_append_args(message,
			DBUS_TYPE_STRING, &pin,
			DBUS_TYPE_INVALID);
#endif

  dbus_connection_send (connection, message, NULL);

  dbus_message_unref (message);
}

void
dbus_pin_result (BluetoothPinRequest *req, gchar *result, void *user_data)
{
  struct pin_request_context *ctx = user_data;

  bluez_pin_send_dbus_reply (ctx->connection, ctx->message, result);

  g_free (ctx);

  g_object_unref (req);
}

DBusHandlerResult
bluez_pin_handle_dbus_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  gboolean out;
  bdaddr_t bdaddr, sbdaddr;
  int type;
  char *address;
  DBusMessage *reply;
  struct pin_request_context *ctx;
  unsigned char *bytes;
  int nbytes;
  BluetoothPinRequest *req;

  dbus_message_iter_init (message, &iter);
 
  type = dbus_message_iter_get_arg_type (&iter);
  if (type != DBUS_TYPE_BOOLEAN)
  {
    reply = dbus_message_new_error (message, WRONG_ARGS_ERROR,
				    "Boolean expected, other type given");
    goto error;
  }

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_iter_get_basic (&iter, &out);
#else
  out = dbus_message_iter_get_boolean (&iter);
#endif

  if (! dbus_message_iter_next (&iter))
    {
      reply = dbus_message_new_error (message, WRONG_ARGS_ERROR,
				      "Byte array expected but missing");
      goto error;
    }

  type = dbus_message_iter_get_arg_type (&iter);

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  nbytes = 0;
  if (type == DBUS_TYPE_ARRAY)
    dbus_message_iter_get_fixed_array (&iter, &bytes, &nbytes);
  if (nbytes != sizeof (bdaddr))
#else
  if (type != DBUS_TYPE_ARRAY
      || ! dbus_message_iter_get_byte_array (&iter, &bytes, &nbytes)
      || nbytes != sizeof (bdaddr))
#endif
  {
    reply = dbus_message_new_error (message, WRONG_ARGS_ERROR,
				    "Byte array expected, other type given");
    goto error;
  }

  memcpy (&bdaddr, bytes, sizeof (bdaddr));

  reply = dbus_message_new_method_return (message);
  if (!reply)
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  ctx = g_malloc (sizeof (*ctx));
  ctx->message = reply;
  ctx->connection = connection;

  baswap (&sbdaddr, &bdaddr);
  address = batostr (&sbdaddr);

  req = bluetooth_pin_request_new (out, address, NULL);

  g_signal_connect (G_OBJECT (req), "result", G_CALLBACK (dbus_pin_result), ctx);

  return DBUS_HANDLER_RESULT_HANDLED;

 error:
  dbus_connection_send (connection, reply, NULL);

  dbus_message_unref (reply);

  return DBUS_HANDLER_RESULT_HANDLED;
}
