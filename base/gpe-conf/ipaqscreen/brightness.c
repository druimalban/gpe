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
#include <string.h>
#include <unistd.h>

#include "gpe/errorbox.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <libintl.h>

#include "brightness.h"
#include "../applets.h"
#include "../parser.h"

#include <fcntl.h>
#ifdef MACH_IPAQ
#include <sys/ioctl.h>
#include <linux/h3600_ts.h>
#define TS_DEV "/dev/touchscreen/0"
static FLITE_IN bl;
#endif

void turn_light(int status)
{
#ifdef MACH_IPAQ
  int fd;
  bl.mode=1;
  bl.pwr=status;
  fd = open(TS_DEV, O_RDWR);
  if (fd == -1)
  	return;
  ioctl(fd,FLITE_ON,(void *)&bl);
  close(fd);
#endif
}

int get_light_state ()
{
#ifndef MACH_IPAQ
  return 10; // bl doesnt exit on i386 dev machines!
#else
  struct h3600_ts_backlight tsbl;
  int fd;
  char state[5];
  
  tsbl.power = 0;
  fd = open(TS_DEV, O_RDONLY);  // if we are allowed, we read bl setting direct from device
  if (fd != -1) {
    ioctl(fd,TS_GET_BACKLIGHT,(void *)&tsbl);
    close(fd);
  }
  else
  {	
    if( parse_pipe("bl","%4s %d",state,&tsbl.brightness))
    {
      gpe_error_box (_("Couldn't read screen settings!"));
      strcpy (state, "on");
      tsbl.brightness = 128;
    }
    if (strcmp(state,"off") == 0)
    {
      tsbl.power = 0;
    }
	else
	{
      tsbl.power = 1;
	}
  }
  if (tsbl.power) bl.brightness = tsbl.brightness;
  return tsbl.power;
#endif
}

void set_brightness (int brightness)
{
#ifdef MACH_IPAQ
  int fd;
	
  if (brightness) 
  {
      bl.pwr= 1;
  }
  bl.mode=1;
  fd = open(TS_DEV, O_RDWR);
  if (fd == -1)
    return;
  bl.brightness=(unsigned char)brightness;
  ioctl(fd,FLITE_ON,(void *)&bl);
   close(fd);
#endif
}

int get_brightness ()
{
#ifndef MACH_IPAQ
  return 10; // bl doesnt exit on i386 dev machines!
#else
  struct h3600_ts_backlight tsbl;
  int fd;
  char state[5];
  
  tsbl.brightness = 10;
  fd = open(TS_DEV, O_RDONLY);  // if we are allowed, we read bl setting direct from device
  if (fd != -1) {
    ioctl(fd,TS_GET_BACKLIGHT,(void *)&tsbl);
    close(fd);
  }
  else
  {	
    if( parse_pipe("bl","%4s %d",state,&tsbl.brightness))
    {
      gpe_error_box (_("Couldn't read screen settings!"));
      strcpy (state, "on");
      tsbl.brightness = 128;
    }
    if (strcmp (state, "off") == 0)
    {
      tsbl.brightness = 0;
    }
  }
  bl.brightness = tsbl.brightness;
  return tsbl.brightness;
#endif
}
