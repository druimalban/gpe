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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include "l-cell.h"
#include "l-grid.h"

LGrid *
l_grid_new (void)
{
  LGrid *grid = g_new0 (LGrid, 1);

  grid->cells_alive = g_tree_new_full (l_cell_cmp, NULL, g_free, g_free);
  return grid;
}

void
l_grid_destroy (LGrid * grid)
{
  if (grid)
    {
      l_cell_free (grid->min);
      l_cell_free (grid->max);

      if (grid->cells_alive)
	g_tree_destroy (grid->cells_alive);
    }
}

void
l_grid_cell_set (LGrid * grid, gint x, gint y)
{
  if (grid && grid->cells_alive)
    {
      gboolean *value = g_new0 (gboolean, 1);

      (*value) = TRUE;
      g_tree_replace (grid->cells_alive, l_cell_new (x, y), value);

      if (grid->min)
        {
          if (x < grid->min->x)
            grid->min->x = x;
          if (y < grid->min->y)
            grid->min->y = y;
	}
      else
        {
	  grid->min = l_cell_new (x, y);
	}
      if (grid->max)
        {
          if (x > grid->max->x)
            grid->max->x = x;
          if (y > grid->max->y)
            grid->max->y = y;
	}
      else
        {
	  grid->max = l_cell_new (x, y);
	}
    }
}

void
l_grid_cell_unset (LGrid * grid, gint x, gint y)
{
  if (grid && grid->cells_alive)
    {
      LCell *c = l_cell_new (x, y);

      g_tree_remove (grid->cells_alive, c);
      l_cell_free (c);
    }
}

gboolean
l_grid_cell_is_set (LGrid * grid, gint x, gint y)
{
  if (grid && grid->cells_alive)
    {
      LCell *c = l_cell_new (x, y);

      if (g_tree_lookup (grid->cells_alive, c))
	{
	  l_cell_free (c);
	  return TRUE;
	}
      l_cell_free (c);
    }
  return FALSE;
}

void
l_grid_cell_toggle (LGrid * grid, gint x, gint y)
{
  if (l_grid_cell_is_set (grid, x, y))
    l_grid_cell_unset (grid, x, y);
  else
    l_grid_cell_set (grid, x, y);
}

/*
void
l_grid_resize (LGrid *grid, gint min_x, gint min_y, gint max_x, gint max_y)
{
  if (grid)
    {
      l_cell_position_set (grid->min, min_x, min_y);
      l_cell_position_set (grid->max, max_x, max_y);
    }
}
*/

gint
l_grid_neighbours_alive (LGrid * grid, gint x, gint y)
{
  gint alive = 0;
  gint i, k;

  for (i = x - 1; i < x + 2; i++)
    for (k = y - 1; k < y + 2; k++)
      if ((i != x || k != y) && l_grid_cell_is_set (grid, i, k))
	alive++;

  return alive;
}

typedef struct {
  LGrid *old;
  LGrid *new;
  LGrid *zombies;
} LGridForeachHelper;

gboolean
l_grid_find_survivors (gpointer key, gpointer value, gpointer data)
{
  LGrid *old_grid = ((LGridForeachHelper *) data)->old;
  LGrid *new_grid = ((LGridForeachHelper *) data)->new;
  LGrid *zombie_grid = ((LGridForeachHelper *) data)->zombies;
  gint x, y;

  if (*(gboolean *) value)	/* this shouldn't be FALSE actually */
    {
      l_cell_position_get ((LCell *) key, &x, &y);
      gint n = l_grid_neighbours_alive (old_grid, x, y);

      /* g_print ("checking if (%d,%d) survives:", x, y); */
      if (n == 2 || n == 3)
	l_grid_cell_set (new_grid, x, y);
      else
        l_grid_cell_set (zombie_grid, x, y);
      /* g_print ("%s\n", l_grid_cell_is_set (new_grid, x, y) ? "yes" : "no"); */
    }
  else
    {
      g_assert_not_reached();
    }
  return FALSE;
}

gboolean
l_grid_find_newborn (gpointer key, gpointer value, gpointer data)
{
  LGrid *old_grid = ((LGridForeachHelper *) data)->old;
  LGrid *new_grid = ((LGridForeachHelper *) data)->new;
  gint x, y;
  gint i, k;

  if (*(gboolean *) value)	/* this shouldn't be FALSE actually */
    {
      l_cell_position_get ((LCell *) key, &x, &y);
      for (i = x-1; i < x + 2; i++)
	for (k = y-1; k < y + 2; k++)
	  if (i != x || k != y)
	    {
              /* g_print ("checking if (%d,%d) will be born:", i, k); */
	      if (l_grid_neighbours_alive (old_grid, i, k) == 3)
	        l_grid_cell_set (new_grid, i, k);
              /*g_print ("%s\n", l_grid_cell_is_set (new_grid, i, k) ? "yes" : "no"); */
	    }
    }
  else
    {
      g_assert_not_reached();
    }
  return FALSE;
}

void
l_grid_next_generation (LGrid * grid)
{
  LGrid old;
  LGridForeachHelper helper;

  old.cells_alive = grid->cells_alive;
  old.min = grid->min;
  old.max = grid->max;

  grid->cells_alive = g_tree_new_full (l_cell_cmp, NULL, g_free, g_free);
  grid->min = NULL;
  grid->max = NULL;

  helper.old = &old;
  helper.new = grid;
  helper.zombies = l_grid_new ();

  /* g_print ("calculating next generation.\n"); */
  g_tree_foreach (old.cells_alive, l_grid_find_survivors, &helper);
  g_tree_foreach (old.cells_alive, l_grid_find_newborn, &helper);
  g_tree_destroy (old.cells_alive);

  l_grid_destroy (grid->zombies);
  grid->zombies = helper.zombies;
}

typedef struct {
  LGridForeachFunc func;
  gpointer user_data;
} LGridTraverseInfo;

gboolean
traverse_cells_tree (gpointer key, gpointer value, gpointer data)
{
  if (*(gboolean *) value)	/* this shouldn't be FALSE actually */
    {
      LCell *c = (LCell *) key;
      LGridTraverseInfo *info = (LGridTraverseInfo *) data;

      ((LGridForeachFunc) (*info->func)) (c->x, c->y, info->user_data);
    }
  else
    {
      g_assert_not_reached();
    }
  return FALSE;
}

void
l_grid_foreach (LGrid *grid, LGridForeachFunc func, gpointer user_data)
{
  if (grid && grid->cells_alive)
    {
      LGridTraverseInfo *info = g_new0 (LGridTraverseInfo, 1);

      info->func = func;
      info->user_data = user_data;
      g_tree_foreach (grid->cells_alive, traverse_cells_tree, info);

      g_free (info);
    }
}

void
l_grid_get_size (LGrid *grid, gint *min_x, gint *min_y, gint *max_x, gint *max_y)
{
  if (grid && grid->min && grid->max)
    {
      l_cell_position_get (grid->min, min_x, min_y);
      l_cell_position_get (grid->max, max_x, max_y);
      return;
    }

  (*min_x) = (*min_y) = (*max_x) = (*max_y) = 0;
}
