/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifndef MAIN_H
#define MAIN_H

#include <gtk/gtk.h>
#include <gpe/desktop_file.h>

#include "cfg.h"

struct package_group
{
  gchar *name;
  GList *categories;
  GList *items;
  gboolean hide;
};

/* main window */
extern GtkWidget *window;
extern GtkWidget *notebook;
extern GtkWidget *recent_box;

/* The items */
extern GList *items;
extern GList *groups;

/* Refresh the UI with the icons */
extern void refresh_tabs (void);

extern GtkWidget *create_icon_pixmap (GtkStyle *style, char *fn, int size);

extern char *get_icon_fn (GnomeDesktopFile *p, int iconsize);

extern void run_package (GnomeDesktopFile *p, GObject *item);

extern gchar *only_group;

extern Display *dpy;

extern gboolean flag_desktop;
extern gboolean flag_rows;

#endif
