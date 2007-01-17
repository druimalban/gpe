/*
 * gpe-conf
 *
 * Copyright (C) 2006  Florian Boor <florian.boor@kernelconcepts.de>
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

static const gchar *device_name = NULL;
static const gchar *device_domain = NULL;
static Device_t *device_map = NULL;

typedef struct
{
	DeviceID_t id;
	gchar **pattern;
	gchar *name;
	gchar *model;
	gchar *manufacturer;
    gchar *type; /* was domain */
	gint features;
} Device_t;

static DeviceMap_t DeviceMap[] = { \
	{ DEV_IPAQ_SA, { "HP iPAQ H3100", "HP iPAQ H3600", "HP iPAQ H3700", "HP iPAQ H3800" }, "ipaq" }, \
	{ DEV_IPAQ_PXA, {  "HP iPAQ H5400",  "HP iPAQ H2200",  "HP iPAQ HX4700" }, "ipaq"},
	{ DEV_SIMPAD, { "Simpad" }, "simpad" },
	{ DEV_RAMSES, { "Ramses" } ,"ramses" },
	{ DEV_ZAURUS_COLLIE, { "Sharp-Collie", "Collie" }, "zaurus-portrait" },
	{ DEV_ZAURUS_POODLE, { "Poodle" }, "zaurus-clamshell" },
	{ DEV_ZAURUS_SHEPHERED, { "SHARP Shepherd" }, "zaurus-clamshell" },
	{ DEV_ZAURUS_HUSKY, { "SHARP Husky" }, "zaurus-clamshell" },
	{ DEV_ZAURUS_CORGI, { "SHARP Corgi" }, "zaurus-clamshell" },
	{ DEV_ZAURUS_SPITZ, { "SHARP Spitz" }, "zaurus-clamshell" },
	{ DEV_ZAURUS_AKITA, { "SHARP Akita" }, "zaurus-clamshell" },
	{ DEV_ZAURUS_BORZOI, { "SHARP Borzoi" }, "zaurus-clamshell" },
	{ DEV_NOKIA_770, { "Generic OMAP1510/1610/1710" }, "nokia-tablet" },
	{ DEV_NETBOOK_PRO, { "Psion Teklogix NetBookPro" }, "netbook" },
	{ DEV_HW_INTEGRAL, { "HW 90200" }, "hwmde" },
	{ DEV_CELLON_8000, { "Cellon C8000 Board" }, "small-phone" },
	{ DEV_JOURNADA, { "HP Jornada 720", "HP Jornada 820", "HP Jornada 56X" }, "jornada" },
	{ DEV_SGI_O2, { "SGI O2" }, "workstation" },
	{ DEV_SGI_INDY, { "SGI Indy" }, "workstation" },
	{ DEV_SGI_INDIGO2, { "SGI Indigo2" }, "workstation" },
	{ DEV_SGI_OCTANE, { "SGI IP30", "SGI Octane" }, "workstation" },
	{ DEV_HTC_UNIVERSAL, { "HTC Universal" }, "smartphone" },
	{ DEV_ETEN_G500, { "Eten G500" }, "smartphone" },
	{ DEV_HW_SKEYEPADXSL, { "HW90350" }, "hwmde" },
};

/* Keep in sync with DeviceId_t defintion. */
static DeviceID_t IdClassVector[] = 
{
	DEVICE_CLASS_NONE,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_TABLET,
	DEVICE_CLASS_MDE,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_TABLET,
	DEVICE_CLASS_MDE,
	DEVICE_CLASS_MDE,
	DEVICE_CLASS_CELLPHONE,
	DEVICE_CLASS_TABLET,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PDA | DEVICE_CLASS_CELLPHONE,
	DEVICE_CLASS_TABLET,
};

static gboolean
devices_load(void)
{
	GKeyFile *devicefile;
	GError *err = NULL;
	gchar **devices;
	guint i, devcnt;
	
	devicefile = g_key_file_new();
	
	if (!g_key_file_load_from_file (devicefile, DEVICE_FILE,
                                    G_KEY_FILE_NONE, &err))
	{
		g_printerr ("%s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
	
	devices = g_key_file_get_keys (devicefile, "Devices",  &devcnt, &err);
	if (devices)
	{
		device_map = g_malloc0 (sizeof(Device_t) * devcnt);

		for (i=0; i < devcnt; i++)
		{
			Device_t *dev = device_map[i];
			
			dev->pattern = g_key_file_get_string_list (devicefile, "Pattern",
                                                      devices[i], NULL, NULL);
			dev->id = g_key_file_get_integer (devicefile, "ID",
                                                      devices[i], NULL, NULL);
			dev->name = g_key_file_get_string (devicefile, "Name",
                                                      devices[i], NULL, NULL);
			dev->model = g_key_file_get_string (devicefile, "Model",
                                                      devices[i], NULL, NULL);
			dev->type = g_key_file_get_string (devicefile, "Type",
                                                      devices[i], NULL, NULL);
			dev->manufacturer = g_key_file_get_string (devicefile, "Manufacturer",
                                                      devices[i], NULL, NULL);
			dev->features = g_key_file_get_integer (devicefile, "Features",
                                                      devices[i], NULL, NULL);
		}
		g_strfreev(devices);
	}
	return TRUE;
}


DeviceID_t 
device_get_id (void)
{
	static gint id = -1;
	gchar **strv;
	guint len = 0;
	gint i = 0;
	gint dnr, pnr;
	gchar *iptr, *str = NULL;
	
	if (id != -1)
		return id;
	
	/* get device info from /proc/cpuinfo, only ARM and MIPS for now */
	if (g_file_get_contents("/proc/cpuinfo", &str, &len, NULL))
	{
		strv = g_strsplit(str, "\n", 128);
		g_free(str);
		while (strv[i])
		{
			if ((iptr = strstr(strv[i], "Hardware")) 
			     || (iptr = strstr(strv[i], "system type")))
			{
				for (dnr = 0; dnr < sizeof(DeviceMap)/sizeof(*DeviceMap); dnr++)
				{
					pnr = 0;
					while (DeviceMap[dnr].pattern[pnr])
					{
						if (strstr (iptr, DeviceMap[dnr].pattern[pnr]))
						{
							id = DeviceMap[dnr].id;
                            device_name = DeviceMap[dnr].pattern[pnr];
                            device_domain = DeviceMap[dnr].domain;
							g_strfreev(strv);
							return id;
						}
						else
							pnr++;
					}
				}
			}
			
			if (strstr (strv[i], "fdiv_bug"))
			{
				id = DEV_X86;
                device_name = "Generic x86 PC";
                device_domain = "workstation";
				g_strfreev(strv);
				return id;
			}
			
			if (strstr (strv[i], "machine"))
			{
				id = DEV_POWERPC;
                device_name = "PowerPC";
                device_domain = "workstation";
				g_strfreev(strv);
				return id;
			}
			
			if (strstr (strv[i], "promlib"))
			{
				id = DEV_SPARC;
                device_name = "Sparc";
                device_domain = "workstation";
				g_strfreev(strv);
				return id;
			}
			
			if (strstr (strv[i], ": Alpha"))
			{
				id = DEV_ALPHA;
                device_name = "Alpha";
                device_domain = "workstation";
				g_strfreev(strv);
				return id;
			}
			
			i++;
		}
		g_strfreev(strv);
	}
	
    device_name = "Unknown Device";
    device_domain = "unknown";
	id = DEV_UNKNOWN;

	return id;
}

DeviceClassID_t
device_get_class_id (DeviceID_t device_id)
{
	if ((device_id >= 0) && (device_id < DEV_MAX))
		return 
			IdClassVector [device_id];
	else
		return
			DEVICE_CLASS_NONE;
}

const gchar *
device_get_name (void)
{
    if (device_name == NULL)
        device_get_id ();
    
    return device_name;        
}

const gchar *
device_get_domain (void)
{
    if (device_domain == NULL)
        device_get_id ();
    
    return device_domain;        
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
    
    /* try device domain then */
    
    result = g_strdup_printf ("%s.%s", basename, device_get_domain());
    if (!access (result, R_OK))
        return result;
    g_free (result);
    
    /* maybe add class here later */
    
    return NULL;
}
