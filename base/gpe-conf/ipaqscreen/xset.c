/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
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
#include "xset.h"


int xset_get_ss_sec()
{
  FILE *pipe;
  int sec = 60,found=0;
  pipe = popen ("xset q", "r");
  if (pipe > 0)
    {
      while ((feof(pipe) == 0) && ! found)
	{
	  char buffer[256];

	  fgets (buffer, 255, pipe);
	  if (sscanf (buffer, "  timeout:  %5d", &sec) > 0)
	      found=1;
	}
      if (!found)
	{
	  fprintf (stderr, "can't interpret output from xset\n");
	}
      pclose (pipe);
    }
  else
    {
      fprintf (stderr, "couldn't read screen saver time\n");
    }
  return sec;

}
void xset_set_ss_sec(int sec)
{
  char buf[20];
  if(sec)
    sprintf(buf,"xset s %d",sec);
  else
    sprintf(buf,"xset s off");

  system(buf);
}
