/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "errorbox.h"

static const char *dname = "/.gpe";

gboolean
gpe_application_init (int *argc, char **argv[])
{
  const char *home;
  char *buf;
  size_t len;

  gtk_set_locale ();
  gtk_init(argc, argv);

  home = getenv ("HOME");
  if (home == NULL) 
    home = "";

  len = strlen (home) + strlen (dname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, dname);
  if (access (buf, F_OK))
    {
      if (mkdir (buf, 0700))
	{
	  gpe_perror_box (buf);
	  g_free (buf);
	  return FALSE;
	}
    }

  g_free (buf);

  return TRUE;
}
