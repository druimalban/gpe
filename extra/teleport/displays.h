/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef DISPLAYS_H
#define DISPLAYS_H

#include <glib.h>

struct display
{
  gchar *host;
  guint dpy;
  guint screen;
  gchar *str;
};

extern GSList *displays;

extern void displays_init (void);

extern struct display *add_display (const gchar *host, int dpy, int screen);

extern void remove_display (struct display *d);

#endif
