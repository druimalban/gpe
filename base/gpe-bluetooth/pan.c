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
#include <unistd.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"

void
start_pan (struct bt_device *bd)
{
  char bdaddr[40];
  pid_t pid;
  
  strcpy (bdaddr, batostr (&bd->bdaddr));

  pid = vfork ();
  if (pid == 0)
    {
      execlp ("pand", "pand", "-c", bdaddr, NULL);
      _exit (1);
    }
}

void
stop_pan (struct bt_device *bd)
{
  char bdaddr[40];
  pid_t pid;
  
  strcpy (bdaddr, batostr (&bd->bdaddr));

  pid = vfork ();
  if (pid == 0)
    {
      execlp ("pand", "pand", "-k", bdaddr, NULL);
      _exit (1);
    }
}

