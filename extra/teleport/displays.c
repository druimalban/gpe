/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>

#include <sqlite.h>

#include "displays.h"

GSList *displays;

static sqlite *sqliteh;

static struct display *
internal_add_display (const gchar *host, int dpy, int screen)
{
  struct display *d = g_malloc (sizeof (struct display));

  d->host = g_strdup (host);
  d->dpy = dpy;
  d->screen = screen;
  d->str = g_strdup_printf ("%s:%d.%d", host, dpy, screen);

  displays = g_slist_append (displays, d);

  return d;
}

struct display *
add_display (const gchar *host, int dpy, int screen)
{
  if (sqliteh)
    sqlite_exec_printf (sqliteh, "insert into display values ('%q', %d, %d)",
			NULL, NULL, NULL, host, dpy, screen);

  return internal_add_display (host, dpy, screen);
}

void
remove_display (struct display *d)
{
  if (sqliteh)
    sqlite_exec_printf (sqliteh, "delete from display where host='%q' and display=%d and screen=%d",
			NULL, NULL, NULL, d->host, d->dpy, d->screen);

  displays = g_slist_remove (displays, d);
  g_free (d->host);
  g_free (d->str);
  g_free (d);
}

static char *schema_info = "create table display (host TEXT, display INTEGER, screen INTEGER)";

static int
display_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 3)
    internal_add_display (argv[0], atoi (argv[1]), atoi (argv[2]));

  return 0;
}

void
displays_init (void)
{
  gchar *home = g_get_home_dir ();
  gchar *filename = g_strdup_printf ("%s/.gpe/migrate/displays", home);
  gchar *err;

  sqliteh = sqlite_open (filename, 0, &err);

  if (sqliteh)
    {
      sqlite_exec (sqliteh, schema_info, NULL, NULL, NULL);
      sqlite_exec (sqliteh, "select * from display",
		   &display_callback, NULL, NULL);
    }
  else
    {
      fprintf (stderr, "Unable to open %s: %s\n", filename, err);
      free (err);
    }
}
