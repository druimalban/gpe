/*
 * gpe-conf
 *
 * Copyright (C) 2002   Moray Allan <moray@sermisy.org>,Pierre TARDY <tardyp@free.fr>
 *               2003   Florian Boor <florian.boor@kernelconcepts.de>
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
#include <libintl.h>

#include "rotation.h"
#include "gpe/errorbox.h"
#include "../parser.h"
#include "../applets.h"

extern char *RotationLabels;

static char *Rotations[4]=
  {
    "normal",
    "left",
    "inverted",
    "right"
  };

static char *xmodmaps[4]=
  {
    "/etc/X11/xmodmap-portrait",
    "/etc/X11/xmodmap-left",
    "/etc/X11/xmodmap-invert",
    "/etc/X11/xmodmap-right"
  };
  
int get_rotation ()
{
  int rotation = 0,i;
  char buffer2[20];
#ifdef __i386__
  return 0;
#else

  if(parse_pipe("/usr/X11R6/bin/xrandr" , "Current rotation - %20s" , buffer2))
    {
      gpe_error_box (_("Can't interpret output from xrandr!"));
      rotation = 0;
    }
  else
    {
      for(i=0;i<4;i++)
	if (strcmp (buffer2, Rotations[i]) == 0)
	  rotation = i;
    }	
  return rotation;
#endif	
}


void set_rotation (int rotation)
{
#ifdef __i386__
      gpe_error_box(_("Can't set rotation on an x86."));
      return ; 
#else
    system_printf("/usr/X11R6/bin/xrandr -o %s",Rotations[rotation]);
    system_printf("/usr/X11R6/bin/xmodmap %s",xmodmaps[rotation]);
#endif  
}
