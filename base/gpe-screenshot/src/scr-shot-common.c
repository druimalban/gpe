/* GPE Screenshot
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
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
 */

#include "scr-shot.h"
#include "scr-i18n.h"

#define EXTERNAL_UPLOAD_COMMAND PKGDATADIR "/scap-png-upload %s"

GQuark
scr_shot_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("scr-shot-error-quark");

  return q;
}

gboolean
scr_shot_upload_from_file (const gchar *path, const gchar *url, gchar **response, GError **error)
{
  gchar *cmd;
  gchar *std_output = NULL;
  gchar *std_error = NULL;
  gchar *start, *end;
  gint retval = 0;
  GError *err = NULL;

  /* upload screenshot using external command. */
  cmd = g_strdup_printf (EXTERNAL_UPLOAD_COMMAND, path);
  g_spawn_command_line_sync (cmd, &std_output, &std_error, &retval, &err);
  g_free (cmd);

  if (err)
    {
      if (err->message)
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_UPLOAD,
                                  "%s", err->message);
	  g_error_free (err);
	}
      else
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_UPLOAD,
                                  _("Upload failed."));
	}

      return FALSE;
    }

  if (retval % 255)
    {
      g_warning ("External command returned exit code: %d", retval % 255);

      if (std_error)
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_UPLOAD,
                                  "%s", std_error);
	}
      else
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_UPLOAD,
                                  _("External upload command returned exit code: %d"), retval % 255);
	}

      return FALSE;
    }

  if (std_output && response)
    {
      start = strstr (std_output, "Your image");
      end = strstr (std_output, ".png") + 4;

      *response = g_malloc (end - start + 2);
      strncpy (*response, start, end - start + 1);
      (*response)[end - start + 1] = 0;
    }

  return TRUE;
}
