/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <glib.h>
#include <string.h>

#include "mime-programs-sql.h"

#define _(x) dgettext(PACKAGE, x)

void
usage ()
{
  printf ("Usage:\n\tmime_edit_db -add \"name\" \"mime\" \"command\"\n\tmime_edit_db -del \"command\"\n");
  exit (1);
}

int
main (int argc, char *argv[])
{
  struct mime_program *new_program = g_malloc (sizeof (struct mime_program));

  if (argc == 5)
  {
    if (programs_sql_start ())
      exit (1);

    if (strcmp (argv[1], "-add") == 0)
    {
      new_program = new_mime_program (argv[2], argv[3], argv[4]);

      printf ("\nAdded program to the mime database.\nName: %s\nMime association: %s\nCommand: %s\n\n", new_program->name, new_program->mime, new_program->command);
    }
  }
  else
  {
    usage ();
  }
}
