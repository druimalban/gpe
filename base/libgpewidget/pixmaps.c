/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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
  
  if (filename[0] == '/')
    {
      pathname = filename;
    }
  else
    {
      snprintf (buf, sizeof (buf) - 1, "%s/%s.png", theme_dir, filename);
      buf[sizeof (buf) - 1] = 0;
      if (access (buf, F_OK) != 0 && theme_dir != default_theme_dir)
	{
	  snprintf (buf, sizeof (buf) - 1, "%s/%s.png", default_theme_dir, 
		    filename);
	  buf[sizeof (buf) - 1] = 0;
	}
      pathname = buf;
    }
  
  pb = gdk_pixbuf_new_from_file (pathname);

  if (pb == NULL && error)
    {
      snprintf (buf, sizeof (buf) - 1, _("Unable to load icon \"%s\""),
		filename);
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

  if (home == NULL)
    home = "/";

  if (theme_dir)
    abort ();

  s = strlen (home) + strlen (theme_dir_tail) + 1;
  buf = alloca (s);
  strcpy (buf, home);
  strcat (buf, theme_dir_tail);
  
  if (access (buf, F_OK) == 0)
    theme_dir = g_strdup (buf);
  else
    theme_dir = default_theme_dir;
 
  g_datalist_init (&pbdata);
  
  while (p->shortname)
    {
      gchar *error;
      p->pixbuf = gpe_load_one_icon (p->filename ? p->filename : p->shortname,
				     &error);

      if (p->pixbuf == NULL)
	{
	  gpe_error_box (error);
	  g_free (error);
	  exit (1);
	}

      g_datalist_set_data (&pbdata, p->shortname, p);
      p++;
    }

  return TRUE;
}

GdkPixbuf *
gpe_find_icon (const char *name)
{
  struct gpe_icon *p = g_datalist_get_data (&pbdata, name);
  if (p == NULL)
    {
      GdkPixbuf *buf = gpe_load_one_icon (name, NULL);
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

gboolean
gpe_find_icon_pixmap (const char *name, GdkPixmap **pixmap, GdkBitmap **bitmap)
{
  GdkPixbuf *pixbuf = gpe_find_icon (name);
  if (pixbuf)
    {
      gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					 pixmap,
					 bitmap,
					 127);      
      return TRUE;
    }
  return FALSE;
}

