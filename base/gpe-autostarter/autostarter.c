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

#define REQUEST_NAME "org.handhelds.gpe.hotplug"

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
	  execlp ("miniwave", "miniwave", NULL);
	  perror ("miniwave");
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

void
autostarter_handle_dbus_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  int type;
  char *subsys;

  dbus_message_iter_init (message, &iter);
  
  type = dbus_message_iter_get_arg_type (&iter);
  if (type != DBUS_TYPE_STRING)
    return;

  subsys = dbus_message_iter_get_string (&iter);

  if (!strcmp (subsys, "net"))
    handle_net_message (message, &iter);
}

DBusHandlerResult
dbus_handler_func (DBusMessageHandler *handler,
		   DBusConnection     *connection,
		   DBusMessage        *message,
		   void               *user_data)
{
  if (dbus_message_has_name (message, REQUEST_NAME))
    autostarter_handle_dbus_request (connection, message);
  
  if (dbus_message_has_name (message, DBUS_MESSAGE_LOCAL_DISCONNECT))
    exit (0);
  
  return DBUS_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

void
autostarter_init_dbus (void)
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

int
main (int argc, char *argv[])
{
  GMainLoop *main_loop;

  signal (SIGCHLD, SIG_IGN);

  main_loop = g_main_loop_new (NULL, FALSE);

  autostarter_init_dbus ();

  g_main_loop_run (main_loop);

  return 0;
}
