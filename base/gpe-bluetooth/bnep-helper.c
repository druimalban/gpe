/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>

#include "bnep.h"

int ctl;

static int 
bnep_connadd(int sk, uint16_t role)
{
  struct bnep_connadd_req req;

  req.sock = sk;
  req.role = role;

  if (ioctl(ctl, BNEPCONNADD, &req))
    return -1;

  return 0;
}

int bnep_init(void)
{
  ctl = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_BNEP);
  if (ctl < 0) 
    {
      perror("Failed to open control socket");
      return 1;
    }
  
  return 0;
}

int
main (int argc, char *argv[])
{
  bnep_init ();

  if (argc < 2)
    exit (1);

  return bnep_connadd (atoi (argv[1]), BNEP_SVC_PANU);
}
