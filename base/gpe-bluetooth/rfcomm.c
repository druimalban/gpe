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
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "main.h"
#include "rfcomm.h"

gboolean
rfcomm_connect (bdaddr_t *bdaddr, int channel, int *fd, GError *error)
{
  struct sockaddr_rc sa;
  int sk;

  sk = socket (AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (sk < 0) 
    return FALSE;

  sa.rc_family  = AF_BLUETOOTH;
  sa.rc_channel = 0;
  sa.rc_bdaddr  = src_addr;

  if (bind (sk, (struct sockaddr *) &sa, sizeof (sa)))
    {
      close (sk);
      return FALSE;
    }
  
  sa.rc_channel = channel;
  baswap (&sa.rc_bdaddr, bdaddr);

  if (connect (sk, (struct sockaddr *) &sa, sizeof (sa)))
    {
      close (sk);
      return FALSE;
    }

  *fd = sk;

  return TRUE;
}

