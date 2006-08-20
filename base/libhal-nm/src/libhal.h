#ifndef LIBHAL_H
#define LIBHAL_H

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

#include <dbus/dbus.h>

/* Fake libhal API implementation 
 * This file contains portions of code from original libhal header file, written
 * by David Zeuthen, <david@fubar.dk>.
 * */


typedef struct LibHalContext_s LibHalContext;

typedef void (*LibHalDeviceAdded) (LibHalContext *ctx,
                                   const char *udi);

typedef void (*LibHalDeviceRemoved) (LibHalContext *ctx,
                                     const char *udi);

typedef void (*LibHalDeviceNewCapability) (LibHalContext *ctx,
                                           const char *udi,
                                           const char *capability);

/* Create a new context for a connection with hald */
LibHalContext *
libhal_ctx_new (void);

/* Initialize the connection to hald */
dbus_bool_t    
libhal_ctx_init (LibHalContext *ctx, DBusError *error);

/* Set DBus connection to use to talk to hald. */
dbus_bool_t    
libhal_ctx_set_dbus_connection (LibHalContext *ctx, DBusConnection *conn);

/* Get user data for the context */
void *
libhal_ctx_get_user_data (LibHalContext *ctx);

/* Set user data for the context */
dbus_bool_t
libhal_ctx_set_user_data (LibHalContext *ctx, void *user_data);

/* Set the callback for when a device is added */
dbus_bool_t
libhal_ctx_set_device_added (LibHalContext *ctx, LibHalDeviceAdded callback);

/* Set the callback for when a device is removed */
dbus_bool_t
libhal_ctx_set_device_removed (LibHalContext *ctx, LibHalDeviceRemoved callback);

/* Set the callback for when a device gains a new capability */
dbus_bool_t
libhal_ctx_set_device_new_capability (LibHalContext *ctx, 
                                      LibHalDeviceNewCapability callback);

/* Watch all devices, ie. the device_property_changed callback is
 * invoked when the properties on any device changes.
 */
dbus_bool_t 
libhal_device_property_watch_all (LibHalContext *ctx, DBusError *error);

/* Find devices with a given capability. */
char **
libhal_find_device_by_capability (LibHalContext *ctx, const char *capability,
                                  int *num_devices, DBusError *error);

/* Determine if a property on a device exists. */
dbus_bool_t 
libhal_device_property_exists (LibHalContext *ctx, const char *udi, 
                               const char *key, DBusError *error);

/* Get the value of a property of type string. */
char *
libhal_device_get_property_string (LibHalContext *ctx, const char *udi,
                                   const char *key, DBusError *error);

/* Free a LibHalContext resource */
dbus_bool_t    
libhal_ctx_free (LibHalContext *ctx);

/* Shut down a connection to hald */
dbus_bool_t    
libhal_ctx_shutdown (LibHalContext *ctx, DBusError *error);

#endif

/* Frees a NULL-terminated array of strings. If passed NULL, does nothing. */
void 
libhal_free_string_array (char **str_array);

/* Frees a nul-terminated string */
void 
libhal_free_string (char *str);

