/*
 * Copyright (C) 2001, 2002, 2004 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "errorbox.h"
#include "spacing.h"

gint saved_argc;
gchar **saved_argv;

gboolean
gpe_application_init (int *argc, char **argv[])
{
  char *fn;
  struct stat buf;
  gchar *user_gtkrc_file;
  const gchar *default_gtkrc_file = PREFIX "/share/gpe/gtkrc";
  const gchar *home_dir = g_get_home_dir ();
  gint i;

  if (argc && argv)
    {
      saved_argc = *argc;
      saved_argv = g_malloc (*argc * sizeof (gchar *));
      for (i = 0; i < saved_argc; i++)
        saved_argv[i] = g_strdup ((*argv)[i]);
    }

  gtk_rc_add_default_file (default_gtkrc_file);
  user_gtkrc_file = g_strdup_printf ("%s/.gpe/gtkrc", home_dir);
  gtk_rc_add_default_file (user_gtkrc_file);
  g_free (user_gtkrc_file);

  gtk_init (argc, argv);
  gtk_set_locale ();

  init_spacing();
	
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

  return TRUE;
}

void
gpe_saved_args (gint *argc, gchar **argv[])
{
  *argc = saved_argc;
  *argv = saved_argv;
}
