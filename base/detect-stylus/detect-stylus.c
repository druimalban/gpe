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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <linux/input.h>

int flag_device;

int
try_open (char *name)
{
  int fd;

  fd = open (name, O_RDONLY);
  if (fd >= 0)
    {
      if (flag_device)
	{
	  printf ("%s\n", name);
	  exit (0);
	}
    }

  return fd;
}

int
main (int argc, char *argv[])
{
  char b = 1;
  char *tsdev;
  Atom atom;
  Display *dpy;
  Window root;
  int fd;
  int i;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "--device"))
	flag_device = 1;
    }

  if (flag_device)
    fprintf (stderr, "%s: warning: the \"--device\" flag is obsolete, use detect-tsdevice instead.\n", argv[0]);

  fd = try_open ("/dev/touchscreen/0");
  
  /* if neccessary probe others */
  if (fd < 0) 
    fd = try_open ("/dev/touchscreen/ucb1x00");

  if (fd < 0) 
    fd = try_open ("/dev/ts");

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
			  continue;
			}
		      
		      if (flag_device)
			{
			  printf ("%s\n", buf);
			  exit (0);
			}
		    }
		}
	    }
	  closedir (d);
	}
    }
	
  /* still not found? try environment */
  if (fd < 0)
    {
      tsdev = getenv ("TSLIB_TSDEVICE");
      if (tsdev != NULL)
        fd = try_open (tsdev);
    }

  if (flag_device)
    exit (1);		/* Nothing found */

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
      
      XChangeProperty (dpy, root, atom, XA_INTEGER, 8, PropModeReplace, (unsigned char *)&b, 1);

      XCloseDisplay (dpy);
      if (access("/usr/bin/xrdb", X_OK))
        system ("echo \"Matchbox.cursor: no\nXcursor.theme: xcursor-transparent\" | /usr/X11R6/bin/xrdb -nocpp -merge");
      else
        system ("echo \"Matchbox.cursor: no\nXcursor.theme: xcursor-transparent\" | /usr/bin/xrdb -nocpp -merge");
    }
      
  exit (0);
}
