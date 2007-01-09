/* GPE SCAP
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
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
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "scr-shot.h"
#include "scr-i18n.h"

#ifndef DESKTOP_BUILD
#define EXTERNAL_SCREENSHOT_COMMAND "fbgrab -s 3 -d /dev/fb0 %s"
#else
#define EXTERNAL_SCREENSHOT_COMMAND "import -window root %s"
#endif /* DESKTOP_BUILD */

struct _Priv
{
  gchar *tmpnam;
  GdkPixbuf *preview;
  gdouble scaling_factor;
};
typedef struct _Priv Priv;

ScrShot *
scr_shot_new ()
{
  ScrShot *shot = g_new0 (ScrShot, 1);
  Priv *priv = g_new0 (Priv, 1);
  gint f;
  GError *error = NULL;
  gint retval = 0;
  gchar *cmd;
  gchar *tmp;

  shot->private_data = (gpointer) priv;

  f = g_file_open_tmp ("gpe-screenshot-XXXXXX", &priv->tmpnam, &error);
  if (f < 0)
    {
      if (error)
        {
          g_warning ("Could not create tmp file: %s.png", error->message);
	  g_error_free (error);
	}

      g_free (priv);
      g_free (shot);

      return NULL;
    }

  /* what we want is a temporary filename only */
  close (f);

  /* filename needs to end in .png for import to creat a PNG */
  g_unlink (priv->tmpnam);
  tmp = priv->tmpnam;
  priv->tmpnam = g_strdup_printf ("%s.png", tmp);
  g_free (tmp);

  /* take screenshot using external command. */
  cmd = g_strdup_printf (EXTERNAL_SCREENSHOT_COMMAND, priv->tmpnam);
  g_spawn_command_line_sync (cmd, NULL, NULL, &retval, &error);
  g_free (cmd);

  if (error)
    {
      if (error)
        {
          g_warning ("Could not spawn external command: %s", error->message);
	  g_error_free (error);
	}
      g_warning ("External command returned exit code: %d", retval);

      g_free (priv);
      g_free (shot);

      return NULL;
    }

  return shot;
}

GdkPixbuf *
scr_shot_get_preview (ScrShot *shot, gdouble scaling_factor)
{
  Priv *priv;
  GdkPixbuf *buf;
  gint width, height;
  GError *error = NULL;

  if (!shot)
    return NULL;

  priv = SCR_SHOT_PRIV (shot);
  if (priv->preview && priv->scaling_factor == scaling_factor)
    /* return cached pixbuf */
    return priv->preview;

  if (!priv->tmpnam)
    {
      g_warning ("Screenshot already saved.");
      return NULL;
    }

  if (!gdk_pixbuf_get_file_info (priv->tmpnam, &width, &height))
    {
      g_warning ("Could not access screenshot: %s", priv->tmpnam);
      return NULL;
    }

  buf = gdk_pixbuf_new_from_file_at_scale (priv->tmpnam, (gint) (width * scaling_factor), (gint) (height * scaling_factor), TRUE, &error);
  if (!buf)
    {
      g_warning ("Could not open screenshot: %s", priv->tmpnam);
      return NULL;
    }

  if (priv->preview)
    g_object_unref (priv->preview);

  g_object_ref (buf);
  priv->preview = buf;
  priv->scaling_factor = scaling_factor;

  return buf;
}

gboolean
scr_shot_upload (const ScrShot *shot, const gchar *url, gchar **response, GError **error)
{
  Priv *priv;
  gboolean ret;

  if (!shot || !url)
    return FALSE;

  priv = SCR_SHOT_PRIV (shot);
  if (!priv->tmpnam)
    {
      g_warning ("Temporary screenshot file no longer available.");
      return FALSE;
    }

  ret = scr_shot_upload_from_file (priv->tmpnam, url, response, error);

  g_free (priv->tmpnam);
  priv->tmpnam = NULL;

  return ret;
}

gboolean
scr_shot_save (const ScrShot *shot, const gchar *path, GError **error)
{
  Priv *priv;
  char buf[4096];
  int bytes;
  int infd, outfd;

  if (!shot || !path)
    return FALSE;

  priv = SCR_SHOT_PRIV (shot);
  if (!priv->tmpnam)
    {
      g_warning ("Screenshot already saved.");
      return FALSE;
    }

  if (!g_rename (priv->tmpnam, path))
    {
      /* cleanup on success */
      g_free (priv->tmpnam);
      priv->tmpnam = NULL;

      return TRUE;
    }

  infd = open (priv->tmpnam, O_RDONLY);
  if (infd < 0)
    {
      if (error)
	*error = g_error_new (SCR_SHOT_ERROR, SCR_SHOT_ERROR_INTERNAL, _("Could not read temporary screenshot file: %s"), priv->tmpnam);

      return FALSE;
    }

  outfd = open (path, O_CREAT|O_TRUNC|O_WRONLY, 0644);
  if (outfd < 0)
    {
      if (error)
        *error = g_error_new (SCR_SHOT_ERROR, SCR_SHOT_ERROR_CREATE, _("Could not save screenshot to file: %s"), path);

      close (infd);

      return FALSE;
    }

  while ((bytes = read (infd, buf, sizeof (buf))) > 0)
    {
      if (write (outfd, buf, bytes) != bytes)
        {
	  close (infd);
	  close (outfd);
	  unlink (path);

	  if (error)
	    *error = g_error_new (SCR_SHOT_ERROR, SCR_SHOT_ERROR_SPACE, _("Not enough space left to save screenshot to file: %s"), path);

	  return FALSE;
	}
    }

  close (infd);
  close (outfd);
  unlink (priv->tmpnam);
  g_free (priv->tmpnam);
  priv->tmpnam = NULL;

  return TRUE;
}

void
scr_shot_free (ScrShot *shot)
{
  if (!shot)
    return;

  if (shot->private_data)
    {
      Priv *priv = SCR_SHOT_PRIV (shot);

      if (priv->tmpnam)
        {
          g_unlink (priv->tmpnam);
          g_free (priv->tmpnam);
	}

      if (priv->preview)
        g_object_unref (priv->preview);
      g_free (priv);
      shot->private_data = NULL;
    }
  
  g_free (shot);
}
