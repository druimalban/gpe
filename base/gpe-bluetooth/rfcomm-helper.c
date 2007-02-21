/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

static int
rfcomm_create_tty (int sk, char *tty, int size)
{
  struct sockaddr_rc sa;
  struct stat st;
  int id;
  uint alen;

  struct rfcomm_dev_req req = 
    {
      flags:   (1 << RFCOMM_REUSE_DLC) | (1 << RFCOMM_RELEASE_ONHUP),
      dev_id:  -1
    };
  
  alen = sizeof (sa);
  if (getpeername (sk, (struct sockaddr *) &sa, &alen) < 0)
    return -1;
  bacpy (&req.dst, &sa.rc_bdaddr);
  
  alen = sizeof (sa);
  if (getsockname (sk, (struct sockaddr *) &sa, &alen) < 0)
    return -1;
  bacpy (&req.src, &sa.rc_bdaddr);
  req.channel = sa.rc_channel;

  id = ioctl (sk, RFCOMMCREATEDEV, &req);
  if (id < 0)
    {
      perror ("RFCOMMCREATEDEV");
      return id;
    }
  
  snprintf (tty, size, "/dev/rfcomm%d", id);
  if (stat (tty, &st) < 0) 
    {
      snprintf (tty, size, "/dev/bluetooth/rfcomm/%d", id);
      if (stat (tty, &st) < 0) 
	{
	  snprintf (tty, size, "/dev/rfcomm%d", id);
	  return -1;
	}
    }
  
  return 0;
}

int
main (int argc, char *argv[])
{
  char buf[64];

  if (argc < 2)
    exit (1);

  if (rfcomm_create_tty (atoi (argv[1]), buf, sizeof (buf)))
    exit (1);

  if (argc == 3 && !strcmp (argv[2], "ppp"))
    {
      execl ("/usr/sbin/pppd", "pppd", buf, "call", "lap", NULL);
      perror ("pppd");
      exit (1);
    }

  printf ("%s\n", buf);

  exit (0);
}
