/*
 * gpe-conf
 *
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>, Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "gpe/errorbox.h"
#include <sys/types.h>
#include <sys/wait.h>

#include "brightness.h"
#include "../applets.h"
#include "../parser.h"

void set_brightness (int brightness)
{
#ifdef __i386__
  return ; // bl doesnt exit on i386 dev machines!
#endif
  if (brightness == 0)
    system ("bl off\n");
  else
    system_printf("bl %d",brightness);
}

int get_brightness ()
{
  
  FILE *pipe;
  int brightness;
  char state[5];
#ifdef __i386__
  return 10; // bl doesnt exit on i386 dev machines!
#endif
  if( parse_pipe("bl","%4s %d",state,&brightness))
    {
      gpe_error_box ( "couldn't read brightness");
      strcpy (state, "on");
      brightness = 255;
    }
  if (strcmp (state, "off") == 0)
    {
      brightness = 0;
    }
  return brightness;
}
