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
  char *fn;
  struct stat buf;
  gchar *user_gtkrc_file;
  const gchar *default_gtkrc_file = PREFIX "/share/gpe/gtkrc";
	  
  gtk_rc_add_default_file (default_gtkrc_file);
  user_gtkrc_file = g_strdup_printf ("%s/.gpe/gtkrc", g_get_home_dir());
  gtk_rc_add_default_file (user_gtkrc_file);
  g_free (user_gtkrc_file);

  gtk_set_locale ();

  gtk_init(argc, argv);

  fn = g_strdup_printf ("%s%s", g_get_home_dir(),dname);
  if (stat (fn, &buf) != 0)
    {
      if (mkdir (fn, 0700) != 0)
	{
	  gpe_perror_box ("Cannot create ~/.gpe");
	  return FALSE;
	}
    } else {
      if (!S_ISDIR(buf.st_mode))
	{
	  gpe_perror_box ("ERROR: ~/.gpe is not a directory!\n");
	  return FALSE;
	}
    }
  
  g_free (fn);

  gpe_what_init ();

  return TRUE;
}
