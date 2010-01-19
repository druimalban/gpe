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

#ifndef PIM_CATEGORIES_UI_H
#define PIM_CATEGORIES_UI_H

#include <gtk/gtk.h>

#ifdef IS_HILDON
extern GtkWidget *gpe_pim_categories_dialog (GSList *selected_categories, gboolean select, GCallback callback, gpointer data);
extern GtkWidget *gpe_pim_categories_list_window (gboolean select);
#else
extern GtkWidget *gpe_pim_categories_dialog (GSList *selected_categories, GCallback callback, gpointer data);
extern GtkWidget *gpe_pim_categories_list_window (void);
#endif
void gpe_pim_categories_reset_window (GtkWidget *w, GSList *selected_categories);
extern GSList *gpe_pim_categories_from_window (GtkWidget *w);

#endif
