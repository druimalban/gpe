/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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
#include <signal.h>

#include <glib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define _(x) gettext(x)

pid_t miniwave_pid;

void
handle_net_wlan (char *action, char *interface)
{
  if (! strcmp (action, "register"))
    {
      pid_t pid;

      pid = vfork ();

      if (pid == 0)
	{
	  execlp ("mb-applet-wireless", "mb-applet-wireless", NULL);
	  perror ("mb-applet-wireless");
	  _exit (1);
	}

      miniwave_pid = pid;
    }
  else if (! strcmp (action, "unregister"))
    {
      if (miniwave_pid)
	{
	  kill (miniwave_pid, SIGTERM);
	  miniwave_pid = 0;
	}
    }
}

void
handle_net_message (DBusMessage *message, DBusMessageIter *iter)
{
  int type;
  char *action, *interface;

  if (! dbus_message_iter_next (iter))
    return;

  type = dbus_message_iter_get_arg_type (iter);
  if (type != DBUS_TYPE_STRING)
    return;

  action = dbus_message_iter_get_string (iter);

  if (! dbus_message_iter_next (iter))
    return;

  type = dbus_message_iter_get_arg_type (iter);
  if (type != DBUS_TYPE_STRING)
    return;

  interface = dbus_message_iter_get_string (iter);

  if (! strncmp (interface, "wlan", 4))
    handle_net_wlan (action, interface);
}

