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
static gboolean enabled = TRUE;
static gboolean automatic = TRUE;
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
	GKeyFile *alarmfile;
	
	alarmfile = g_key_file_new();
	
	g_key_file_set_boolean (alarmfile, "Settings", "enabled", enabled);
	g_key_file_set_boolean (alarmfile, "Settings", "automatic", automatic);
	g_key_file_set_integer (alarmfile, "Settings", "level", level);
	
	cfgfile = fopen(FILE_SETTINGS, "w");
	if (cfgfile)
	{
		fprintf(cfgfile, 
		        "# alarm settings - file created by gpe-conf\n\n");
		fprintf(cfgfile, "%s\n", g_key_file_to_data (alarmfile, NULL, NULL));
		fclose(cfgfile);
	}
	if (alarmfile)
		g_key_file_free (alarmfile);
}


void 
alarm_load_settings(void)
{
	GKeyFile *alarmfile;
	GError *err = NULL;
	gint i;
	
	alarmfile = g_key_file_new();
	
	if (!g_key_file_load_from_file (alarmfile, FILE_SETTINGS,
                                    G_KEY_FILE_NONE, &err))
	{
		g_printerr ("%s\n", err->message);
		g_error_free(err);
		if (alarmfile)
			g_key_file_free (alarmfile);
		return;
	}
	
	i = g_key_file_get_boolean (alarmfile, "Settings", "enabled", &err);
	if (err) 
	{
		g_error_free(err);
		err = NULL;
	}
	else
		enabled = i;
	
	i = g_key_file_get_boolean (alarmfile, "Settings", "automatic", &err);
	if (err) 
	{
		g_error_free(err);
		err = NULL;
	}
	else
		automatic = i;
	
	i = g_key_file_get_integer (alarmfile, "Settings", "level", &err);
	if (err) 
	{
		g_error_free(err);
		err = NULL;
	}
	else
		if ((i >= 0) && (i <=100))
			level = i;
	
	if (alarmfile)
		g_key_file_free (alarmfile);
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
