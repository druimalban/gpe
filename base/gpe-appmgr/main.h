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

#include "package.h"
#include "cfg.h"

/* main window */
GtkWidget *window;
GtkWidget *notebook;
GtkWidget *recent_box;

/* The items */
GList *items;
GList *groups;

/* Refresh the UI with the icons */
extern void refresh_tabs (void);

extern char *get_icon_fn (struct package *p, int iconsize);

extern GtkWidget *create_icon_pixmap (GtkStyle *style, char *fn, int size);

extern void run_package (struct package *p);

#endif
