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
#include "gpe/errorbox.h"
#include "xset.h"
#include "../parser.h"
#include "../applets.h"


int xset_get_ss_sec()
{
  int sec = 0;
  if(parse_pipe("xset q","  timeout: %5d",&sec))
    {
       gpe_error_box( "couldn't read screen saver time");
    }
  return sec;

}

void xset_set_ss_sec(int sec)
{
  if(sec)
    system_printf("xset s %d",sec);
  else
    system("xset s off");
}
