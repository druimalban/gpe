/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef PIXMAPS_H
#define PIXMAPS_H

struct pix
{
  const char *shortname;
  const char *filename;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
};

extern gboolean load_pixmaps (struct pix *);

extern struct pix *find_pixmap (const char *name);

#endif
