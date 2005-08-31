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
	int port; // the port for the nsqld;
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
