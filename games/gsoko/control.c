/*****************************************************************************
 * gsoko/control.c : control of possible/impossible moves
 *****************************************************************************
 * Copyright (C) 2000 Jean-Michel Grimaldi
 *
 * Author: Jean-Michel Grimaldi <jm@via.ecp.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *****************************************************************************/

#include "gsoko.h"

/* return TRUE if we can move from (i,j) towards dir, FALSE else */
int is_mvt(int i, int j, int dir)
{
	int ox, oy;
	ox = mv_result[dir][0];
	oy = mv_result[dir][1];
	
	switch (board[i + ox][j + oy])
	{
		case 0:	/* empty square */
			return TRUE;
		case 1:	/* container */
			return TRUE;
		case 2:	/* wall */
			return FALSE;
		case 3:	/* box */
		case 4: /* 'contained' box */
			/* OK if the next square is empty */
			if (board[i + 2 * ox][j + 2 * oy] < 2)
			{
				s_push = 1;
				return TRUE;
			}
			else
				return FALSE;
		default:
			return FALSE;
	}
}
