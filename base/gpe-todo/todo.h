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

typedef enum
{
  NOT_STARTED,
  IN_PROGRESS,
  COMPLETED
} item_state;

struct todo_item
{
  int id, pos;
  time_t time;
  const char *what;
  const char *summary;
  item_state state;
  GSList *categories;
#if GTK_MAJOR_VERSION >= 2
  PangoLayout *layout;
#endif
};

struct todo_category
{
  const char *title;
  int id;
};

extern GtkWidget *g_draw;
extern GtkWidget *the_notebook;
extern GSList *categories, *items;

extern struct todo_item *new_item (void);
extern GtkWidget *edit_item (struct todo_item *item);
extern void push_item (struct todo_item *i);
extern void delete_item (struct todo_item *i);

extern struct todo_category *new_category (const char *title);
extern void del_category (struct todo_category *);

extern int hide;

extern void configure (GtkWidget *w, gpointer list);

extern void categories_menu (void);

extern void refresh_items (void);

extern gboolean category_matches (GSList *valid, guint id);

#endif
