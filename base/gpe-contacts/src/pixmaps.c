/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <gdk_imlib.h>

#include "pixmaps.h"

static GData *pdata;

gboolean 
load_pixmaps (struct pix *p)
{
  g_datalist_init (&pdata);
  
  while (p->shortname && p->filename)
    {
      if (! gdk_imlib_load_file_to_pixmap (p->filename, 
					   &p->pixmap, &p->mask))
	{
	  gpe_perror_box (p->filename);
	  return FALSE;
	}

      g_datalist_set_data (&pdata, p->shortname, p);
      p++;
    }

  return TRUE;
}

struct pix *
find_pixmap (const char *name)
{
  struct pix *p = g_datalist_get_data (&pdata, name);

  return p;
}

