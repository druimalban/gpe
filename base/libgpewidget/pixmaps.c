/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libintl.h>

#include "pixmaps.h"
#include "errorbox.h"

static GData *pbdata;

static const gchar *theme_dir_tail = "/.gpe/pixmaps";
static const gchar *default_theme_dir = PREFIX "/share/gpe/pixmaps/default";
static const gchar *theme_dir;

#define _(x) dgettext(PACKAGE, x)

static GdkPixbuf *
gpe_load_one_icon (const char *filename, gchar **error)
{
  const gchar *pathname;
  char buf[1024];
  GdkPixbuf *pb;
#if GDK_PIXBUF_MAJOR >= 2
  GError *g_error = NULL;
#endif
  
  if (filename[0] == '/')
    {
      pathname = filename;
    }
  else
    {
      gboolean found = FALSE;
      if (theme_dir)
	{
	  snprintf (buf, sizeof (buf) - 1, "%s/%s.png", theme_dir, filename);
	  buf[sizeof (buf) - 1] = 0;
	  if (access (buf, R_OK) == 0)
	    found = TRUE;
	}
      if (found == FALSE && theme_dir != default_theme_dir)
	{
	  snprintf (buf, sizeof (buf) - 1, "%s/%s.png", default_theme_dir, 
		    filename);
	  buf[sizeof (buf) - 1] = 0;
	}
      pathname = buf;
    }
  
#if GDK_PIXBUF_MAJOR < 2
  pb = gdk_pixbuf_new_from_file (pathname);
#else
  pb = gdk_pixbuf_new_from_file (pathname, &g_error);
#endif

  if (pb == NULL && error)
    {
#if GDK_PIXBUF_MAJOR < 2
      snprintf (buf, sizeof (buf) - 1, _("Unable to load icon \"%s\""),
		filename);
#else
      snprintf (buf, sizeof (buf) - 1, _("Unable to load icon \"%s\": %s"),
		filename, g_error->message);
      g_error_free (g_error);
#endif
      buf[sizeof (buf)-1] = 0;
      *error = g_strdup (buf);
    }

  return pb;
}

gboolean 
gpe_load_icons (struct gpe_icon *p)
{
  gchar *home = getenv ("HOME");
  gchar *buf;
  size_t s;
  gboolean ok = TRUE;

  if (home == NULL)
    home = "/";

  if (theme_dir == NULL)
    {
      s = strlen (home) + strlen (theme_dir_tail) + 1;
      buf = alloca (s);
      strcpy (buf, home);
      strcat (buf, theme_dir_tail);
  
      if (access (buf, F_OK) == 0)
	theme_dir = g_strdup (buf);
      else
	theme_dir = default_theme_dir;
 
      g_datalist_init (&pbdata);
    }
  
  while (p->shortname)
    {
      gchar *error;
      p->pixbuf = gpe_load_one_icon (p->filename ? p->filename : p->shortname,
				     &error);

      if (p->pixbuf == NULL)
	{
	  gpe_error_box (error);
	  g_free (error);
	  ok = FALSE;
	}

      g_datalist_set_data (&pbdata, p->shortname, p);
      p++;
    }

  return ok;
}

GdkPixbuf *
gpe_try_find_icon (const char *name, gchar **error)
{
  struct gpe_icon *p = g_datalist_get_data (&pbdata, name);

  if (p == NULL)
    {
      GdkPixbuf *buf = gpe_load_one_icon (name, error);
      if (buf)
	{
	  p = g_malloc (sizeof (struct gpe_icon));
	  p->shortname = g_strdup (name);
	  p->pixbuf = buf;
	  g_datalist_set_data (&pbdata, p->shortname, p);
	}
    }
  
  return p ? p->pixbuf : NULL;
}

GdkPixbuf *
gpe_find_icon (const char *name)
{
  struct gpe_icon *p = g_datalist_get_data (&pbdata, name);

  if (p == NULL)
    {
      gchar *error = g_strdup_printf (_("Icon \"%s\" not loaded"), name);
      gpe_error_box (error);
      g_free (error);
      exit (1);
    }

  return p->pixbuf;
}

gboolean
gpe_find_icon_pixmap (const char *name, GdkPixmap **pixmap, GdkBitmap **bitmap)
{
  GdkPixbuf *pixbuf = gpe_find_icon (name);
  gdk_pixbuf_render_pixmap_and_mask (pixbuf,
				     pixmap,
				     bitmap,
				     127);
  return TRUE;
}

void
gpe_set_window_icon (GtkWidget *window, gchar *icon)
{
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  gtk_widget_realize (window);

  gpe_find_icon_pixmap (icon, &pmap, &bmap);
  gdk_window_set_icon (window->window, NULL, pmap, bmap);
}
