/*****************************************************************************
 * gsoko/gfx.c : display management
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

/* [dir][step][X/Y] */
//const static int offset[5][5][2] = {
//	{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
//	{{0, 0}, {-4,0}, {-12,0}, {-20,0}, {-28,0}},	/* left */
//	{{0, 0}, { 4,0}, { 12,0}, { 20,0}, { 28,0}},	/* right */
//	{{0, 0}, {0,-4}, {0,-12}, {0,-20}, {0,-28}},	/* up */
//	{{0, 0}, {0, 4}, {0, 12}, {0, 20}, {0, 28}}};	/* down */

const static int offset[5][5][2] = {
	{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
	{{0, 0}, {-1,0}, {-3,0}, {-5,0}, {-7,0}},	/* left */
	{{0, 0}, { 1,0}, { 3,0}, { 5,0}, { 7,0}},	/* right */
	{{0, 0}, {0,-1}, {0,-3}, {0,-5}, {0,-7}},	/* up */
	{{0, 0}, {0, 1}, {0, 3}, {0, 5}, {0, 7}}};	/* down */

/* draw the character onto pixmap */
GdkRectangle draw_s (void)
{
	GdkRectangle rect;
	/* offset for the box */
	const static int o_box[5][2] = {{0,0},{-W_TILE,0},{W_TILE,0},{0,-H_TILE},{0,H_TILE}};
	/* offset for the bloc box+character */
	const static int o_bloc[5][2] = {{0,0},{-W_TILE,0},{0,0},{0,-H_TILE},{0,0}};
	/* width and heigth of the bloc */
	const static int w_bloc[5][2] = {{0,0},{2*W_TILE,H_TILE},{2*W_TILE,H_TILE},{W_TILE,2*H_TILE},{W_TILE,2*H_TILE}};
	
	rect.x = W_TILE * s_i;
	rect.y = H_TILE * s_j;

	rect.x += s_undo * offset[s_dir][s_step][0];
	rect.y += s_undo * offset[s_dir][s_step][1];

	if (s_push)	/* if we are pushing a box */	
	{
		/* draw ps_pxm onto darea_pxm trough darea_gc */
		gdk_draw_pixbuf(
			darea_pxm, darea_gc,
			ps_pxm[s_dir][s_step], 0, 0,
			rect.x, rect.y, W_S, H_S, GDK_RGB_DITHER_NORMAL, 0, 0);

		/* draw the pushed box */
		gdk_draw_pixbuf(
			darea_pxm, darea_gc,
			box_pxm, 0, 0,
			rect.x + o_box[s_dir][0], rect.y + o_box[s_dir][1], W_TILE, H_TILE, GDK_RGB_DITHER_NORMAL, 0, 0);
		
		rect.x += o_bloc[s_dir][0];
		rect.y += o_bloc[s_dir][1];
		rect.width = w_bloc[s_dir][0];
		rect.height = w_bloc[s_dir][1];
	}
	else
	{
		gdk_draw_pixbuf(
			darea_pxm, darea_gc,
			s_pxm[s_dir][s_step], 0, 0,
			rect.x, rect.y, W_S, H_S, GDK_RGB_DITHER_NORMAL, 0, 0);

		rect.width = W_S;
		rect.height = H_S;
	}

	return rect;
}

/* erase the character from pixmap */
GdkRectangle erase_s (void)
{
	GdkRectangle rect;

	rect.x = W_TILE * s_i;
	rect.y = H_TILE * s_j;

	rect.x += s_undo * offset[s_dir][s_step][0];
	rect.y += s_undo * offset[s_dir][s_step][1];

	rect.width = W_S;
	rect.height = H_S;
		
	restore_bg(rect);

	return rect;
}

/* refresh darea */
void refresh_darea (GdkRectangle rect)
{
	gtk_widget_draw(darea, &rect);
}

/* restore the background */
void restore_bg (GdkRectangle rect)
{
	int imin, imax, jmin, jmax;
	int i, j;

	imin = rect.x / W_TILE;
	imax = (rect.x + rect.width - 1) / W_TILE;

	jmin = rect.y / H_TILE;
	jmax = (rect.y + rect.height - 1) / H_TILE;

	for (i = imin; i <= imax; i++)
	for (j = jmin; j <= jmax; j++)
		draw_tile(i, j);
}

/* draw one tile */
void draw_tile (int i, int j)
{
	switch (board[i][j])
	{
		case 0:
			gdk_draw_pixbuf(darea_pxm, darea_gc, tile_pxm, 0, 0, i * W_TILE, j * H_TILE, W_TILE, H_TILE, GDK_RGB_DITHER_NORMAL, 0, 0);
			break;
		case 1:
			gdk_draw_pixbuf(darea_pxm, darea_gc, tile2_pxm, 0, 0, i * W_TILE, j * H_TILE, W_TILE, H_TILE, GDK_RGB_DITHER_NORMAL, 0, 0);
			break;
		case 2:
			gdk_draw_pixbuf(darea_pxm, darea_gc, wall_pxm, 0, 0, i * W_TILE, j * H_TILE, W_TILE, H_TILE, GDK_RGB_DITHER_NORMAL, 0, 0);
			break;
		case 3:
			gdk_draw_pixbuf(darea_pxm, darea_gc, box_pxm, 0, 0, i * W_TILE, j * H_TILE, W_TILE, H_TILE, GDK_RGB_DITHER_NORMAL, 0, 0);
			break;
		case 4:
			gdk_draw_pixbuf(darea_pxm, darea_gc, box2_pxm, 0, 0, i * W_TILE, j * H_TILE, W_TILE, H_TILE, GDK_RGB_DITHER_NORMAL, 0, 0);
			break;
	}
}

