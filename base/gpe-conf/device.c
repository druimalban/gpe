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

typedef struct
{
	gint id;
	gchar *pattern[5];
}DeviceMap_t;

static DeviceMap_t DeviceMap[] = { \
	{ DEV_IPAQ_SA, { "HP iPAQ H3100", "HP iPAQ H3600", "HP iPAQ H3700", "HP iPAQ H3800" } }, \
	{ DEV_IPAQ_PXA, {  "HP iPAQ H5400",  "HP iPAQ H2200",  "HP iPAQ HX4700" } },
	{ DEV_SIMPAD, { "Simpad" } },
	{ DEV_RAMSES, { "Ramses" } },
	{ DEV_ZAURUS_COLLIE, { "Sharp-Collie", "Collie" } },
	{ DEV_ZAURUS_POODLE, { "Poodle" } },
	{ DEV_ZAURUS_SHEPHERED, { "SHARP Shepherd" } },
	{ DEV_ZAURUS_HUSKY, { "SHARP Husky" } },
	{ DEV_ZAURUS_CORGI, { "SHARP Corgi" } },
	{ DEV_ZAURUS_SPITZ, { "SHARP Spitz" } },
	{ DEV_ZAURUS_AKITA, { "SHARP Akita" } },
	{ DEV_ZAURUS_BORZOI, { "SHARP Borzoi" } },
	{ DEV_NOKIA_770, { "Generic OMAP1510/1610/1710" } },
	{ DEV_NETBOOK_PRO, { "Psion Teklogix NetBookPro" } },
	{ DEV_HW_INTEGRAL, { "HW 90200" } },
	{ DEV_CELLON_8000, { "Cellon C8000 Board" } },
	{ DEV_JOURNADA, { "HP Jornada 720", "HP Jornada 820", "HP Jornada 56X" } },
	{ DEV_SGI_O2, { "SGI O2" } },
	{ DEV_SGI_INDY, { "SGI Indy" } },
	{ DEV_SGI_INDIGO2, { "SGI Indigo2" } },
	{ DEV_SGI_OCTANE, { "SGI IP30", "SGI Octane" } },
};


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
	if (g_file_get_contents("/dev/cpuinfo", &str, &len, NULL))
	{
		strv = g_strsplit(str, "\n", 128);
		g_free(str);
		while (strv[i])
		{
			if ((iptr = strstr(strv[i], "Hardware")) 
			     || (iptr = strstr(strv[i], "system type")))
			{
				for (dnr = 0; dnr < (DEV_MAX - 2); dnr++)
				{
					pnr = 0;
					while (DeviceMap[dnr].pattern[pnr])
					{
						if (strstr (iptr, DeviceMap[dnr].pattern[pnr]))
						{
							id = DeviceMap[dnr].id;
							g_strfreev(strv);
							return id;
						}
						else
							pnr++;
					}
				}
			}
			i++;
		}
		g_strfreev(strv);
	}
	
	id = DEV_UNKNOWN;
	return id;
}
