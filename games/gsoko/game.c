/*****************************************************************************
 * gsoko/game.c : levels management
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

#include <stdio.h>	/* FILE, sprintf() */

#include "gsoko.h"

/* start a new game */
void new_game(void)
{
	level = 1;
	restart();
}

/* restart at current level */
void restart(void)
{
	load_level(level);
}

/* load (and draw) the nth level */
void load_level(int n)
{
	char filename[FILENAME_SIZE];
	char buf;
	FILE *file;
	int i, j;
	GdkRectangle rect;

	/* load the board */
	sprintf(filename, PREFIX "/share/gsoko/levels/%i.lev", n);
	file = fopen(filename, "rb");
	for (j = 0; j < H_BOARD; j++)	/* parse the lines */
	{
		for (i = 0; i < W_BOARD; i++)
		{
			fread(&buf, sizeof(char), 1, file);
			switch (buf)
			{
				case ' ':	/* empty square */
					buf = 0;
					break;
				case 'x':	/* container */
					buf = 1;
					break;
				case '@':	/* wall */
					buf = 2;
					break;
				case 'o':	/* box */
					buf = 3;
					break;
				case '#':	/* 'contained' box */
					buf = 4;
					break;
				case '<':	/* facing left */
					buf = 6;
					break;
				case '>':	/* right */
					buf = 7;
					break;
				case '^':	/* up */
					buf = 8;
					break;
				case 'v':	/* down */
					buf = 9;
					break;
				case '(':	/* left over a container */
					buf = 10;
					break;
				case ')':	/* ... */
					buf = 11;
					break;
				case 'A':
					buf = 12;
					break;
				case 'V':
					buf = 13;
					break;
				default:
					buf = 0;
			}
			board[i][j] = buf;
		}
		/* discard "\n" */
		fread(&buf, sizeof(char), 1, file);
	}
	fclose(file);

	/* parse the board :
	 * - read initial position and direction
	 * - count the boxes */
	nbox = 0;
	for (i = 0; i < W_BOARD; i++)
	for (j = 0; j < H_BOARD; j++)
	{
		switch (board[i][j])
		{
			case 3:
				nbox++;
				break;
			case 6:	/* left */
			case 7:	/* right */
			case 8:	/* up */
			case 9:	/* down	*/
				s_i = i;
				s_j = j;
				s_dir = board[i][j] - 5;
				board[i][j] = 0;
				break;
			case 10:	/* left over a container */
			case 11:	/* ... */
			case 12:
			case 13:
				s_i = i;
				s_j = j;
				s_dir = board[i][j] - 10;
				board[i][j] = 1;
				break;
		}
	}

	/* update the number of moves and the title of the main window */
	nmoves = 0;
	make_title();

	/* character's initial position */
	s_step = 0;	/* not moving */
	s_push = 0;	/* not pushing */
	s_undo = 1;     /* not an undo */

	/* history is empty */
	s_hist = 0;
	for (i = 0; i < HIST_SIZE; i++) {
	  hist[i].dir = hist[i].push = 0;
	}

	/* draw the board */
	rect.x = rect.y = 0;
	rect.width = W_DAREA;
	rect.height = H_DAREA;
	restore_bg(rect);

	/* draw the character */
	draw_s();

	/* update darea */
	refresh_darea(rect);
}

/* go to next level */
void next_level(void)
{
	if (++level > N_LEVELS)
	{
		udidit();
		level--;
	}
	else
		restart();
}
