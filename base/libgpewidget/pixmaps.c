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

static GData *pdata;
static const gchar *theme_dir_tail = "/.gpe/pixmaps";
static const gchar *default_theme_dir = "/usr/local/share/gpe/pixmaps/default";

#define _(x) gettext(x)

gboolean 
gpe_load_pixmaps (struct pix *p)
{
  gchar *home = getenv ("HOME");
  gchar *buf;
  const gchar *theme_dir;
  size_t s;
  if (home == NULL)
    home = "/";

  s = strlen (home) + strlen (theme_dir_tail) + 1;
  buf = alloca (s);
  strcpy (buf, home);
  strcat (buf, theme_dir_tail);

  if (access (buf, F_OK) == 0)
    theme_dir = buf;
  else
    theme_dir = default_theme_dir;
 
  g_datalist_init (&pdata);
  
  while (p->shortname && p->filename)
    {
      const gchar *filename;
      char buf[1024];
      GdkPixbuf *pixbuf;

      if (p->filename[0] == '/')
	{
	  filename = p->filename;
	}
      else
	{
	  snprintf (buf, sizeof (buf) - 1, "%s/%s.png", theme_dir, p->filename);
	  buf[sizeof (buf) - 1] = 0;
	  if (access (buf, F_OK) != 0 && theme_dir != default_theme_dir)
	    {
	      snprintf (buf, sizeof (buf) - 1, "%s/%s.png", default_theme_dir, 
			p->filename);
	      buf[sizeof (buf) - 1] = 0;
	    }
	  filename = buf;
	}

      pixbuf = gdk_pixbuf_new_from_file (filename);
      if (pixbuf == NULL)
	{
	  char buf[1024];
	  sprintf (buf, _("Unable to load %s"), filename);
	  gpe_error_box (buf);
	  exit (1);
	}
      gdk_pixbuf_render_pixmap_and_mask (pixbuf,
					 &p->pixmap,
					 &p->mask,
					 1);

      g_datalist_set_data (&pdata, p->shortname, p);
      p++;
    }

  return TRUE;
}

gboolean 
load_pixmaps () __attribute__ ((weak, deprecated, alias ("gpe_load_pixmaps")));

struct pix *
gpe_find_pixmap (const char *name)
{
  struct pix *p = g_datalist_get_data (&pdata, name);

  return p;
}

struct pix *
find_pixmap () __attribute__ ((weak, deprecated, alias ("gpe_find_pixmap")));

