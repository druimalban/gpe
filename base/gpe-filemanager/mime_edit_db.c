/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include "mime-programs-sql.h"

#define _(x) dgettext(PACKAGE, x)

int
main (int argc, char *argv[])
{
  struct mime_program *new_program = NULL;

  if (argc == 5)
  {
    new_program = new_mime_program (const char *name, const char *mime, const char *command);
  }
  else
  {
    printf ("Usage:\n\tmime_edit_db add name mime command\n\tmime_edit_db del command\n");
  }
}
