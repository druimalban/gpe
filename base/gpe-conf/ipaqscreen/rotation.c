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
#include "../parser.h"
#include "../applets.h"

extern int initialising;
extern char *RotationLabels;

static char *Rotations[4]=
  {
    "normal",
    "left",
    "inverted",
    "right"
  };

int get_rotation ()
{
  int rotation = 0,i;
  char buffer2[20];
#ifdef __i386__
  return 0; // xrandr doesnt exit on i386 dev machines!
#endif

  if(parse_pipe("xrandr" , "Current rotation - %20s" , buffer2))
    {
      gpe_error_box ("can't interpret output from xrandr\n");
      rotation = 0;
    }
  else
    {
      for(i=0;i<4;i++)
	if (strcmp (buffer2, Rotations[i]) == 0)
	  rotation = i;

    }
  return rotation;
}


void set_rotation (int rotation)
{
#ifdef __i386__
      gpe_error_box( "couldn't set rotation\n");
      return ; // xrandr doesnt exit on i386 dev machines!
#endif
  if (initialising == 0)
    system_printf("xrandr -o %s",Rotations[rotation]);
}
