/*
 * gpe-conf
 *
 * Copyright (C) 2006, 2007, 2008  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Device detection module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)
#include <gtk/gtk.h>

#include "device.h"

#define DEVICE_FILE PREFIX "/share/gpe-conf/deviceinfo"

typedef struct
{
	DeviceID_t id;
	gchar **pattern;
	gchar *name;
	gchar *model;
	gchar *manufacturer;
	gchar *type;
	gint features;
} Device_t;

static gint local_device_id = -1;
static Device_t local_device;
static Device_t *DeviceMap = NULL;
static gint maplen = 0;

static void
device_copy (Device_t *src, Device_t *dst)
{
	dst->id = src->id;
	if (src->name) dst->name = g_strdup (src->name);
	if (src->model) dst->model = g_strdup (src->model);
	if (src->type) dst->type = g_strdup (src->type);
	if (src->manufacturer) dst->manufacturer = g_strdup (src->manufacturer);
	dst->features = src->features;
}

static void
devices_free (void)
{
	gint i;
	
	for (i=0; i < maplen; i++)
	{
		Device_t dev = DeviceMap[i];
		
		if (dev.pattern) g_strfreev (dev.pattern);
		if (dev.name) g_free (dev.name);
		if (dev.model) g_free (dev.model);
		if (dev.type) g_free (dev.type);
		if (dev.manufacturer) g_free (dev.manufacturer);
	}
	
	g_free (DeviceMap);
	DeviceMap = NULL;
}

static gboolean
devices_load(void)
{
	GKeyFile *devicefile;
	GError *err = NULL;
	gchar **devices;
	guint i;
	gsize devcnt;
	
	devicefile = g_key_file_new();
	
	if (!g_key_file_load_from_file (devicefile, DEVICE_FILE,
                                    G_KEY_FILE_NONE, &err))
	{
		g_printerr ("%s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
	
	devices = g_key_file_get_groups (devicefile, &devcnt);

	if (devices)
	{
		DeviceMap = g_malloc0 (sizeof(Device_t) * (devcnt + 1));
	
		for (i=0; i < devcnt; i++)
		{
			Device_t *dev = &DeviceMap[i];
			
			dev->pattern = g_key_file_get_string_list (devicefile, devices[i], 
			                                          "Pattern", NULL, NULL);
			dev->id = g_key_file_get_integer (devicefile, devices[i], 
			                                 "ID", NULL);
			dev->name = g_key_file_get_string (devicefile, devices[i], 
			                                  "Name", NULL);
			dev->model = g_key_file_get_string (devicefile, devices[i], 
			                                   "Model", NULL);
			dev->type = g_key_file_get_string (devicefile, devices[i], 
			                                  "Type", NULL);
			dev->manufacturer = g_key_file_get_string (devicefile, devices[i],
			                                          "Manufacturer", NULL);
			dev->features = g_key_file_get_integer (devicefile, devices[i], 
			                                       "Features", NULL);
		}
		g_strfreev(devices);
		maplen = devcnt;
	}

	return TRUE;
}


DeviceID_t 
device_get_id (void)
{
	static gint id = -1;
	gchar **strv = NULL;
	gsize len = 0;
	gint i = 0;
	gint dnr, pnr;
	gchar *iptr, *str = NULL;
	
	if (id != -1)
		return id;
	
	if (DeviceMap == NULL)
		devices_load ();
	
	/* get device info from /proc/cpuinfo */
	if (g_file_get_contents("/proc/cpuinfo", &str, &len, NULL))
	{
		strv = g_strsplit(str, "\n", 128);
		g_free(str);
		while (strv[i])
		{
			if ((iptr = strstr(strv[i], "Hardware")) 
			     || (iptr = strstr(strv[i], "system type")))
			{
				for (dnr = 0; dnr < maplen; dnr++)
				{
					pnr = 0;
		                	while ((DeviceMap[dnr].pattern) && (DeviceMap[dnr].pattern[pnr]))
					{
						if (strstr (iptr, DeviceMap[dnr].pattern[pnr]))
						{
							id = DeviceMap[dnr].id;
							if (DeviceMap[dnr].name == NULL)
								DeviceMap[dnr].name = g_strdup(DeviceMap[dnr].pattern[pnr]);
							device_copy (&DeviceMap[dnr], &local_device);
							goto out;
						}
						else
							pnr++;
					}
				}
			}
			else 
			if (strstr (strv[i], "fdiv_bug"))
			{
				id = DEV_X86;
				if (id < maplen)
					device_copy (&DeviceMap[id], &local_device);
				else  
					g_printerr ("Device data not available.");
				goto out;
			}
			else 
			if (strstr (strv[i], "machine"))
			{
				id = DEV_POWERPC;
				if (id < maplen)
					device_copy (&DeviceMap[id], &local_device);
				else 
					g_printerr ("Device data not available.");
				goto out;
			}
			else
			if (strstr (strv[i], "promlib"))
			{
				id = DEV_SPARC;
				if (id < maplen)
					device_copy (&DeviceMap[id], &local_device);
				else 
					g_printerr ("Device data not available.");
				goto out;
			}
			else
			if (strstr (strv[i], ": Alpha"))
			{
				id = DEV_ALPHA;
				if (id < maplen)
					device_copy (&DeviceMap[id], &local_device);
				else 
					g_printerr ("Device data not available.");
				goto out;
			}
			
			i++;
		}
	}
	
	id = DEV_UNKNOWN;
	local_device.name = g_strdup(_("Unknown Device"));

out: 
	local_device_id = id;
	if (strv) 
		g_strfreev(strv);
	devices_free ();

	return id;
}

DeviceFeatureID_t
device_get_features (void)
{
	if (local_device_id < 0)
		device_get_id();
	
	return local_device.features;
}

const gchar *
device_get_name (void)
{
	if (local_device_id < 0)
		device_get_id();
	
	return local_device.name;
}

const gchar *
device_get_type (void)
{
	if (local_device_id < 0)
		device_get_id();
	
	return local_device.type;
}

const gchar *
device_get_manufacturer (void)
{
	if (local_device_id < 0)
		device_get_id();
	
	return local_device.manufacturer;
}

gchar *
device_get_specific_file (const gchar *basename)
{
    gchar *result;
    const gchar *name = device_get_name ();
    
    /* check for a device specific file per name first */
    if (name != NULL)
    {
        result = g_strdup_printf ("%s.%s", basename, name);
        if (!access (result, R_OK))
            return result;
        g_free (result);
    }
    
    /* try device number then */
    result = g_strdup_printf ("%s.%d", basename, device_get_id());
    if (!access (result, R_OK))
        return result;
    g_free (result);
    
    /* try device type then */
    
    result = g_strdup_printf ("%s.%s", basename, device_get_type());
    if (!access (result, R_OK))
        return result;
    g_free (result);
    
    /* maybe add class here later */
    
    return NULL;
}
