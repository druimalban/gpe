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
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/gpe-iconlist.h>
#include <gpe/tray.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "main.h"
#include "dun.h"

#define _(x) gettext(x)

void
stop_dun (struct bt_device *bd)
{
  pid_t p = vfork ();
  char bdaddr[40];

  strcpy (bdaddr, batostr (&bd->bdaddr));

  if (p == 0)
    {
      execlp ("dund", "dund", "-k", bdaddr, NULL);
      _exit (1);
    }
}

void
start_dun (struct bt_device *bd)
{
  /* connect */
  char port[16];
  char address[64];
  pid_t p;
  GSList *list;
  struct bt_service *sv = NULL;

  for (list = bd->services; list; list = list->next)
    {
      struct bt_service *tsv = list->data;
      if (tsv->type == BT_DUN)
	sv = tsv;
    }

  if (! radio_is_on)
    {
      gpe_error_box (_("Radio is switched off"));
      return;
    }
  
  strcpy (address, batostr (&bd->bdaddr));
  sprintf (port, "%d", sv->port);
  
  p = vfork ();
  
  if (p == 0)
    {
      execlp ("dund", "dund", "-n", "-C", port, "-c", address, NULL);
      _exit (1);
    }
}
