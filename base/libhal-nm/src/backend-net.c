/* backend-net.c - Simple /proc-based network backend implementation.
   Copyright (C) 2006 Milan Plzik <mmp@handhelds.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */



#include <stdio.h>
#include <string.h>
#include <glib-2.0/glib.h>
#include "libhal-nm-backend.h"
#include "backend-net.h"

typedef struct InterfaceInfo_s InterfaceInfo;

typedef enum {net_unknown = 0, net_80203, net_80211 } InterfaceType;

struct InterfaceInfo_s 
{
	char if_name[16];
	InterfaceType type;
	gboolean is_usb;
};

#define INTERFACE_INFO(i) ((InterfaceInfo *)(i))

const int proc_net_dev_line = 255 ;

static GList *network_devices = NULL;
static int network_devices_count = 0, wireless_devices_count = 0;
static gshort *id;

/*
 * Utility routines
 */

/* Skips leading spaces in string */
static gchar *
skip_ws (gchar *s)
{
	while (*s == ' ') s++;
	return s;
}

/* Finds interface identified by name */
static InterfaceInfo *
find_interface_by_name (const gchar *if_name)
{
	GList *device = network_devices;

	while ((device != NULL) 
	        && (strcmp (INTERFACE_INFO (device->data)->if_name, if_name) ))
		device = g_list_next (device);

	if (device == NULL)
		return NULL;

	return INTERFACE_INFO (device->data);
}

/* Initializes the backend, reads initial information about interfaces. */
static gboolean 
net_init (void)
{
	FILE *dev, *wireless;
	char line[proc_net_dev_line + 1], *if_name;
	InterfaceInfo *device;
	char *colon_position;

	if (network_devices != NULL) {
		g_warning("net_init called more than once");
		return TRUE;
	}
	

	/* Read all known interfaces from /proc/net/dev */

	dev = fopen ("/proc/net/dev", "r");

	if (dev == NULL) {
		g_warning ("Could not open /proc/net/dev, no devices will be available.");
		return FALSE;
	}

	/* In /proc/net/dev, the first two lines are just description of
	 * columns - we can ignore them.
	 * WARNING: This currently does not handle differences between physical
	 * interfaces and tunnels in satisfiable manner - for now it is just
	 * putting put interfaces like lo, sit* and so on.
	 */

	if ((strlen (fgets(line, sizeof(line), dev)) >= proc_net_dev_line - 1) 
	   || (strlen (fgets(line, sizeof(line), dev)) >= proc_net_dev_line - 1))
		g_error ("The /proc/net/dev file is not in known format");

	while (!feof (dev)) {
		//fgets (line, sizeof(line), dev);
		
		if (fgets (line, sizeof(line), dev) == NULL)
			break;

		if (strlen (line) >= proc_net_dev_line -1)
			g_error("The /proc/net/dev/file device line contains devicermations in unknown format");

		/* the occurence of first colon in the file marks end of the
		 * interface name */

		colon_position = strchr (line, ':');
		if ((long)(colon_position - line) >= sizeof (device->if_name))
			g_error("Interface name too long");

		/* Get the interface name */
		*colon_position = 0;
		g_strchomp (line); 
		if_name = skip_ws (line);
		
		/* Check the device against the blacklist */
		if (!strcmp (if_name, "lo")
		    || !strncmp (if_name, "sit", 3))
			continue;

		/* Interface names are aligned to the right, so skip initial
		 * whitespace - that's what skip_ws does. */
		device = g_malloc0 (sizeof (InterfaceInfo));
		strcpy (device->if_name, if_name);

		/* AFAIK there is no reliable way of knowing what kind of device
		 * is the interface we are working with */
		device->type = net_80203;
		network_devices = g_list_append (network_devices, device);
		network_devices_count ++;
	}

	fclose (dev);

	/* Next, we need to identify which devices do support Wireless
	 * Extensions. For this we will use the /proc/net/wireless file. */

	wireless = fopen ("/proc/net/wireless", "r");



	if ( wireless ) {
		/* Skip initial header in file */
		
		if ((strlen (fgets(line, sizeof(line), wireless)) >= proc_net_dev_line - 1) 
		   || (strlen (fgets(line, sizeof(line), wireless)) >= proc_net_dev_line - 1))
			g_error ("The /proc/net/wireless file is not in known format");

		while ( !feof (wireless) ) {

			if (fgets (line, sizeof(line), wireless) == NULL)
				break;
			
			if (strlen (line) >= proc_net_dev_line - 1)
				g_error("The /proc/net/dev/file device line contains devicermations in unknown format");

			/* Truncate the string so it will contain only data
			 * before the colon and remove leading spaces. */
			colon_position = strchr (line, ':');
			*colon_position = 0;
			if_name = skip_ws (line);
			
			device = find_interface_by_name (if_name);
			
			/* Device must have occured in /proc/net/dev. If it has
			 * not, there is probably something wrong with the
			 * system. */

			
			if (device == NULL) continue;	/* FIXME: remove after removing wlan0 from blacklist */
			g_assert (device != NULL);

			device->type = net_80211;
			wireless_devices_count ++;
		};

		fclose (wireless);

	} else
		g_warning ("Can not open /proc/net/wireless, no device will be 802.11 capable.");


	return TRUE;
};

static void
net_exit (void)
{
	GList *device;
	/* Free list of interface informations */

	for (device = network_devices; device != NULL; device = g_list_next (device))
		g_free (device->data);

	g_list_free (network_devices);

	network_devices = NULL;
}


static gchar **
net_find_by_capa (const gchar *capa, guint32 *devices_n)
{
        gchar ** ret;
        gchar ** end;
	InterfaceType dev_type;
	GList *dev;
	guint32 required_mem;

        g_debug ("net_find_by_capa ('%s', %p)",
                 capa, devices_n);


        /* This should call internal function, which will enumerate all
         * available interfaces and deliver them here. */

        if (strncmp (capa, "net", 3)) {
		*devices_n = 0;
		g_debug ("No devicess with capability '%s'", capa);
		return NULL;
	};

	/* select what kind of device will be returned */
	if (0) {
	} else if (!strcmp (capa, "net.80203")) {
		dev_type = net_80203;
	} else if (!strcmp (capa, "net.80211")) {
		dev_type = net_80211;
	} else dev_type = (InterfaceType)(-1);

	/* allocate required memory - we keep track about all counts we need */
	switch (dev_type) {
		case -1:
			required_mem = network_devices_count;
			break;
		case net_80203:
			required_mem = network_devices_count - wireless_devices_count;
			break;
		case net_80211:
			required_mem = wireless_devices_count;
			break;
		default:
			g_assert_not_reached ();
			break;
	};

	ret = g_malloc ((required_mem + 1) * sizeof (gchar *));
	end = ret;

	/* create list of interfaces to return */
	for (dev = network_devices;  dev != NULL; dev = g_list_next (dev)) {
		if ( (dev_type != -1) && (dev_type != INTERFACE_INFO(dev->data)->type) ) continue;
		*end = g_strdup_printf("%d/%s", *id, INTERFACE_INFO(dev->data)->if_name);
		g_assert ( (end - ret) <= required_mem );
		g_print( "%s\n", *end);
		end++;

	};
	
	*end = NULL;
	*devices_n = required_mem;

        g_debug ("libhal_find_device_by_capability returns %p and count %i", ret, *devices_n);
        return ret;

}

static gboolean 
net_property_exists (const gchar *device, const gchar *prop) 
{
	InterfaceInfo *dev;
	char *dname;
	
	dname = strchr (device, '/') + 1;

	g_debug ("getting property '%s' for '%s'", prop, device);

	if (!strncmp (dname, "physdev_", 8)) {
		/* handle request for physdev */
		if (!strcmp (prop, "info.linux.driver")) 
			return TRUE;
	} else {

		/* Find out whether requested device is in our list */
		dev = find_interface_by_name (dname);
	
		if (dev == NULL) return FALSE;
		if (!strcmp (prop, "info.category")
	            || !strcmp (prop, "net.interface")
	            /* || !strcmp (prop, "info.linux.driver") */ /* this is for physical
								    device */
	            || !strcmp (prop, "net.physical_device"))
	                return TRUE;
	        /*if (!strcmp (prop, "usb.class.interface") )
	                if (dev->type == net_80211)
	                        return TRUE; */
	};
	return FALSE;
}

static gchar *
net_get_prop_string (const gchar *device, const gchar *prop)
{
	InterfaceInfo *dev;
	char *dname;


	dname = strchr (device, '/') +1;


	if (!strncmp (dname, "physdev_", 8)) {
		/* handle physical device request */
		if (0) {
		} else if (!strcmp (prop, "info.linux.driver")) {
			g_debug("returning kernel24");
			return g_strdup ("kernel24");
		};

	} else {
		/* handle network interface request */
		
		/* Find out whether requested device is in our list */
		dev = find_interface_by_name (dname);
	
		if (dev == NULL) return FALSE;

		g_debug ("getting property %s for %s", prop, device);
	
	        if (0) {
	        } else if (!strcmp (prop, "info.category")) {
			switch (dev->type) {
				case net_80211:
					g_debug ("returning net.80211");
					return g_strdup ("net.80211");
				case net_80203:
					g_debug ("returning net.80203");
					return g_strdup ("net.80203");
				case net_unknown:
					g_debug ("returning net");
					return g_strdup ("net");
				default:
					g_assert_not_reached ();
					break;
			};
	        } else if (!strcmp (prop, "net.interface")) {
			g_debug ("returning '%s'", dev->if_name);
	                return g_strdup (dev->if_name);
		} else if (!strcmp (prop, "net.physical_device")) {
	                return g_strdup_printf ("%d/physdev_%s", *id, dev->if_name);
	        }
	};

	g_assert_not_reached ();
	return NULL;
}

/* registers backend in libhal-nm */
void 
backend_net_main (void)
{
	static BackendIface interface= {
		.name = "net",
		.init = net_init,
		.exit = net_exit,
		.find_by_capability = net_find_by_capa,
		.property_exists = net_property_exists,
		.get_property_string = net_get_prop_string
	};

	libhal_nm_backend_register (&interface);
	id = &interface.id;
}
