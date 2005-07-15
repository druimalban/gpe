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

#ifndef __L_CELL_H__
#define __L_CELL_H__

struct _LCell
{
  gint x;
  gint y;
};
typedef struct _LCell LCell;

LCell *l_cell_new (gint x, gint y);

void l_cell_free (LCell * cell);

void l_cell_position_set (LCell * cell, gint x, gint y);

void l_cell_position_get (LCell * cell, gint * x, gint * y);

gint l_cell_cmp (gconstpointer a_ptr, gconstpointer b_ptr, gpointer ignored);

#endif /* __L_CELL_H__ */
