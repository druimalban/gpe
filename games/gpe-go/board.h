/* gpe-go, a GO board for GPE
 *
 * $Id$
 *
 * Copyright (C) 2003-2004 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef BOARD_H
#define BOARD_H

#include <gtk/gtk.h>

#include "model.h"

#define BOARD_SIZE 240 //pixels CLEAN: remove

extern GdkColor red;
extern GdkColor blue;

typedef struct {
  GtkWidget * widget; /* the top level container */

  GtkWidget * drawing_area;
  GdkPixmap * drawing_area_pixmap_buffer;

  /* graphics */
  GdkPixbuf * loaded_board;
  GdkPixbuf * loaded_black_stone;
  GdkPixbuf * loaded_white_stone;

  GdkPixbuf * pixbuf_black_stone;
  GdkPixbuf * pixbuf_white_stone;
  GdkPixmap * pixmap_empty_board;

  int size;       //px PARAM - size of the board widget

  int margin;     //***px - left/top margin
  int cell_size;  //***px - == stone_size + stone_space 
  int stone_size; //px
  int stone_space;//px PARAM space between two stones (0/1 px)  
  int grid_stroke;//px PARAM width of the grid stroke (1px)

  gboolean lock_variation_choice;

} GoBoard;

void go_board_init();

void load_graphics();
void scale_graphics();

void paint_board(GtkWidget * widget);

/* optional last args for specific marks */
void paint_mark(int col, int row, GoMark mark, GdkColor * color, ...);
void paint_stone(int col, int row, GoItem item);
void unpaint_stone(int col, int row);

#endif
