/* GPE SCAP
 * Copyright (C) 2006  Florian Boor <florian@linuxtogo.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Configuration file access.
 */

#include <string.h>
#include <locale.h>
#include <libintl.h>

#include <glib.h>
#include <glib/gstdio.h>
#include "cfgfile.h"

#define _(x) gettext(x)
#define FILENAME_CONFIG ".gpescaprc"

void
load_config (t_gpe_scap_cfg * cfg)
{
  GKeyFile *configfile;
  gchar *filename;
  GError *err = NULL;

  filename = g_strdup_printf ("%s/%s", g_get_home_dir (), FILENAME_CONFIG);
  configfile = g_key_file_new ();

  if (!g_key_file_load_from_file (configfile, filename,
				  G_KEY_FILE_KEEP_COMMENTS, &err))
    {
      g_error_free (err);
      err = NULL;
      return;
    }

  /* basic configuration */
  cfg->warning_disabled = g_key_file_get_boolean (configfile, "Configuration",
						  "warning_disabled", &err);
  if (err)
    {
      g_error_free (err);
      err = NULL;
    }

  g_free (filename);
  g_key_file_free (configfile);
}

gchar *
save_config (t_gpe_scap_cfg *cfg)
{
  GKeyFile *configfile;
  gchar *filename;
  gchar *filecontent;
  FILE *fp;

  filename = g_strdup_printf ("%s/%s", g_get_home_dir (), FILENAME_CONFIG);

  configfile = g_key_file_new ();

  g_key_file_set_boolean (configfile, "Configuration", "warning_disabled",
			  cfg->warning_disabled);

  fp = fopen (filename, "w");
  if (!fp)
    {
      return (_("Could not open config file for writing."));
    }

  filecontent = g_key_file_to_data (configfile, NULL, NULL);

  fprintf (fp, "%s\n", filecontent);
  fclose (fp);

  g_free (filename);
  g_free (filecontent);
  g_key_file_free (configfile);
  return NULL;
}
