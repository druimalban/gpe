/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>

typedef enum
{
  PAGE,
  GROUP,
  ITEM_MULTI_LINE,
  ITEM_SINGLE_LINE,
  ITEM_DATE,
  ITEM_IMAGE
} edit_thing_type;

struct edit_thing
{
  edit_thing_type type;
  gchar *name;
  gchar *tag;
  GSList *children;
  gboolean hidden;
  struct edit_thing *parent;
};

typedef struct edit_thing *edit_thing_t;

extern GtkWidget *edit_structure (void);
extern GtkWidget *edit_window (void);

extern void print_structure (FILE *);
extern gboolean read_structure (gchar *);
extern void initial_structure (void);

extern GSList *edit_pages;

extern void load_well_known_tags (void);

#endif
