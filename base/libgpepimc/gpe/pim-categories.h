/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef PIM_CATEGORIES_H
#define PIM_CATEGORIES_H

#include <gtk/gtk.h>

struct gpe_pim_category
{
  const gchar *name;
  guint id;
};

extern gboolean gpe_pim_categories_init (void);
extern GSList *gpe_pim_categories_list (void);

extern gboolean gpe_pim_category_new (const gchar *title, gint *id);

extern GtkWidget *gpe_pim_categories_dialog (GSList *selected_categories, GCallback callback, gpointer data);

#endif
