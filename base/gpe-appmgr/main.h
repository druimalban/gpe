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

#include "cfg.h"

/* main window */
GtkWidget *window;
GtkWidget *notebook;
GtkWidget *recent_box;

/* The items */
GList *items;
GList *groups;

/* Refresh the UI with the icons */
void refresh_tabs (void);

/* Creates the image/label combo for a tab */
GtkWidget *create_tab_label (char *name, char *icon_file, GtkStyle *style);
GtkWidget *create_group_tab_label (char *group, GtkStyle *style);

/* Creates a tab */
GtkWidget *create_tab (GList *all_items, char *current_group, tab_view_style style);

/* Create and prepend the 'All' tab */
void create_all_tab ();

/* Refreshes the tab labels for autohide */
void autohide_labels (int page);

/* (Re)create the recent list */
void create_recent_box();

char *get_icon_fn (struct package *p, int iconsize);

#endif
