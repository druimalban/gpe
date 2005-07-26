#ifndef GPE_SYNC_H
#define GPE_SYNC_H

#include <opensync/opensync.h>

typedef struct {
	OSyncMember *member;
	char *change_id;
	char *configfile;
	char *addressbook_path;
	// tasks, calendar are to come
	
	char ip[4]; // the ip of the handheld;
	int port; // the port for the nsqld;
	char *syncuser; // The user on the handheld
	
	int socket; // the network connection to the nsqld.
	int debuglevel;
} gpe_environment;

#include "gpe_adresses.h"

#endif
