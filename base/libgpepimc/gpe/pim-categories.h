/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *               2006 Florian Boor <florian.boor@kernelconcepts.de>
 *               2007 Graham R. Cobb <g+gpe@cobb.uk.net>
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

#include <glib.h>

/* basic functions */
extern gboolean gpe_pim_categories_init (void);
extern GSList *gpe_pim_categories_list (void);

extern gboolean gpe_pim_category_new (const gchar *title, gint *id);
extern const gchar *gpe_pim_category_name (gint id);
extern gint gpe_pim_category_id (const gchar *name);
extern gboolean gpe_pim_category_rename (gint id, gchar *new_name);
extern void gpe_pim_category_delete (gint id);

/* colour functions */
extern gboolean gpe_pim_category_set_colour (gint id, const gchar *new_colour);
extern const gchar *gpe_pim_category_colour (gint id);

#endif
