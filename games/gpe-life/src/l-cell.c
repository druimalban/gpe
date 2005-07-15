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

LCell *
l_cell_new (gint x, gint y)
{
  LCell *c = g_new0 (LCell, 1);

  c->x = x;
  c->y = y;

  return c;
}

void
l_cell_free (LCell * cell)
{
  if (cell)
    g_free (cell);
}

void
l_cell_position_set (LCell * cell, gint x, gint y)
{
  if (cell)
    {
      cell->x = x;
      cell->y = y;
    }
}

void
l_cell_position_get (LCell * cell, gint * x, gint * y)
{
  if (cell)
    {
      if (x)
	(*x) = cell->x;
      if (y)
	(*y) = cell->y;
    }
}

gint
l_cell_cmp (gconstpointer a_ptr, gconstpointer b_ptr, gpointer ignored)
{
  LCell *a = (LCell *) a_ptr;
  LCell *b = (LCell *) b_ptr;

  if (a->x < b->x)
    {
      return -1;
    }
  else if (a->x == b->x)
    {
      if (a->y < b->y)
	return -1;
      else if (a->y == b->y)
	return 0;
      else
	return 1;
    }
  return 1;
}
