/*
 * gpe-conf
 *
 * Copyright (C) 2002   Moray Allan <moray@sermisy.org>,Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "rotation.h"
#include "gpe/errorbox.h"

extern int initialising;
extern char *RotationLabels;
int get_rotation ()
{
  FILE *pipe;
  int rotation;
#ifdef __i386__
  return 0; // xrandr doesnt exit on i386 dev machines!
#endif
  pipe = popen ("xrandr", "r");

  if (pipe > 0)
    {
      char buffer[256], buffer2[20];
      fgets (buffer, 255, pipe);
      rotation = -1;
      while ((feof(pipe) == 0) && (rotation < 0))
	{
	  if (sscanf (buffer, "Current rotation - %20s", buffer2) > 0)
	    {
	      if (strcmp (buffer2, "normal") == 0)
		{
		  rotation = 0;
		}
	      else if (strcmp (buffer2, "left") == 0)
		{
		  rotation = 1;
		}
	      else if (strcmp (buffer2, "inverted") == 0)
		{
		  rotation = 2;
		}
	      else if (strcmp (buffer2, "right") == 0)
		{
		  rotation = 3;
		}
	    }
	  fgets (buffer, 255, pipe);
	}
      if (rotation < 0)
	{
	  gpe_error_box ("can't interpret output from xrandr\n");
	  rotation = 0;
	}
      pclose (pipe);
    }
  else
    {
      gpe_error_box( "couldn't read rotation\n");
      rotation = 0;
    }

  return rotation;
}

static char *Rotations[4]=
  {
    "normal",
    "left",
    "inverted",
    "right"
  };

void set_rotation (int rotation)
{
#ifdef __i386__
      gpe_error_box( "couldn't set rotation\n");
      return ; // xrandr doesnt exit on i386 dev machines!
#endif
  if (initialising == 0)
    {
      int pid;
       pid = fork();
      if (pid == 0)
	{
	  execlp ("xrandr", "xrandr", "-o", Rotations[rotation], 0);
	  exit (0);
	}
      else if (pid > 0)
	{
	  waitpid (pid, NULL, 0);
	}
    }
}
