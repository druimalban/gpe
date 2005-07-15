/* GPE Life
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __L_GRID_H__
#define __L_GRID_H__

#include <glib.h>
#include "l-cell.h"

struct _LGrid
{
  LCell *min;
  LCell *max;
  GTree *cells_alive;
  struct _LGrid *zombies;
};
typedef struct _LGrid LGrid;

typedef void (*LGridForeachFunc) (gint, gint, gpointer);

LGrid *
l_grid_new (void);

void
l_grid_destroy (LGrid * grid);

void
l_grid_cell_set (LGrid * grid, gint x, gint y);

void
l_grid_cell_unset (LGrid * grid, gint x, gint y);

gboolean
l_grid_cell_is_set (LGrid * grid, gint x, gint y);

void
l_grid_cell_toggle (LGrid * grid, gint x, gint y);

/*
void
l_grid_resize (LGrid *grid, gint min_x, gint min_y, gint max_x, gint max_y);
*/

gint
l_grid_neighbours_alive (LGrid * grid, gint x, gint y);

gboolean
l_grid_find_survivors (gpointer key, gpointer value, gpointer data);

gboolean
l_grid_find_newborn (gpointer key, gpointer value, gpointer data);

void
l_grid_next_generation (LGrid * grid);

void
l_grid_foreach (LGrid *grid, LGridForeachFunc func, gpointer user_data);

void
l_grid_get_size (LGrid *grid, gint *min_x, gint *min_y, gint *max_x, gint *max_y);

#endif /* __L_GRID_H__ */
