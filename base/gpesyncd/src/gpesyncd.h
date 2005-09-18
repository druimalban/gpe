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

#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sqlite.h>
#include <mimedir/mimedir.h>
#include <mimedir/mimedir-vcard.h>

#include <gpe/vcard.h>
#include <gpe/vevent.h>
#include <gpe/vtodo.h>

#define _GNU_SOURCE

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

  sqlite *contact_db;
  sqlite *event_db;
  sqlite *todo_db;

  GString *result;

  int quit;
} gpesyncd_context;

#include "import.h"
#include "export.h"

gpesyncd_context *gpesyncd_context_new (char *err);

void gpesyncd_context_free (gpesyncd_context * ctx);

void gpesyncd_printf (gpesyncd_context * ctx, char *format, ...);

gboolean do_command (gpesyncd_context * ctx, gchar * command);

void command_loop (gpesyncd_context * ctx);

int remote_loop (gpesyncd_context * ctx);

#endif /* GPESYNCD_H */
