/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define _XOPEN_SOURCE

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sqlite.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>

#include <gpe/errorbox.h>

#include "mime-programs-sql.h"

static sqlite *sqliteh;

static const char *fname = "/.gpe/mime-programs";

GSList *mime_programs;

static struct mime_program *
new_mime_program_internal (int id, const char *name, const char *mime, const char *command)
{
  struct mime_program *e = g_malloc (sizeof (struct mime_program));

  e->name = name;
  e->mime = mime;
  e->command = command;

  mime_programs = g_slist_append (mime_programs, e);

  return e;
}

struct mime_program *
new_mime_program (const char *name, const char *mime, const char *command)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "insert into mime_programs values (NULL, '%q', '%q', '%q')",
			  NULL, NULL, &err, name, mime, command))
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return new_mime_program_internal (sqlite_last_insert_rowid (sqliteh), name, mime, command);
}

void
del_mime_program (struct mime_program *e)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "delete from mime_programs where uid=%d",
			  NULL, NULL, &err,
			  e->id))
    {
      gpe_error_box (err);
      free (err);
    }

  mime_programs = g_slist_remove (mime_programs, e);
}

static int
mime_program_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 4 && argv[0] && argv[1])
    new_mime_program_internal (atoi (argv[0]), g_strdup (argv[1]), g_strdup (argv[2]), g_strdup (argv[3]));
  return 0;
}

int
programs_sql_start (void)
{
  static const char *schema1_str = 
    "create table mime_programs (uid INTEGER PRIMARY KEY, name TEXT, mime TEXT, command TEXT)";

  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL) 
    home = "";
  len = strlen (home) + strlen (fname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, fname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  sqlite_exec (sqliteh, schema1_str, NULL, NULL, &err);

  if (sqlite_exec (sqliteh, "select uid,name,mime,command from mime_programs",
		   mime_program_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      return -1;
    }

  return 0;
}

void
programs_sql_close (void)
{
  sqlite_close (sqliteh);
}
