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
#include "what.h"

static const char *dname = "/.gpe";

gboolean
gpe_application_init (int *argc, char **argv[])
{
  const char *home;
  char *buf;
  size_t len;
  const gchar *user_gtkrc_file;
  const gchar *default_gtkrc_file = PREFIX "/share/gpe/gtkrc";
	  
  gtk_rc_add_default_file (default_gtkrc_file);
  user_gtkrc_file = g_strdup_printf ("%s/.gpe/gtkrc", g_get_home_dir());
  gtk_rc_add_default_file (user_gtkrc_file);

  gtk_set_locale ();

  gtk_init(argc, argv);

  /* FIXME: use g_get_home_dir(), move directory creation away [CM]: */
  home = getenv ("HOME");
  if (home == NULL) 
    home = "";

  len = strlen (home) + strlen (dname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  if (access (buf, F_OK))
    {
      gpe_perror_box (buf);
      g_free (buf);
      return FALSE;
    }

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

  gpe_what_init ();

  return TRUE;
}
