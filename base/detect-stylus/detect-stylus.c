/*
 * Copyright (C) 2002, 2004 Philip Blundell <philb@gnu.org>
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <linux/input.h>

int
main (int argc, char *argv[])
{
  char b = 1;
  Atom atom;
  Display *dpy;
  Window root;
  int fd = open ("/dev/touchscreen/0", O_RDONLY);
  
  /* if neccessary probe others */
  if (fd < 0) 
    fd = open ("/dev/touchscreen/ucb1x00", O_RDONLY);

  if (fd < 0)
    {
      DIR *d;

      d = opendir ("/dev/input");
      if (d)
	{
	  struct dirent *de;
	  
	  while (fd < 0)
	    {
	      de = readdir (d);
	      if (!de)
		break;
	      if (!strncmp (de->d_name, "event", 5))
		{
		  char buf[512];
		  int fd;
		  sprintf (buf, "/dev/input/%s", de->d_name);
		  fd = open (buf, O_RDONLY);
		  if (fd >= 0)
		    {
		      long c;
		      if (ioctl (fd, EVIOCGBIT(EV_ABS, sizeof (c)), &c) < 0)
			c = 0;
		      if ((c & 3) != 3)
			{
			  close (fd);
			  fd = -1;
			}
		    }
		}
	    }
	  closedir (d);
	}
    }
  
  if (fd >= 0)
    {
      close (fd);

      if ((dpy = XOpenDisplay (NULL)) == NULL)
	{
	  fprintf (stderr, "Cannot connect to X server.");
	  exit (1);
	}
      
      root = RootWindow (dpy, 0);
      atom = XInternAtom (dpy, "_GPE_STYLUS_MODE", 0);
      
      XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, &b, 1);

      XCloseDisplay (dpy);
      if (!access("/usr/X11R6/bin/xrdb", X_OK))
        system ("echo \"Matchbox.cursor: no\nXcursor.theme: fully-transparent\" | /usr/X11R6/bin/xrdb -nocpp -merge");
      else
        system ("echo \"Matchbox.cursor: no\nXcursor.theme: fully-transparent\" | /usr/bin/xrdb -nocpp -merge");
    }
      
  exit (0);
}
