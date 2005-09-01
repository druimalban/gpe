/*
 * Copyright (C) 2005 Martin Felis <martin@silef.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Parts of this file are derieved from Phil Blundells libnsqlc.
 * See http://handhelds.org/cgi-bin/cvsweb.cgi/gpe/base/libnsqlc/
 * for more information.
 */

#ifndef _GPE__CLIENT_H_
#define _GPE__CLIENT_H_
#include <stdarg.h>     /* Needed for the definition of va_list */

#define BUFFER_LENGTH 8192

typedef struct gpesync_client gpesync_client;

/*! \brief This opens a connection to the gpesyncd
 *
 * \param addr		The addr of the client. Must be in username@host format
 * \param errmsg	If an error occurs, the message will be written there.
 *
 */
gpesync_client *gpesync_client_open(const char *addr, char **errmsg);

/*! \brief Closes an exisiting connection and frees the memory.
 *
 * \param client	The client that was previously connected with
 * 			gpesync_client_open
 */
void gpesync_client_close(gpesync_client *client);

/*! \brief A definition of the callback function format.
 */
typedef int (*gpesync_client_callback)(void*,int,char**);

/*! \brief Executes a command on the gpesyncd
 *
 * \param client	The client, that is connectd
 * \param command	The command that should be sent
 * \param gpesync_client_callback
 * 			The pointer to a callback function that should be
 * 			called for each result line of the query.
 * \param void		A pointer that will be passed as first argument of
 * 			the callback function.
 * \param errmsg	If an error occurs, the message will be written here.
 */
int gpesync_client_exec(
  gpesync_client*,                      /* An open database */
  const char *command,              /* SQL to be executed */
  gpesync_client_callback,              /* Callback function */
  void *,                       /* 1st argument to callback function */
  char **errmsg                 /* Error msg written here */
);

#define GPESYNC_CLIENT_OK	0
#define GPESYNC_CLIENT_ERROR	1
#define GPESYNC_CLIENT_ABORT	2

/* \brief This is just a wrapper for gpesync_client_exec
 */
int gpesync_client_exec_printf(
  gpesync_client *client,                   /* An open database */
  const char *query_format,        /* printf-style format string for the SQL */
  gpesync_client_callback xCallback,    /* Callback function */
  void *pArg,                   /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  ...                           /* Arguments to the format string. */
);

/* \brief A callback function which just prints out the result
 */
int client_callback_print (void *arg, int argc, char **argv);

/* \brief A callback function that returns the result as a list
 */
int client_callback_list (void *arg, int argc, char **argv);

/* \brief A callback function that returns the result in a 
 * 	  gchar * string.
 */
int client_callback_string (void *arg, int argc, char **argv);

/* \brief A callback function that returns the result as a GString*
 */
int client_callback_gstring (void *arg, int argc, char **argv);

#endif /* _GPE__CLIENT_H_ */
