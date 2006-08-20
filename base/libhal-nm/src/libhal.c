/* libhal.c - Event menu implementation.
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

/* Fake libhal API implementation 
 * This file contains portions of code from original libhal header file, written
 * by David Zeuthen, <david@fubar.dk>.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib-2.0/glib.h>
#include <dbus/dbus.h>
#include "libhal.h"
#include "libhal-nm-backend.h"

struct LibHalContext_s {
	void *user_data;

	DBusConnection *connection;
	dbus_bool_t watch_property_changes;

	LibHalDeviceAdded device_added;
	LibHalDeviceRemoved device_removed;
	LibHalDeviceNewCapability new_capability;
};


/* Create a new context for a connection with hald */
LibHalContext *
libhal_ctx_new (void)
{
	LibHalContext *ret;

	ret = (LibHalContext *) g_malloc0 (sizeof (LibHalContext));

	g_debug ("libhal_ctx_new return");

	return ret;
}

/* Initialize the connection to hald */
dbus_bool_t    
libhal_ctx_init (LibHalContext *ctx, DBusError *error)
{
	g_debug ("libhal_ctx_init (%p, %p)", ctx, error);

	g_return_val_if_fail ( ctx != NULL, FALSE);
	if (error) dbus_error_init (error);

	/* Here is temporary place to initialize the backend. It should move
	 * somewhere else, because there is no guarrantee that this will be
	 * called just once. 
	 */

	libhal_nm_backend_init ();

	g_debug ("libhal_ctx_init return");

	return ctx != NULL;
}

/* Set DBus connection to use to talk to hald. */
dbus_bool_t    
libhal_ctx_set_dbus_connection (LibHalContext *ctx, DBusConnection *conn)
{
	g_debug ("libhal_ctx_set_dbus_connection (%p, %p)", ctx, conn);
	g_return_val_if_fail (ctx != NULL, FALSE);
	g_return_val_if_fail (conn != NULL, FALSE);

	ctx->connection = conn;

	g_debug ("libhal_ctx_set_dbus_connection return");
	return TRUE;
}

/* Get user data for the context */
void *
libhal_ctx_get_user_data (LibHalContext *ctx)
{
	g_debug ("libhal_ctx_get_user_data (%p)", ctx);
	g_return_val_if_fail (ctx != NULL, NULL);

	g_debug ("libhal_ctx_get_user_data returns %p", ctx->user_data);
	return ctx->user_data;
}

/* Set user data for the context */
dbus_bool_t
libhal_ctx_set_user_data (LibHalContext *ctx, void *user_data)
{
	g_debug ("libhal_ctx_set_user_data (%p, %p)", ctx, user_data);
	g_return_val_if_fail (ctx != NULL, FALSE);
	ctx->user_data = user_data;

	g_debug ("libhal_ctx_set_user_data return");
	return TRUE;
}

/* Set the callback for when a device is added */
dbus_bool_t
libhal_ctx_set_device_added (LibHalContext *ctx, LibHalDeviceAdded callback)
{
	g_debug ("libhal_ctx_set_device_adder (%p, %p)", ctx, callback);
	g_return_val_if_fail (ctx != NULL, FALSE);
	ctx->device_added = callback;

	return TRUE;
}

/* Set the callback for when a device is removed */
dbus_bool_t
libhal_ctx_set_device_removed (LibHalContext *ctx, LibHalDeviceRemoved callback)
{
	g_debug ("libhal_ctx_set_device_removed (%p, %p)", ctx, callback);
	g_return_val_if_fail (ctx !=  NULL, FALSE);
	ctx->device_removed = callback;

	g_debug ("libhal_ctx_set_device_removed returns");
	return TRUE;
}

/* Set the callback for when a device gains a new capability */
dbus_bool_t
libhal_ctx_set_device_new_capability (LibHalContext *ctx, 
                                      LibHalDeviceNewCapability callback)
{
	g_debug ("libhal_ctx_set_device_new_capability (%p, %p)", ctx, callback);
	g_return_val_if_fail (ctx != NULL, FALSE);
	ctx->new_capability = callback;

	g_debug ("libhal_ctx_set_device_new_capability returns");
	return TRUE;
}

/* Watch all devices, ie. the device_property_changed callback is
 * invoked when the properties on any device changes.
 */
dbus_bool_t 
libhal_device_property_watch_all (LibHalContext *ctx, DBusError *error)
{
	g_debug ("libhal_device_property_watch_all (%p, %p)", ctx, error);
	g_return_val_if_fail (ctx != NULL, FALSE);
	if (error) dbus_error_init (error);

	ctx->watch_property_changes = TRUE;

	g_debug ("libhal_device_property_watch_all returns");
	return TRUE;
}
	

/* Find devices with a given capability. */
char **
libhal_find_device_by_capability (LibHalContext *ctx, const char *capability,
                                  int *num_devices, DBusError *error)
{
	gchar ** ret = NULL;
	int tmp, num;

	g_debug ("libhal_find_device_by_capability (%p, '%s', %p, %p)", 
	         ctx, capability, num_devices, error);

	g_return_val_if_fail (ctx != NULL, NULL);
	if (error) dbus_error_init (error);

	for (tmp = 0; tmp < backends_registered; tmp ++) {
		ret = backends[tmp]->find_by_capability (capability, &num);
		if (num != 0) break;
	};

	*num_devices = num;

	g_assert (ret != NULL);

	return ret;
}

/* Determine if a property on a device exists. */
dbus_bool_t 
libhal_device_property_exists (LibHalContext *ctx, const char *udi, 
                               const char *key, DBusError *error)
{
	int module;

	/* FIXME: should check for ctx validity */

	module = atoi (udi);
	
	return backends[module]->property_exists (udi, key);
}

/* Get the value of a property of type string. */
char *
libhal_device_get_property_string (LibHalContext *ctx, const char *udi,
                                   const char *key, DBusError *error) 
{
	int module;
	char *ret;

	/* FIXME: should check for ctx validity */

	module = atoi (udi);

	ret = backends[module]->get_property_string (udi, key);

	g_assert (ret != NULL);

	return ret;
}

/* Free a LibHalContext resource */
dbus_bool_t    
libhal_ctx_free (LibHalContext *ctx) 
{
	g_return_val_if_fail (ctx != NULL, FALSE);
	g_free (ctx);

	return TRUE;
}



/* Shut down a connection to hald */
dbus_bool_t    
libhal_ctx_shutdown (LibHalContext *ctx, DBusError *error)
{
	g_debug ("shutting down LibHalContext");

	g_return_val_if_fail (ctx != NULL, FALSE);
	if (error) dbus_error_init (error);
	libhal_nm_backend_exit ();
	
	g_debug ("libhal_ctx_shutdown returns");
	return TRUE;
}	
	
/* Frees a NULL-terminated array of strings. If passed NULL, does nothing. */
void 
libhal_free_string_array (char **str_array)
{
	char ** s;

	if (str_array == NULL) return;

	for (s = str_array; *s != NULL; s ++) 
		g_free (*s);

	g_free (str_array);
}

/* Frees a nul-terminated string */
void libhal_free_string (char *str)
{
	g_free (str);
}

