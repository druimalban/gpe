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
#include <dbus/dbus-glib-lowlevel.h>

#include <X11/Xlib.h>

#define PATH_NAME "/org/handhelds/gpe/hotplug"
#define SIGNAL_NAME "HotplugEvent"

#define _(x) gettext(x)

extern DBusHandlerResult handle_bluetooth_message (DBusMessage *message, DBusMessageIter *iter);
extern DBusHandlerResult handle_net_message (DBusMessage *message, DBusMessageIter *iter);

Display *dpy;

DBusHandlerResult
autostarter_handle_dbus_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  int type;
  char *subsys;

  dbus_message_iter_init (message, &iter);
  
  type = dbus_message_iter_get_arg_type (&iter);
  if (type != DBUS_TYPE_STRING)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  subsys = dbus_message_iter_get_string (&iter);

  if (!strcmp (subsys, "net"))
    return handle_net_message (message, &iter);

#if 0
  if (!strcmp (subsys, "bluetooth"))
    handle_bluetooth_message (message, &iter);
#endif

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusHandlerResult
dbus_handler_func (DBusConnection     *connection,
		   DBusMessage        *message,
		   void               *user_data)
{
  if (dbus_message_is_signal (message, PATH_NAME, SIGNAL_NAME))
    return autostarter_handle_dbus_request (connection, message);
  
  if (dbus_message_is_signal (message,
                              DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL,
                              "Disconnected"))
    exit (0);
  
  return DBUS_HANDLER_RESULT_HANDLED;
}

void
autostarter_init_dbus (void)
{
  DBusConnection *connection;
  DBusError error;

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

  dbus_bus_add_match (connection,
                      "type='signal'",
                      &error);
  if (dbus_error_is_set (&error))
    {
      fprintf (stderr, "Error: %s\n", error.message);
      exit (1);
    }

  dbus_connection_add_filter (connection, dbus_handler_func, NULL, NULL);
}

typedef struct
{
  GSource source; /**< the parent GSource */

  GPollFD *poll_fd;
} GSourceWrapped;

static gboolean
source_prepare (GSource *source,
		gint    *timeout)
{
  *timeout = -1;

  return FALSE;
}

static gboolean
source_check (GSource *source)
{
  GSourceWrapped *s = (GSourceWrapped *)source;

  if (s->poll_fd->revents != 0)
    return TRUE;

  return FALSE;  
}

static gboolean
source_dispatch (GSource     *source,
		 GSourceFunc  callback,
		 gpointer     user_data)
{
  XEvent ev;

  while (XPending (dpy))
    XNextEvent (dpy, &ev);

  return TRUE;
}

GSourceFuncs source_funcs =
  {
    source_prepare,
    source_check,
    source_dispatch,
    NULL
  };

int
main (int argc, char *argv[])
{
  GMainLoop *main_loop;
  GSourceWrapped *source;

  signal (SIGCHLD, SIG_IGN);

  main_loop = g_main_loop_new (NULL, FALSE);

  dpy = XOpenDisplay (NULL);
  if (dpy == NULL)
    {
      fprintf (stderr, "Couldn't connect to X display\n");
      exit (1);
    }

  source = (GSourceWrapped *)g_source_new (&source_funcs, sizeof (GSourceWrapped));

  source->poll_fd = g_new (GPollFD, 1);
  source->poll_fd->fd = ConnectionNumber (dpy);
  source->poll_fd->events = G_IO_IN | G_IO_ERR | G_IO_HUP;

  g_source_add_poll ((GSource *)source, source->poll_fd);
  g_source_attach ((GSource *)source, g_main_loop_get_context (main_loop));

  autostarter_init_dbus ();

  g_main_loop_run (main_loop);

  return 0;
}
