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

#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/ioctl.h> 

#include <glib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define _(x) gettext(x)

static pid_t miniwave_pid;
static GSList *wlan_devs;

static void
start_miniwave (void)
{
  pid_t pid;

  pid = vfork ();

  if (pid == 0)
    {
      /* prefix argv[0] with + to defeat matchbox-panel session handling */
      execlp ("mb-applet-wireless", "+mb-applet-wireless", NULL);
      perror ("mb-applet-wireless");
      _exit (1);
    }
  
  miniwave_pid = pid;
}

static void
stop_miniwave (void)
{
  if (miniwave_pid)
    {
      kill (miniwave_pid, SIGTERM);
      miniwave_pid = 0;
    }
}

static gboolean 
device_is_wlan (char* ifname)
{
  int fd;
  struct iwreq wrq;
  gboolean result;
  
  fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    return 0;
  
  memset (&wrq, 0, sizeof (wrq));
  strncpy (wrq.ifr_name, ifname, IFNAMSIZ);
  if (ioctl (fd, SIOCGIWNAME, &wrq) < 0)
    result = FALSE;
  else
    result = TRUE;
  close (fd);
  
  return result;
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

  if (! strcmp (action, "register") || ! strcmp (action, "add"))
    {
      if (device_is_wlan (interface))
	{
	  if (!wlan_devs)
	    start_miniwave ();
	  wlan_devs = g_slist_prepend (wlan_devs, g_strdup (interface));
	}
    }
  else if (! strcmp (action, "unregister") || ! strcmp (action, "remove"))
    {
      GSList *i;

      for (i = wlan_devs; i; i = i->next)
	{
	  gchar *s = i->data;
	  if (!strcmp (s, interface))
	    {
	      wlan_devs = g_slist_remove (wlan_devs, s);
	      g_free (s);
	      if (wlan_devs == NULL)
		stop_miniwave ();
	      break;
	    }
	}
    }
}

