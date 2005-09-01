/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#ifndef GPE_SYNC_H
#define GPE_SYNC_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <glib.h>
#include <glib-object.h>
#include <opensync/opensync.h>

// this is needed for gpe_xml.c:
#include <libxml/parser.h>

#include "gpesync_client.h"

typedef struct {
	OSyncMember *member;
	OSyncHashTable *hashtable;

	gpesync_client *client;
	
	// configuration
	char *device_addr; // the ip of the handheld;
	char *username; // The user on the handheld
	
	int debuglevel;
} gpe_environment;

#include "utils.h"
#include "contacts.h"
#include "calendar.h"
#include "todo.h"
#include "gpe_xml.h"

#define GPE_CONNECT_ERROR 1
#define GPE_SQL_EXEC_ERROR 2
#define GPE_HASH_LOAD_ERROR 3

#endif
