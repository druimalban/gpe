/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  Copyright (C) 2007 Graham Cobb <g+gpe@cobb.uk.net>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#ifndef GPESYNCD_H
#define GPESYNCD_H

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sqlite.h>
#include <mimedir/mimedir.h>
#include <mimedir/mimedir-vcard.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gpe/vcard.h>
#include <gpe/vevent.h>
#include <gpe/vtodo.h>
#include <gpe/contacts-db.h>
#include <gpe/event-db.h>
#include <gpe/todo-db.h>
#include <gpe/pim-categories.h>

/*
 * Protocol version number
 *
 * Major number: increment this if backward compatibility is broken.
 * This should never happen!  The other end should check this for
 * equality and disconnect if the expected value is not returned. 
 *
 * Minor number: increment this if a new command/feature is added in 
 * a backwards compatible way.  The other end may expect a minimum 
 * version but should not fail if the version is higher than expected.
 *
 * Edit number: increment this if it is useful for a human trying to
 * debug a problem at the other end to know the version of gpesyncd
 * (for example a major bug has been fixed).  This is intended for
 * information only and the other end should normally ignore it.
 * This should be incremented for ever (never reset back to 0).
 * NOTE: this means the value will rapidly exceed a single byte -- use an int!
 *
 * Version history
 * ---------------
 *
 * 1.0.0    Original version.  Implied version if VERSION command not implemented.
 *
 * 1.1.1    VERSION command added.
 *
 * 1.2.2    PATH VEVENT command added.
 *
 * 1.3.3    Support for additional RFC2426 attributes implemented in libmimedir and libgpevtype.
 *
 */
#define PROTOCOL_MAJOR 1
#define PROTOCOL_MINOR 3
#define PROTOCOL_EDIT 3

#define BUFFER_LEN 25 

typedef enum
{
  GPE_DB_TYPE_UNKNOWN = 1,
  GPE_DB_TYPE_VCARD,
  GPE_DB_TYPE_VEVENT,
  GPE_DB_TYPE_VTODO
} gpe_db_type;

//typedef struct gpesyncd_context gpesyncd_context;

typedef struct
{
  FILE *ifp, *ofp;
  int socket;
  int remote;

  EventDB *event_db;
  GSList *event_calendars;
  EventCalendar *import_calendar;

  GString *result;

  int quit;
} gpesyncd_context;

#include "import.h"
#include "export.h"

extern void free_object_list(GSList *);

gpesyncd_context *gpesyncd_context_new (char *err);

void gpesyncd_context_free (gpesyncd_context * ctx);

void gpesyncd_printf (gpesyncd_context * ctx, char *format, ...);

gboolean do_command (gpesyncd_context * ctx, gchar * command);

void command_loop (gpesyncd_context * ctx);

int remote_loop (gpesyncd_context * ctx);

extern char *
do_import_vevent (EventDB *event_db, EventCalendar *ec, MIMEDirVEvent *event, Event **new_ev);

extern MIMEDirVEvent *
export_event_as_vevent (Event *ev);

GSList *todo_db_item_to_tags (struct todo_item *t);
struct todo_item *todo_db_find_item_by_id(guint uid);

#endif /* GPESYNCD_H */
