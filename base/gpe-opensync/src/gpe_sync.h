#ifndef GPE_SYNC_H
#define GPE_SYNC_H

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <opensync/opensync.h>

// this is needed for gpe_xml.c:
#include <strings.h>
#include <stdlib.h>

#include <libxml/parser.h>

typedef struct {
	OSyncMember *member;
	char *change_id;
	char *configfile;
	char *addressbook_path;
	// tasks, calendar are to come

	// configuration
	char ip[4]; // the ip of the handheld;
	int port; // the port for the nsqld;
	char *user; // The user on the handheld
	
	// information about the current status
	int socket; // the network connection to the nsqld.
	int debuglevel;
} gpe_environment;


#include "gpe_adresses.h"
#include "gpe_xml.h"

#endif
