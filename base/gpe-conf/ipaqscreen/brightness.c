/*
 * gpe-conf
 *
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>, 
 *   Pierre TARDY <tardyp@free.fr>
 * Copyright (C) 2004 Florian Boor <florian.boor@kernelconcepts.de>
 * 
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

#include <sys/types.h>
#include <sys/wait.h>
#include <libintl.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>

#include <gdk/gdk.h>

#include "brightness.h"


typedef enum
{
	P_NONE,
	P_IPAQ,
	P_ZAURUS,
	P_CORGI,
	P_INTEGRAL,
	P_SIMPAD,
	P_SIMPAD_NEW,
    P_GENERIC
}t_platform;


/* for iPAQ touchscreen */
#define IOC_H3600_TS_MAGIC  'f'
#define TS_GET_BACKLIGHT        _IOR(IOC_H3600_TS_MAGIC, 20, struct h3600_ts_backlight)
#define TS_SET_BACKLIGHT        _IOW(IOC_H3600_TS_MAGIC, 20, struct h3600_ts_backlight)

enum flite_pwr {
        FLITE_PWR_OFF = 0,
        FLITE_PWR_ON  = 1
};

struct h3600_ts_backlight {
        enum flite_pwr power;          /* 0 = off, 1 = on */
        unsigned char  brightness;     /* 0 - 255         */
};

#define TS_DEV "/dev/touchscreen/0raw"

/* for Integral 90200 */
#define PROC_LIGHT "/proc/hw90200/backlight"

/* for Sharp Corgi */
#define CORGI_FL "/proc/driver/fl/corgi-bl"

/* other Zauri */
#define ZAURUS_FL "/dev/fl"
#define FL_IOCTL_STEP_CONTRAST    100

/* Simpad, a little bit nasty - PWM reg. is set directly */

#define SIMPAD_BACKLIGHT_REG	"/proc/driver/mq200/registers/PWM_CONTROL"
#define SIMPAD_BACKLIGHT_MASK	0x00a10044

/* Simpad, new interface */
#define SIMPAD_BACKLIGHT_REG_NEW	"/proc/driver/mq200/backlight"

/* Generic backight */
#define GENERIC_PROC_DRIVER "/proc/driver/backlight"

static t_platform platform = P_NONE;

static t_platform
detect_platform(void)
{
	if (!access(TS_DEV,R_OK))
		return P_IPAQ;
	if (!access(PROC_LIGHT,R_OK))
		return P_INTEGRAL;
	if (!access(CORGI_FL,R_OK))
		return P_CORGI;
	if (!access(ZAURUS_FL,R_OK))
		return P_ZAURUS;
	if (!access(SIMPAD_BACKLIGHT_REG_NEW,R_OK)) /* preserve order */
		return P_SIMPAD_NEW;
	if (!access(SIMPAD_BACKLIGHT_REG,R_OK))
		return P_SIMPAD;
	if (!access(GENERIC_PROC_DRIVER,R_OK))
		return P_GENERIC;
	return P_NONE;
}

void 
init_light(void)
{
	platform = detect_platform();
}


int 
generic_set_level(int level)
{
  FILE *f_light;
  int val = level/8;
  
  if (val < 1) val = 1;
  f_light = fopen(GENERIC_PROC_DRIVER,"w");
  if (f_light != NULL)
  {
    fprintf(f_light,"%d\n", val);
  	fclose(f_light);
	return level;
  }
  else
	  return -1;
}

int 
generic_get_level(void)
{
  FILE *f_light;
  int level;
  
  f_light = fopen(GENERIC_PROC_DRIVER,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%d", &level);
  	fclose(f_light);
	return level*8;
  }
  return -1;
}  

int 
simpad_new_set_level(int level)
{
  FILE *f_light;
  
  if (level < 1) level = 1;
  f_light = fopen(SIMPAD_BACKLIGHT_REG_NEW,"w");
  if (f_light != NULL)
  {
    fprintf(f_light,"%d\n", level);
  	fclose(f_light);
	return level;
  }
  else
	  return -1;
}

int 
simpad_new_get_level(void)
{
  FILE *f_light;
  int level;
  
  f_light = fopen(SIMPAD_BACKLIGHT_REG_NEW,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%d", &level);
  	fclose(f_light);
	return level;
  }
  return -1;
}  


int 
simpad_set_level(int level)
{
  int val;
  FILE *f_light;
  
  if (level < 1) level = 1;
  f_light = fopen(SIMPAD_BACKLIGHT_REG,"w");
  if (f_light >= 0)
  {
	val = 255 - level;
	val = val << 8;
	val += (int)SIMPAD_BACKLIGHT_MASK;
    fprintf(f_light,"0x%x\n", val);
  	fclose(f_light);
	return level;
  }
  else
	  return -1;
}

int 
simpad_get_level(void)
{
  FILE *f_light;
  int level;
  
  f_light = fopen(SIMPAD_BACKLIGHT_REG,"r");
  if (f_light >= 0)
  {
  	fscanf(f_light,"0x%x", &level);
  	fclose(f_light);
	level -= (int)SIMPAD_BACKLIGHT_MASK;
	level = level >> 8;
	return 255 - level;
  }
  return -1;
}  

int 
corgi_set_level(int level)
{
	int fd, val, len, res = -1;
	gchar buf[20];
	
	if ((fd = open(CORGI_FL, O_WRONLY)) >= 0)
	{
		val = (level == 1) ? 1 : level * ( 17.0 / 255.0 );
		len = snprintf(buf, 20, "0x%x\n", val);
		res = (write (fd, buf, len) >= 0);
		close (fd);
    }
	return ((res < 0) ? -1 : level);
}

int 
corgi_get_level(void)
{
	int val, res;
	FILE *fd;
	
	if ((fd = fopen(CORGI_FL, O_RDONLY)) != NULL)
	{
		res = fscanf(fd, "0x%x", &val);
		fclose (fd);
		if (res) 
			return ((val * 255) / 17);
    }
	return (-1);
}

int 
zaurus_set_level(int level)
{
	int fd, val, res = -1;
	
	if ((fd = open(ZAURUS_FL, O_WRONLY)) >= 0)
	{
		val = (level * 4 + 127) / 255;
		if (level && !val)
			val = 1;
            res = ioctl(fd, FL_IOCTL_STEP_CONTRAST, val);
		close (fd);
    }
	return (res != 0) ? -1 : level;
}

/* seems to be missing in kernel implementation */
int 
zaurus_get_level(void)
{
	return 25;	
}


int 
ipaq_set_level(int level)
{
	int light_fd;
	struct h3600_ts_backlight bl;

	light_fd = open (TS_DEV, O_RDONLY);
	if (light_fd > 0) {
  		bl.brightness = level;
  		bl.power = (level > 0) ? FLITE_PWR_ON : FLITE_PWR_OFF;
 		if (ioctl (light_fd, TS_SET_BACKLIGHT, &bl) != 0)
		{
			close(light_fd);
    		return -1;
		}
  		else
		{
			close(light_fd);
    		return level;
		}
	}
	return -1;
}

int 
ipaq_get_level(void)
{
	int light_fd;
	struct h3600_ts_backlight bl;

	light_fd = open (TS_DEV, O_RDONLY);
	if (light_fd > 0) {
 		if (ioctl (light_fd, TS_GET_BACKLIGHT, &bl) != 0)
		{
			close(light_fd);
    		return -1;
		}
  		else
		{
			close(light_fd);
    		return bl.brightness;
		}
	}
	return -1;
}

int 
integral_set_level(int level)
{
  FILE *f_light;
  
  f_light = fopen(PROC_LIGHT,"w");
  if (f_light != NULL)
  {
  	fprintf(f_light,"%i\n", level);
  	fclose(f_light);
	return level;
  }
  else
	  return -1;
}

int 
integral_get_level(void)
{
  FILE *f_light;
  int level;
  
  f_light = fopen(PROC_LIGHT,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%i", &level);
  	fclose(f_light);
	return level;
  }
  return -1;
}  


void set_brightness (int brightness)
{
	if (brightness < 0)
		brightness = 0;
	if (brightness > 255)
		brightness = 255;
	
	if (platform == P_NONE)
		init_light();
	
	switch (platform)
	{
	case P_IPAQ:
		ipaq_set_level(brightness);
	break;
	case P_ZAURUS:
		zaurus_set_level(brightness);
	break;
	case P_CORGI:
		corgi_set_level(brightness);
	break;
	case P_INTEGRAL:
		integral_set_level(brightness);
	break;
	case P_SIMPAD_NEW:
		simpad_new_set_level(brightness);
	break;
	case P_SIMPAD:
		simpad_set_level(brightness);
	break;
	case P_GENERIC:
		generic_set_level(brightness);
	break;
	default:
	break;
	}
}


int get_brightness ()
{
	
	if (platform == P_NONE)
		init_light();
	
	switch (platform)
	{
	case P_IPAQ:
		return ipaq_get_level();
	break;
	case P_ZAURUS:
		return zaurus_get_level();
	break;
	case P_CORGI:
		return corgi_get_level();
	break;
	case P_INTEGRAL:
		return integral_get_level();
	break;
	case P_SIMPAD_NEW:
		return simpad_new_get_level();
	break;
	case P_SIMPAD:
		return simpad_get_level();
	break;
	case P_GENERIC:
		return generic_get_level();
	break;
	default:
		return 0;
	break;
	}
  return 0;
}
