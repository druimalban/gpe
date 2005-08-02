#ifndef GPE_SYNC_H
#define GPE_SYNC_H

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <opensync/opensync.h>

#include <strings.h>
#include <stdlib.h>

// this is needed for gpe_xml.c:
#include <libxml/parser.h>
#include <nsqlc.h>
#include <gpe/tag-db.h>
#include <gpe/vcard.h>
#include <gpe/tag-db.h>

typedef struct {
	OSyncMember *member;
	OSyncHashTable *hashtable;
	char *change_id;

	nsqlc *contacts;
	nsqlc *calendar;
	nsqlc *todo;
	
	// configuration
	char *device_addr; // the ip of the handheld;
	int port; // the port for the nsqld;
	char *username; // The user on the handheld
	
	int debuglevel;
} gpe_environment;

#include "contacts.h"
#include "gpe_xml.h"
#include "gpe_db.h"

#define GPE_CONNECT_ERROR 1
#define GPE_SQL_EXEC_ERROR 2
#define GPE_HASH_LOAD_ERROR 3

#endif
