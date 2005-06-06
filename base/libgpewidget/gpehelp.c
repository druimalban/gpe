/*
 * Copyright (C) Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Rewrite for integration of new GPE help system:
 * Copyright (C) Philippe De Swert <philippedeswert@scarlet.be>
 *
 * Dedicated to Rammstein - Live in Berlin - Disk 1 
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
#include <gtk/gtk.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gkeyfile.h>


/* helper function definitions*/
char *gpe_check_for_help (const char *appname);	/* can be used as a quick help check when starting an app */
static GKeyFile *load_help_key_file (void);

/** 
 * gpe_show_help
 * @appname: Name of the application to show help about.
 * @topic: String describing help topic.
 *
 *  This function provides a generic interface for displaying
 *  full text online help. It is intended to be independent from
 *  file format and location.
 *  Return value is FALSE if help is found and displayed, TRUE
 *  if an error occurs.
 *	 
 *  Returns: TRUE if an error occured, FALSE otherwise.
 */
gboolean
gpe_show_help (const char *appname, const char *topic)
{
  gchar *helpfile;
  gchar *helpadress;
  gchar *apps = "gpe-helpviewer dillo osb-browser minimo";
  gchar **app, **applist;
  const gchar *command;
  gboolean spawn = FALSE;

  /* lookup help file */
  helpfile = gpe_check_for_help (appname);
  if (helpfile == NULL)
    return TRUE;

  /* check if the file is readable */
  if (access (helpfile, R_OK))
    {
#if defined DEBUG
      printf ("helpfile %s, not readable\n", helpfile);
#endif
      return TRUE;
    }

  /* construct the complete help address */
  if ((topic) && strlen (topic))
    helpadress = g_strdup_printf ("%s#%s", helpfile, topic);
  else
    helpadress = helpfile;

  /* make array of strings to be able to check for the different aplications */
  app = g_strsplit (apps, " ", 0);
  applist = app;

  while (*app != NULL)
    {
      command = g_strdup_printf ("%s %s", *app, helpadress);
#if defined DEBUG
      printf ("command = %s\n", command);
#endif
      spawn = g_spawn_command_line_async (command, NULL);
      if (spawn)
         return FALSE;
      app++;
    }
  if (!spawn)
    {
      g_free (command);
      g_free (helpfile);
      g_strfreev (applist); 
      if (helpadress != helpfile)
        g_free (helpadress); 
      return TRUE;
    }
}

/** 
 * gpe_check_for_help
 * @appname: Name of the application.
 *
 * Checks for the existance of any installed help. Returns NULL if no help, or
 * if the help for that particular application is not installed.
 * If the help is found it returns the filename to that help.
 *
 * Returns: Filename if help is installed.
 */
char *
gpe_check_for_help (const char *appname)
{
  gchar *file;
  GKeyFile *info;

  info = load_help_key_file ();
  if (info == NULL)
    {
      return NULL;
    }
  else
    {
      file = g_key_file_get_value (info, "Help", appname, NULL);
      g_free (info);
      return file;
    }

}

/*
 * Loads the keyfile in memory and returns a pointer to it.
 * This will be NULL if no help is installed or the helpviewer
 * configuration file is not there 
*/
static GKeyFile *
load_help_key_file (void)
{
  GKeyFile *config;
  gboolean loaded;
  const char *filename = "/etc/gpe/gpe-help.conf";

  config = g_key_file_new ();
  loaded =
    g_key_file_load_from_file (config, filename, G_KEY_FILE_NONE, NULL);
  if (loaded)
    return config;
  else
    return NULL;
}
