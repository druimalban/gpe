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

#define _(x) gettext(x)

DBusHandlerResult
obex_client_handle_dbus_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  DBusMessage *reply;
  int type;

  dbus_message_iter_init (message, &iter);
 
  reply = dbus_message_new_method_return (message);
  if (!reply)
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  return DBUS_HANDLER_RESULT_HANDLED;
}

