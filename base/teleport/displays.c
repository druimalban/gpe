/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "displays.h"

GSList *displays;

struct display *
add_display (gchar *host, int dpy, int screen)
{
  struct display *d = g_malloc (sizeof (struct display));

  d->host = g_strdup (host);
  d->dpy = dpy;
  d->screen = screen;
  d->str = g_strdup_printf ("%s:%d.%d", host, dpy, screen);

  displays = g_slist_append (displays, d);

  return d;
}

void
remove_display (struct display *d)
{
  displays = g_slist_remove (displays, d);
  g_free (d->host);
  g_free (d->str);
  g_free (d);
}

void
displays_init (void)
{
}
