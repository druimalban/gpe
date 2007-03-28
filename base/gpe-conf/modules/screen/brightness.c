/*
 * gpe-conf
 *
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>, 
 *   Pierre TARDY <tardyp@free.fr>
 * Copyright (C) 2004, 2007 Florian Boor <florian.boor@kernelconcepts.de>
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
#include <dirent.h>

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
    P_GENERIC,
	P_SYSCLASS_770,
	P_SYSCLASS
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

/* Linux 2.6 sysfs interface */
#define SYSCLASS 	"/sys/class/backlight/"
#define SYS_STATE_ON  0
#define SYS_STATE_OFF 4
static gchar *SYS_BRIGHTNESS = NULL;
static gchar *SYS_MAXBRIGHTNESS = NULL;
static gchar *SYS_POWER = NULL;

/* uncommon sysclass path on Nokia 770 */
#define SYSCLASS_770 	"/sys/devices/platform/omapfb/panel/"

static t_platform platform = P_NONE;

static gchar
*get_sysclass_bl(void)
{
	DIR *dp;
	gchar *dentry;
	struct dirent *d;

	if((dp = opendir(SYSCLASS)) == NULL) {
		fprintf(stderr, "unable to open %s", SYSCLASS);
		return NULL;
	}

	while((d = readdir(dp))) {
		if (!(strcmp(d->d_name, ".") == 0) &&
			!(strcmp(d->d_name, "..") == 0))
			asprintf(&dentry, "%s%s", SYSCLASS, d->d_name);
	}
	closedir(dp);
	return dentry;
}

static void
setup_sysclass(void)
{
	gchar *bl_dev;
	
	bl_dev = get_sysclass_bl();
	
	SYS_BRIGHTNESS = g_strdup_printf("%s/brightness", bl_dev);
	SYS_MAXBRIGHTNESS = g_strdup_printf("%s/max_brightness", bl_dev);
	SYS_POWER = g_strdup_printf("%s/power", bl_dev);
}

static void
setup_sysclass_770(void)
{
	SYS_BRIGHTNESS = g_strdup_printf("%s/backlight_level", SYSCLASS_770);
	SYS_MAXBRIGHTNESS = g_strdup_printf("%s/backlight_max", SYSCLASS_770);
	SYS_POWER = NULL;
}

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
	if (!access(SYSCLASS_770, R_OK))
	{
		setup_sysclass_770();
		return P_SYSCLASS_770;
	}
	if (!access(SYSCLASS, R_OK))
	{
		setup_sysclass();
		return P_SYSCLASS;
	}
	
	return P_NONE;
}

void 
backlight_init(void)
{
	platform = detect_platform();
}


void 
sysclass_set_power(gboolean power)
{
  FILE *f_power;

  f_power = fopen(SYS_POWER, "w");
  if (f_power != NULL)
  {
    fprintf(f_power,"%d\n",  power ? SYS_STATE_ON : SYS_STATE_OFF);
  	fclose(f_power);
  }
}


void
ipaq_set_power(gboolean power)
{
	int light_fd;
	struct h3600_ts_backlight bl;
	
	light_fd = open (TS_DEV, O_RDONLY);
	if (light_fd > 0) 
	{
		bl.brightness = (unsigned char)ipaq_get_level();
	
		if (power)
			bl.power = FLITE_PWR_ON;
		else
			bl.power = FLITE_PWR_OFF;
	
		if (ioctl (light_fd, TS_SET_BACKLIGHT, &bl) != 0)
			g_warning ("Changing backlight power state failed.\n");
		
		close(light_fd);
	}
}

gboolean 
sysclass_get_power(void)
{
  FILE *f_power;
  gint value = SYS_STATE_ON;
  
  f_power = fopen(SYS_POWER, "r");
  if (f_power != NULL)
  {
  	fscanf(f_power,"%i", &value);
  	fclose(f_power);
	  
	return (value == SYS_STATE_ON) ? TRUE : FALSE;
  }
  return TRUE; /* we default to have backlight on */
}  

gboolean
ipaq_get_power(void)
{
	int light_fd;
	struct h3600_ts_backlight bl;

	light_fd = open (TS_DEV, O_RDONLY);
	if (light_fd > 0) {
 		if (ioctl (light_fd, TS_GET_BACKLIGHT, &bl) != 0)
		{
			close(light_fd);
    		return TRUE; /* in case of failure we assume to have backlight on */
		}
  		else
		{
			close(light_fd);
    		return (bl.power == FLITE_PWR_ON) ? TRUE : FALSE;
		}
	}
	return TRUE;
}


gint 
sysclass_set_level(gint level)
{
  FILE *f_light;
  FILE *f_max;
  gint val, maxlevel;
  
  if ( level > 255 ) level = 255;
  if ( level < 1 ) level = 1;

  f_max = fopen(SYS_MAXBRIGHTNESS, "r");
  if (f_max != NULL)
  {
    fscanf(f_max,"%d", &maxlevel);
    fclose(f_max);
  }
  val = ( level == 1 ) ? 1 : ( level * maxlevel ) / 255;
  if (val > maxlevel)
  {
    val = maxlevel;
  }
  f_light = fopen(SYS_BRIGHTNESS, "w");
  if (f_light != NULL)
  {
    fprintf(f_light,"%d\n", val);
  	fclose(f_light);
  }
  else
	  return -1;
  
  f_light = fopen(SYS_POWER, "w");
  if (f_light != NULL)
  {
    fprintf(f_light,"%d\n",  level ? SYS_STATE_ON : SYS_STATE_OFF);
  	fclose(f_light);
  }
  
  return level;
}

int 
sysclass_get_level(void)
{
  FILE *f_light;
  FILE *f_max;
  gfloat level, maxlevel, factor;
  
  f_max = fopen(SYS_MAXBRIGHTNESS, "r");
  if (f_max != NULL)
  {
    fscanf(f_max,"%f", &maxlevel);
    fclose(f_max);
  }
  factor = 255/maxlevel;
  f_light = fopen(SYS_BRIGHTNESS, "r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%f", &level);
  	fclose(f_light);
    level = level * factor;
    if (level > 255)
    {
      level = 255;
    }
	return (int)level;
  }
  return -1;
}  

int 
generic_set_level(int level)
{
  FILE *f_light;
  gint val = level/8;
  
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

gint 
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

gint 
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

gint 
simpad_new_get_level(void)
{
  FILE *f_light;
  gint level;
  
  f_light = fopen(SIMPAD_BACKLIGHT_REG_NEW,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%d", &level);
  	fclose(f_light);
	return level;
  }
  return -1;
}  


gint 
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

gint 
simpad_get_level(void)
{
  FILE *f_light;
  gint level;
  
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

gint 
corgi_set_level(int level)
{
	gint fd, val, len, res = -1;
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

gint 
corgi_get_level(void)
{
	gint val, res;
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

gint 
zaurus_set_level(gint level)
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
gint 
zaurus_get_level(void)
{
	return 25;	
}


gint 
ipaq_set_level(gint level)
{
	gint light_fd;
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

gint 
integral_set_level(gint level)
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

gint 
integral_get_level(void)
{
  FILE *f_light;
  gint level;
  
  f_light = fopen(PROC_LIGHT,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%i", &level);
  	fclose(f_light);
	return level;
  }
  return -1;
}  


void 
backlight_set_brightness (gint brightness)
{
	if (brightness < 0)
		brightness = 0;
	if (brightness > 255)
		brightness = 255;
	
	if (platform == P_NONE)
		backlight_init();
	
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
	case P_SYSCLASS:
	case P_SYSCLASS_770:
		sysclass_set_level(brightness);
	break;	
	default:
	break;
	}
}


gint 
backlight_get_brightness (void)
{
	
	if (platform == P_NONE)
		backlight_init();
	
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
	case P_SYSCLASS:
	case P_SYSCLASS_770:
		return sysclass_get_level();
	break;	
	default:
		return 0;
	break;
	}
  return 0;
}

gboolean
backlight_get_power (void)
{
	if (platform == P_NONE)
		backlight_init();
	
	switch (platform)
	{
	case P_IPAQ:
		return ipaq_get_power();
	break;
	case P_SYSCLASS:
	case P_SYSCLASS_770:
		return sysclass_get_power();
	break;	
	default:
		return TRUE;
	break;
	}
}

void
backlight_set_power (gboolean power)
{
	if (platform == P_NONE)
		backlight_init();
	
	switch (platform)
	{
	case P_IPAQ:
		ipaq_set_power(power);
	break;
	case P_SYSCLASS:
	case P_SYSCLASS_770:
		return sysclass_set_power(power);
	break;	
	default:
		return;
	break;
	}
}
