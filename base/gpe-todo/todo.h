/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef TODO_H
#define TODO_H

#include <sys/time.h>
#include <gtk/gtk.h>
#include <gpe/todo-db.h>

extern GtkWidget *g_draw;
extern GtkWidget *the_notebook;

extern GtkWidget *edit_item (struct todo_item *item,
                             gint initial_category,
                             GtkWindow *parent);

extern void categories_menu (void);
extern void refresh_items (void);
extern void gpe_todo_exit (void);

/* screen geometry information */
extern gboolean mode_landscape;
extern gboolean large_screen;

/* list store column definitions */
enum
{
    COL_ICON,
    COL_SUMMARY,
    COL_STRIKETHROUGH,
    COL_DATA,
    COL_STATUS,
    COL_DUE,
    COL_CATEGORY,
    COL_PRIORITY,
    COL_PRIORITY_TEXT,
    COL_COLORS,
    NUM_COLUMNS
};

struct menu_map
{
  gchar *string;
  gint value;
};

extern struct menu_map state_map[];

#endif
