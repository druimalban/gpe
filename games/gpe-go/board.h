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

#define BOARD_SIZE 240 //pixels

extern GdkColor red;
extern GdkColor blue;

GtkWidget * go_board_new();

void load_graphics();
void scale_graphics();

void paint_board(GtkWidget * widget);

/* optional last args for specific marks */
void paint_mark(int col, int row, GoMark mark, GdkColor * color, ...);
void paint_stone(int col, int row, GoItem item);
void unpaint_stone(int col, int row);

#endif
