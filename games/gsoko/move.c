/*****************************************************************************
 * gsoko/move.c : animation and commands handling
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

/* [direction][X/Y] */
const int mv_result[5][2] = {{0,0}, {-1,0}, {1,0}, {0,-1}, {0,1}};

/* callback for the animation timeout */
int move_callback(gpointer data)
{
	/* 0 indicates that this is a recursive call */
	move_s (0);

	return FALSE;
}

/* callback for the undo move */
void undo_move(void)
{
	/* -1 indicates that this is a undo move call */
	move_s (-1);

}

/* indicate that we want to move towards dir */
void move_s (int dir)
{
	GdkRectangle r1;
	static GdkRectangle r2;
	static int next_dir;
	int i, j, ok, mem_dir;
	
	switch(s_step)
	{
	        case 0:	/* start */
		        mem_dir = s_dir;
			if (dir)	/* a key was pressed */
			{
				r1 = erase_s();
				s_dir = dir;
			}
			else  /* this is a recursive call */
			{
				r1 = r2;
				restore_bg(r1);
				s_dir = next_dir;
			}
			next_dir = 0;
			if (s_dir == -1) /* this is an undo */
			{
			    s_undo = -1;
			    if (hist[s_hist].dir) {
				s_dir = hist[s_hist].dir;
				s_push = hist[s_hist].push;
				hist[s_hist].dir = 0;
				hist[s_hist].push = 0;
				s_hist = (s_hist) ? s_hist - 1 : HIST_SIZE - 1;
				ok = 1;
			    }
			    else { /* history is empty */
				s_dir = mem_dir;
				s_push = 0;
				ok = 0;
			    }
			}
	                else {
			    s_undo = 1;
			    ok = is_mvt(s_i, s_j, s_dir);
			    if (ok) {
				s_hist = (s_hist < HIST_SIZE - 1) ? s_hist + 1 : 0;
				hist[s_hist].dir = s_dir;
				hist[s_hist].push = s_push;
			    }
			}
			r2 = draw_s();
			if (ok)	/* if the movement is allowed */
			{
				if (s_push)	/* if we are pushing a box */
				{
					i = s_i + mv_result[s_dir][0];
					j = s_j + mv_result[s_dir][1];
					/* remove it from its original location: 3->0 and 4->1 */
					board[i][j] -= 3;
					if (board[i][j])	/* if the box was over a container */
						nbox++;
				}
				s_step = 1;
				gtk_timeout_add(ANIM_DELAY, move_callback, NULL);
			}
			refresh_darea(r1);
			refresh_darea(r2);
			break;

		case 1:
		case 2:
		case 3:	/* keep walking */
			if (dir) break;	/* don't listen to the user too early */
			r1 = r2;
			restore_bg(r1);
			s_step++;
			gtk_timeout_add(ANIM_DELAY, move_callback, NULL);
			r2 = draw_s();
			refresh_darea(r1);
			refresh_darea(r2);
			break;

		case 4:	/* last step */
			if (dir)	/* now listen to the user */
			{
				next_dir = dir;
				break;
			}
			/* updates the coordinates after the move */
			s_i += s_undo * mv_result[s_dir][0];
			s_j += s_undo * mv_result[s_dir][1];
			/* count the move (cannot be put after because of the possible 'new_level()') */
			nmoves += s_undo;
			if (s_push)	/* if we were pushing a box */
			{
				i = s_i + mv_result[s_dir][0];
				j = s_j + mv_result[s_dir][1];
				/* put it to its new location */
				if (board[i][j])	/* if it is put over a container */
					nbox--;
				board[i][j] += 3;
				/* refresh this new box */
				draw_tile(i,j);
				r1.x = i * W_TILE;
				r1.y = j * H_TILE;
				r1.width = W_TILE;
				r1.height = H_TILE;
				refresh_darea(r1);
				/* stop pushing */
				s_push = 0;
/*				g_print ("%i boxes left\n", nbox);*/
				if (!nbox)	/* if all the boxes are 'contained' */
					next_level();
			}
			/* update the title (cannot be put before because of the possible 'new_level()') */
			make_title();
			s_step = 0;
			if (next_dir)	/* keep moving */
			    move_callback(NULL);
			else	/* stop */
			{
				r1 = r2;
				restore_bg(r1);
				r2 = draw_s();
				refresh_darea(r1);
				refresh_darea(r2);
			}
			break;
	}
}

