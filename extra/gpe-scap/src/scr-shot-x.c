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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "scr-shot.h"
#include "scr-i18n.h"
#include "screenshot-utils.h"

struct _Priv
{
  GdkPixbuf *full;
  GdkPixbuf *preview;
  gdouble scaling_factor;
};
typedef struct _Priv Priv;

ScrShot *
scr_shot_new ()
{
  ScrShot *shot = g_new0 (ScrShot, 1);
  Priv *priv = g_new0 (Priv, 1);
  GdkPixbuf *buf = NULL;

  shot->private_data = (gpointer) priv;

  /* take screenshot */
  buf = screenshot_get_pixbuf (GDK_ROOT_WINDOW ());
  if (!buf)
    {
      g_free (priv);
      g_free (shot);

      return NULL;
    }

  g_object_ref (buf);
  priv->full = buf;

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

  width = gdk_pixbuf_get_width (priv->full);
  height = gdk_pixbuf_get_height (priv->full);

  buf = gdk_pixbuf_scale_simple (priv->full,
                                 (gint) (width * scaling_factor),
				 (gint) (height * scaling_factor),
				 GDK_INTERP_BILINEAR);
  if (!buf)
    {
      g_warning ("Could not rescale pixbuf");
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
  gchar *tmpnam;
  gboolean ret;
  gint f;
  GError *err = NULL;
  gchar *tmp;

  if (!shot || !url)
    return FALSE;

  f = g_file_open_tmp ("gpe-screenshot-XXXXXX", &tmpnam, &err);
  if (f < 0)
    {
      if (err && err->message)
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_TMPFILE,
                                  _("Could not create temporary file: %s"), err->message);
	  g_error_free (err);
	}
      else
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_TMPFILE,
                                  _("Could not create temporary file."));
	}

      return FALSE;
    }

  /* what we want is a temporary filename only */
  close (f);

  priv = SCR_SHOT_PRIV (shot);
  /* save to temporary file */
  if (! gdk_pixbuf_save (priv->full, tmpnam,
                             "png", &err,
                             "tEXt::CREATOR", "gpe-screenshot",
                             NULL))
    {
      if (err && err->message)
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_TMPFILE,
                                  _("Could not save screenshot to temporary file: %s"), err->message);
	  g_error_free (err);
	}
      else
        {
          if (error)
            *error = g_error_new (SCR_SHOT_ERROR,
                                  SCR_SHOT_ERROR_TMPFILE,
                                  _("Could not save screenshot to temporary file."));
	}

      g_unlink (tmpnam);

      return FALSE;
    }

  ret = scr_shot_upload_from_file (tmpnam, url, response, error);

  g_unlink (tmpnam);
  g_free (tmpnam);
  tmpnam = NULL;

  return ret;
}

gboolean
scr_shot_save (const ScrShot *shot, const gchar *path, GError **error)
{
  Priv *priv;

  if (!shot || !path)
    return FALSE;

  priv = SCR_SHOT_PRIV (shot);

  return gdk_pixbuf_save (priv->full, path,
                         "png", error,
                         "tEXt::CREATOR", "gpe-screenshot",
                         NULL);
}

void
scr_shot_free (ScrShot *shot)
{
  if (!shot)
    return;

  if (shot->private_data)
    {
      Priv *priv = SCR_SHOT_PRIV (shot);

      if (priv->full)
        g_object_unref (priv->full);
      if (priv->preview)
        g_object_unref (priv->preview);

      g_free (priv);
      shot->private_data = NULL;
    }
  
  g_free (shot);
}
