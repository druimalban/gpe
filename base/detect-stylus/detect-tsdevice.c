/*
 * Copyright (C) 2002, 2004, 2006 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include <linux/input.h>

void
try_open (char *name)
{
  int fd;

  fd = open (name, O_RDONLY);
  if (fd >= 0)
    {
      printf ("%s\n", name);
      exit (0);
    }
}

int
main (int argc, char *argv[])
{
  char *tsdev;
  DIR *d;

  try_open ("/dev/touchscreen/0");
  try_open ("/dev/touchscreen/ucb1x00");

  d = opendir ("/dev/input");
  if (d)
    {
      struct dirent *de;
      
      while (de = readdir (d), de != NULL)
	{
	  if (!strncmp (de->d_name, "event", 5))
	    {
	      int fd;
	      char buf[512];
	      sprintf (buf, "/dev/input/%s", de->d_name);
	      fd = open (buf, O_RDONLY);
	      if (fd >= 0)
		{
		  long c;
		  if (ioctl (fd, EVIOCGBIT (EV_ABS, sizeof (c)), &c) < 0)
		    c = 0;
		  if ((c & 3) != 3)
		    {
		      close (fd);
		      fd = -1;
		      continue;
		    }
		  
		  printf ("%s\n", buf);
		  exit (0);
		}
	    }
	}
      
      closedir (d);
    }
	
  /* still not found? try environment */
  tsdev = getenv ("TSLIB_TSDEVICE");
  if (tsdev != NULL)
    try_open (tsdev);

  exit (1);		/* Nothing found */
}
