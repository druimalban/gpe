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

gboolean
gpe_application_init (int *argc, char **argv[])
{
  char *fn;
  struct stat buf;
  gchar *user_gtkrc_file;
  const gchar *default_gtkrc_file = PREFIX "/share/gpe/gtkrc";
  gchar *home_dir = g_get_home_dir ();
	  
  gtk_rc_add_default_file (default_gtkrc_file);
  user_gtkrc_file = g_strdup_printf ("%s/.gpe/gtkrc", home_dir);
  gtk_rc_add_default_file (user_gtkrc_file);
  g_free (user_gtkrc_file);

  gtk_init (argc, argv);
  gtk_set_locale ();

  if (home_dir[0] && strcmp (home_dir, "/"))
    {

      /* Maybe this belongs somewhere else */
      fn = g_strdup_printf ("%s/.gpe", home_dir);
      if (stat (fn, &buf) != 0)
	{
	  if (mkdir (fn, 0700) != 0)
	    {
	      gpe_perror_box ("Cannot create ~/.gpe");
	      g_free (fn);
	      return FALSE;
	    }
	} 
      else 
	{
	  if (!S_ISDIR (buf.st_mode))
	    {
	      gpe_error_box ("ERROR: ~/.gpe is not a directory!");
	      g_free (fn);
	      return FALSE;
	    }
	}
  
      g_free (fn);
    }

  gpe_what_init ();

  return TRUE;
}
