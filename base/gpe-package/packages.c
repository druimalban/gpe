/*
 * gpe-package
 *
 * Copyright (C) 2003, 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE package manager module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <gdk/gdk.h>
#include <libipkg.h>

#include "packages.h"
#include "main.h"

static int sock;
static int await_response = FALSE;
char *response = NULL;
static void do_command (pkcommand_t command, char *params, char *list);
static int wait_message ();

static args_t args;



/* message send and receive */

static void
send_message (pkcontent_t ctype, int prio, const char *str1, const char *str2, 
	const char *str3, pkg_state_status_t status)
{
pkmessage_t msg;

	msg.type = PK_FRONT;
	msg.ctype = ctype;
	msg.content.tf.priority = prio;
	snprintf(msg.content.tf.str1,LEN_STR1-1, str1);
	snprintf(msg.content.tf.str2,LEN_STR2-1, str2);
	snprintf(msg.content.tf.str3,LEN_STR3-1, str3);
	msg.content.tf.status = status;
	if (write (sock, (void *) &msg, sizeof (pkmessage_t)) < 0) {
		perror ("ERR sending data to frontend");
	}
}


static void
do_response (char *res)
{
	response = res;
	await_response = FALSE;
}


/* ipkg interface */
char * 
ipkg_question (char *question)
{
	send_message (PK_QUESTION, 1, question, NULL, NULL, 0);
	await_response = TRUE;
	while (await_response)
		wait_message();
	return (response);
}


int
ipkg_msg (ipkg_conf_t * conf, message_level_t level, char *msg)
{
	if (level <= IPKG_NOTICE) {
		if (level <= IPKG_ERROR)
			send_message (PK_ERROR, level, msg, NULL, NULL, 0);
		else
			send_message (PK_INFO, level, msg, NULL, NULL, 0);
	}
	return 0;
}


int
list_entry (char *name, char *desc, char *version, 
	pkg_state_status_t status, void *userdata)
{
	send_message (PK_LIST, 1, name, desc, version, status);
	return 0;
}


int 
package_info(char *name, int istatus, char *desc, void *userdata)
{
	send_message (PK_PKGINFO, 1, name, desc, NULL, 0);
	return 0;
}


int 
package_status(char *name, int istatus, char *desc, void *userdata)
{
	return 0;	
}


static int
wait_message ()
{
static pkmessage_t msg;
struct pollfd pfd[1];
static int retry_count = 0;

	pfd[0].fd = sock;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
	while (poll (pfd, 1, -1) > 0) {
		if ((pfd[0].revents & POLLERR) || (pfd[0].revents & POLLHUP)) {
#ifdef DEBUG
			perror ("Err: connection lost: ");
#endif		
			retry_count++;
			if (retry_count > 6)
				return FALSE;
			usleep(500000);
		} else {
			if (read (sock, (void *) &msg, sizeof (pkmessage_t)) < 0) {
#ifdef DEBUG
				perror ("err receiving data packet");
#endif
				return FALSE;
			} else {
				// fprintf(stderr,"got type %d ctype %d\n", msg.type, msg.ctype);
				if (msg.type == PK_BACK) {
					retry_count = 0;
					switch (msg.ctype) {
						case (PK_COMMAND):
							do_command (msg.content.tb.command,
								msg.content.tb.params,
								msg.content.tb.list);
							break;
						case (PK_REPLY):
							do_response (msg.content.tb.params);
							break;
						default:
							break;
					}
				}
			}
		} /* else */	
	} /* while */
	return TRUE;
}


static void
do_command (pkcommand_t command, char *params, char *list)
{
	/* seems to be necessary for latest ipkg or nothing will happen */
	if (!strlen(list))
		list = NULL;
	
	switch (command) {
		case CMD_INSTALL:
			ipkg_packages_install(&args,list);
			break;
		case CMD_REMOVE:
			ipkg_packages_remove(&args,list,FALSE);
			break;
		case CMD_PURGE:
			ipkg_packages_remove(&args,list,TRUE);
			break;
		case CMD_UPDATE:
			ipkg_lists_update(&args);
			break;
		case CMD_UPGRADE:
			ipkg_packages_upgrade(&args);
			break;
		case CMD_STATUS:
			ipkg_packages_status(&args,list,package_status,NULL);
			break;
		case CMD_INFO:
			ipkg_packages_info(&args,list,package_info,NULL);
			break;
		case CMD_SEARCH:
			ipkg_file_search(&args,list,list_entry, NULL);
			break;
		case CMD_LIST:
			ipkg_packages_list(&args,list,list_entry, NULL);
			break;
		case CMD_FILES:
			ipkg_package_files(&args,list,list_entry, NULL);
			break;
		default:
			break;
	}
	
	send_message(PK_FINISHED,0,"","","",0);
}


/* app mainloop */

int
suidloop (int csock)
{
	/* init ipkg lib */
	sock = csock;
	ipkg_init (ipkg_msg, ipkg_question, &args);

	while (wait_message ()) ;
		
	ipkg_deinit(&args);

	return 0;
}
