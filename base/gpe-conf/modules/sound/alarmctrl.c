/*
 * gpe-conf
 *
 * Copyright (C) 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE sound settings module, alarms backend.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#include "alarmctrl.h"
#include "soundctrl.h"

static const char *FILE_SETTINGS = NULL;
static int enabled = 1;
static int automatic = 1;
static int level = 80;
static int old_enabled;
static int old_automatic;
static int old_level;


void
alarm_init(void)
{
	FILE_SETTINGS = 
		g_strdup_printf("%s/.gpe/alarm.conf", g_get_home_dir());
	alarm_load_settings();
	old_enabled = enabled;
	old_level = level;
}


void 
alarm_save_settings(void)
{
	FILE *cfgfile;
	
	cfgfile = fopen(FILE_SETTINGS, "w");
	if (cfgfile)
	{
		fprintf(cfgfile, 
		        "# alarm settings - file created by gpe-conf\n\n");
		fprintf(cfgfile, "enabled %d\n", enabled);
		fprintf(cfgfile, "automatic %d\n", automatic);
		fprintf(cfgfile, "level %d\n", level);
		fclose(cfgfile);
	}
}


void 
alarm_load_settings(void)
{
	FILE *cfgfile;
	
	cfgfile = fopen(FILE_SETTINGS, "r");
	if (cfgfile)
	{
		int val = -1, ret;
		char buf[128];
		while (fgets(buf, 128, cfgfile))
		{
			ret = sscanf(buf, "enabled %d", &val);
			if (ret)
				enabled = val;
			
			ret = sscanf(buf, "automatic %d", &val);
			if (ret)
				automatic = val;
			
			ret = sscanf(buf, "level %d", &val);			
			if (ret)
			{
				if ((val >= 0) && (val <=100))
					level = val;
			}
		}
		fclose(cfgfile);
	}
}


void 
alarm_restore_settings(void)
{
	if (!FILE_SETTINGS) 
		return;
	level = old_level;
	enabled = old_enabled;
	automatic = old_automatic;
}

int 
get_alarm_level(void)
{
	return level;
}

void 
set_alarm_level(int newlevel)
{
	level = newlevel;
}

int 
get_alarm_enabled(void)
{
	return enabled;
}

void 
set_alarm_enabled(int newenabled)
{
	enabled = newenabled;
}

int 
get_alarm_automatic(void)
{
	return automatic;
}

void 
set_alarm_automatic(int newauto)
{
	automatic = newauto;
}
