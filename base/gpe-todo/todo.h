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

#endif
