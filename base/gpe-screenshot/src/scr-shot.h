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

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

struct _ScrShot
{
  gpointer private_data;
};
typedef struct _ScrShot ScrShot;

#define SCR_SHOT_PRIV(x) (Priv *) (x)->private_data

ScrShot *
scr_shot_new ();

GdkPixbuf *
scr_shot_get_preview (ScrShot *shot, gdouble scaling_factor);

gboolean
scr_shot_upload (const ScrShot *shot, const gchar *url, gchar **response, GError **error);

gboolean
scr_shot_save (const ScrShot *shot, const gchar *path, GError **error);

void
scr_shot_free (ScrShot *shot);

#define SCR_SHOT_ERROR (scr_shot_error_quark())

enum
{
  SCR_SHOT_ERROR_SPACE,
  SCR_SHOT_ERROR_CREATE,
  SCR_SHOT_ERROR_UPLOAD,
  SCR_SHOT_ERROR_TMPFILE,
  SCR_SHOT_ERROR_INTERNAL
};

/* private */
gboolean
scr_shot_upload_from_file (const gchar *path, const gchar *url, gchar **response, GError **error);
