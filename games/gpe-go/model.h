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
#ifndef MODEL_H
#define MODEL_H

//CLEAN: move away GUI stuff
#include <gtk/gtk.h>

void init_new_game(int game_size);

void play_at(int col, int row);

void undo_turn();
void redo_turn();
void pass_turn();

void goto_first();
void goto_last ();

typedef struct _go {
  //--ui
  GtkWidget * window;
  GtkWidget * notebook;

  GtkWidget * status;
  GtkWidget * status_expander;

  GdkPixmap * drawing_area_pixmap_buffer;
  GtkWidget * drawing_area;

  GdkPixbuf * loaded_board;
  GdkPixbuf * loaded_black_stone;
  GdkPixbuf * loaded_white_stone;

  GdkPixbuf * pixbuf_black_stone;
  GdkPixbuf * pixbuf_white_stone;
  GdkPixmap * pixmap_empty_board;

  GtkWidget * capture_label;
  char      * capture_string;

  GtkWidget * file_selector;

  GtkWidget * game_popup_button;

  int selected_game_size;
  GtkWidget * game_size_spiner;

  gboolean save_game;

  GtkWidget * comment_text_view;
  GtkTextBuffer * comment_buffer;
  gboolean comment_edited;

#ifdef TURN_LABEL
  GtkWidget * turn_label;
  char      * turn_string;
#endif

  //--board
  int board_size;//px PARAM - size of the board widget
  int margin;    //***px - left/top margin
  
  int cell_size;  //***px - == stone_size + stone_space 
  int stone_size; //px
  int stone_space;//px PARAM space between two stones (0/1 px)
  
  int grid_stroke;//px PARAM width of the grid stroke (1px)

  //--game
  int game_size; //9, 13, 19, ...

  char ** grid;   //current state of the board
  char ** stamps;

  int white_captures; //stones captured by white!
  int black_captures; 

  int turn;

  //last played
  int last_col;
  int last_row;

  //current position
  int col;
  int row;

  GNode * history_root; //root node
  GNode * history;      //current pointer
  GNode * main_branch;  //ref to the leaf of the main branch
  GNode * variation_main_branch_node;

  gboolean lock_variation_choice;

} Go;

Go go;

typedef enum {
  EMPTY = 0,
  BLACK_STONE,
  WHITE_STONE
} GoItem;

typedef enum {
  NO_MARK,
  MARK_SQUARE,
  MARK_SPOT,
  //MARK_TRIANGLE
  MARK_LABEL
} GoMark;

typedef enum {
  PASS,
  PLAY,
  CAPTURE
} GoAction;

typedef struct _hist_item{
  GoAction action;
  GoItem   item;
  int col;
  int row;
  gchar * comment;
  //GList * captured; FIXME: to use
} Hitem;

#endif
