/*
 * Copyright (C) 2003, 2004 Philip Blundell <philb@gnu.org>
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

pid_t gpe_bluetooth_pid;

void
handle_bluetooth_message (DBusMessage *message, DBusMessageIter *iter)
{
  int type;
  char *action;

  if (! dbus_message_iter_next (iter))
    return;

  type = dbus_message_iter_get_arg_type (iter);
  if (type != DBUS_TYPE_STRING)
    return;

  dbus_message_iter_get_basic (iter, &action);

  if (! strcmp (action, "register") || ! strcmp (action, "add"))
    {
      pid_t pid;

      pid = vfork ();

      if (pid == 0)
	{
	  execlp ("gpe-bluetooth", "+gpe-bluetooth", NULL);
	  perror ("gpe-bluetooth");
	  _exit (1);
	}

      gpe_bluetooth_pid = pid;
    }
  else if (! strcmp (action, "unregister") || ! strcmp (action, "remove"))
    {
      if (gpe_bluetooth_pid)
	{
	  kill (gpe_bluetooth_pid, SIGTERM);
	  gpe_bluetooth_pid = 0;
	}
    }
}
