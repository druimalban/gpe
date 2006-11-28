/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#ifndef GPESYNCD_H
#define GPESYNCD_H

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

#define _GNU_SOURCE

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
