/*
 * gpe-packages
 *
 * Copyright (C) 2003  Florian Boor <florian.boor@kernelconcepts.de>
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
#include <ipkglib.h>

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
send_message (pkcontent_t ctype, int prio, const char *str1, const char *str2, const char *str3)
{
	pkmessage_t msg;
	msg.type = PK_FRONT;
	msg.ctype = ctype;
	msg.content.tf.priority = prio;
	snprintf(msg.content.tf.str1,LEN_STR1, str1);
	snprintf(msg.content.tf.str2,LEN_STR2, str2);
	snprintf(msg.content.tf.str3,LEN_STR3, str3);
	if (write (sock, (void *) &msg, sizeof (pkmessage_t)) < 0)
	{
		perror ("err sending data to backend");
	}
}


static void
do_response (char *res)
{
	response = res;
	await_response = FALSE;
}


/* ipkg interface */
char * /*ipkg_response_callback*/
ipkg_question (char *question)
{
	send_message (PK_QUESTION, 1, question, NULL, NULL);
	await_response = TRUE;
	while (await_response) wait_message();
printf("Response: %s\n",response);
	return (response);
}


int /*ipkg_message_callback*/
ipkg_msg (ipkg_conf_t * conf, message_level_t level, char *msg)
{
	if (level <= IPKG_NOTICE)
	{
		if (level <= IPKG_ERROR)
			send_message (PK_ERROR, level, msg, NULL, NULL);
		else
			send_message (PK_INFO, level, msg, NULL, NULL);
	}
	return 0;
}


int /*ipkg_list_callback*/
list_entry (char *name, char *desc, char *version)
{
	send_message (PK_LIST, 1, name, desc, version);
	return 0;
}


static int
wait_message ()
{
	static pkmessage_t msg;
	struct pollfd pfd[1];

	pfd[0].fd = sock;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
	while (poll (pfd, 1, -1) > 0)
	{
		if ((pfd[0].revents & POLLERR) || (pfd[0].revents & POLLHUP))
		{
#ifdef DEBUG
			perror ("Err: connection lost: ");
#endif
			return FALSE;
		}
		if (read (sock, (void *) &msg, sizeof (pkmessage_t)) < 0)
		{
#ifdef DEBUG
			perror ("err receiving data packet");
#endif
			close (sock);
			exit (1);
		}
		else if (msg.type == PK_BACK)
		{
			switch (msg.ctype)
			{
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
	return TRUE;
}


static void
do_command (pkcommand_t command, char *params, char *list)
{
	/* add args here */
	
	switch (command)
	{
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
//		ipkg_packages_status(&args,list,status_cb);
	break;
	case CMD_SEARCH:
		ipkg_file_search(&args,list,list_entry);
	break;
	case CMD_LIST:
		ipkg_packages_list(&args,list,list_entry);
	break;
	case CMD_FILES:
		ipkg_package_files(&args,list,list_entry);
	break;
	}
	
	send_message(PK_FINISHED,0,"","","");
}


/* app mainloop */

int
suidloop (int csock)
{
	/* init ipkg lib */
	sock = csock;
	
	ipkg_init (/*(ipkg_message_callback)*/ ipkg_msg,
		   /*(ipkg_response_callback)*/ ipkg_question, &args);

	while (wait_message ()) ;
printf("leaving...\n");
	ipkg_deinit(&args);
	
	close (sock);

	return 0;
}
