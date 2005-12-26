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

#include "pin-ui.h"

#define SERVICE_NAME   "org.bluez.PinAgent"
#define INTERFACE_NAME SERVICE_NAME

#define _(x) gettext(x)

extern DBusHandlerResult bluez_pin_handle_dbus_request (DBusConnection *connection, DBusMessage *message);

DBusHandlerResult
handler_func (DBusConnection     *connection,
	      DBusMessage        *message,
	      void               *user_data)
{
  if (dbus_message_is_method_call (message, INTERFACE_NAME, "PinRequest"))
    return bluez_pin_handle_dbus_request (connection, message);
  
  if (dbus_message_is_signal (message,
                              DBUS_INTERFACE_LOCAL,
                              "Disconnected"))
    exit (0);
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusObjectPathVTable dbus_vtable = {
  NULL,
  handler_func,
  NULL
};

void
bluez_pin_dbus_server_run (void)
{
  DBusConnection *connection;
  DBusError error;
  static const char *old_object_path = "/org/handhelds/gpe/bluez/PinAgent";
  static const char *new_object_path = "/org/bluez/PinAgent"; 

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

  dbus_connection_register_object_path (connection, new_object_path, &dbus_vtable, NULL);
  dbus_connection_register_object_path (connection, old_object_path, &dbus_vtable, NULL);

  dbus_bus_request_name (connection, SERVICE_NAME, 0, &error);
  if (dbus_error_is_set (&error))
    {
      fprintf (stderr, "Failed to acquire %s service: %s\n", SERVICE_NAME, error.message);
      dbus_error_free (&error);
      exit (1);
    }
}

void
legacy_pin_result (BluetoothPinRequest *req, gchar *result)
{
  if (result)
    printf ("PIN:%s\n", result);
  else
    printf ("ERR\n");

  gtk_main_quit ();
}

void
legacy_pin_request (gboolean outgoing, gchar *address, gchar *name)
{
  BluetoothPinRequest *req;

  req = bluetooth_pin_request_new (outgoing, address, name);

  g_signal_connect (G_OBJECT (req), "result", G_CALLBACK (legacy_pin_result), NULL);
}

void
usage (char *argv[])
{
  fprintf (stderr, _("Usage: %s <in|out> <address> [name]\n"), argv[0]);
  fprintf (stderr, _(" or    %s --dbus\n"), argv[0]);
  exit (1);
}

int 
main (int argc, char *argv[])
{
  char *dpy;
  gboolean dbus_mode = FALSE;
  gchar *address, *name = NULL;

  dpy = getenv ("DISPLAY");

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (argc > 1 && !strcmp (argv[1], "--dbus"))
    dbus_mode = TRUE;

  if (dbus_mode == FALSE && dpy == NULL)
    {
      char *auth = NULL;
      FILE *fp;
      dpy = ":0";
      fp = popen ("/bin/ps --format args --no-headers -C X -C Xorg -C XFree86", "r");
      if (fp)
	{
	  char buf[1024];
	  while (fgets (buf, sizeof (buf), fp))
	    {
	      char *p = strtok (buf, " ");
	      int authf = 0;
	      while (p)
		{
		  if (authf)
		    {
		      auth = strdup (p);
		      authf = 0;
		    }
		  else if (p[0] == ':')
		    dpy = strdup (p);
		  else if (!strcmp (p, "-auth"))
		    authf = 1;
		  p = strtok (NULL, " ");
		}
	    }
	  pclose (fp);
	}
      setenv ("DISPLAY", dpy, 0);
      if (auth)
	setenv ("XAUTHORITY", auth, 0);
    }

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  if (dbus_mode)
    {
      bluez_pin_dbus_server_run ();

      gtk_main ();
    }
  else
    {
      gboolean outgoing = FALSE;

      if (argc < 3)
	usage (argv);

      if (strcmp (argv[1], "in") == 0)
	outgoing = FALSE;
      else if (strcmp (argv[1], "out") == 0)
	outgoing = TRUE;
      else
	usage (argv);

      address = argv[2];
      if (argc == 4)
	name = argv[3];

      legacy_pin_request (outgoing, address, name);

      gtk_main ();
    }

  exit (0);
}
