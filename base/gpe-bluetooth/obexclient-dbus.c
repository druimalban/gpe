/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <openobex/obex.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

#include "obexclient.h"
#include "obex-glib.h"
#include "main.h"
#include "sdp.h"

#define WRONG_ARGS_ERROR "org.handhelds.gpe.bluez.Error.WrongArgs"

#define _(x) gettext(x)

struct dbus_push_context
{
  gchar *filename;
  gchar *mimetype;
  gchar *data;

  DBusConnection *connection;
  DBusMessage *reply;
};

static void
push_done (gboolean result, struct dbus_push_context *ctx)
{
  dbus_free (ctx->filename);
  dbus_free (ctx->mimetype);
  dbus_free (ctx->data);

  dbus_connection_send (ctx->connection, ctx->reply, NULL);

  dbus_message_unref (ctx->reply);

  g_free (ctx);
}

DBusHandlerResult
obex_client_handle_dbus_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  DBusMessage *reply;
  gchar *filename = NULL, *mime_type = NULL;
  unsigned char *data = NULL;
  size_t len;
  struct dbus_push_context *ctx;

  dbus_message_iter_init (message, &iter);
 
  if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
    goto wrong_args;

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_iter_get_basic (&iter, &filename);
#else
  filename = dbus_message_iter_get_string (&iter);
#endif

  if (!dbus_message_iter_next (&iter))
    goto wrong_args;

  if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
    goto wrong_args;

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_iter_get_basic (&iter, &mime_type);
#else
  mime_type = dbus_message_iter_get_string (&iter);
#endif

  if (!dbus_message_iter_next (&iter))
    goto wrong_args;
  
  if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY
#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
      || dbus_message_iter_get_element_type (&iter) != DBUS_TYPE_BYTE)
#else
      || dbus_message_iter_get_array_type (&iter) != DBUS_TYPE_BYTE)
#endif
    goto wrong_args;

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_iter_get_fixed_array (&iter, &data, &len);
#else
  dbus_message_iter_get_byte_array (&iter, &data, &len);
#endif

  reply = dbus_message_new_method_return (message);
  if (!reply)
    {
      dbus_free (filename);
      dbus_free (mime_type);
      dbus_free (data);

      return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

  ctx = g_new (struct dbus_push_context, 1);

  ctx->filename = filename;
  ctx->mimetype = mime_type;
  ctx->data = data;
  ctx->reply = reply;
  ctx->connection = connection;

  obex_object_push (filename, mime_type, data, len, G_CALLBACK (push_done), ctx);

  return DBUS_HANDLER_RESULT_HANDLED;

 wrong_args:
  if (filename)
    dbus_free (filename);

  if (mime_type)
    dbus_free (mime_type);

  if (data)
    dbus_free (data);

  reply = dbus_message_new_error (message, WRONG_ARGS_ERROR,
				  "Wrong arguments for ObjectPush method");

  dbus_connection_send (connection, reply, NULL);

  dbus_message_unref (reply);

  return DBUS_HANDLER_RESULT_HANDLED;
}
