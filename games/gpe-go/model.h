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

#include <glib.h>

void init_new_game(int game_size);

void play_at(int col, int row);

void undo_turn();
void redo_turn();
void pass_turn();

void goto_first();
void goto_last ();

//NOTE: typedef char Stamp;
//NOTE: Stamp  ** stamps;
//NOTE: GoItem ** grid;
typedef struct {

  int size; //9, 13, 19, ...

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

  //Game Tree
  GNode * history_root; //root node
  GNode * history;      //current pointer
  GNode * main_branch;  //ref to the leaf of the main branch
  GNode * variation_main_branch_node;

} GoGame;

//GList * collection; to handle games collections

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
} GoMark;//--> GoMarkType

//struct {
//  GoMarkType type;
//  GdkColor * color;
//  gchar * label;
//  int col;
//  int row;
//} GoMark;

typedef enum {
  PASS,
  PLAY, //USE: PLACE_STONE (includes handicaps), MAKE_MOVE, NONE (?)
  CAPTURE //--> remove
} GoAction;

typedef struct {//FIXME: match SGF specs
  //node type ? (root, setup, move, game-info, ...)
  GoAction action;
  GoItem   item; //--> color
  int col;
  int row;
  //GList * captured; FIXME: to use

  gchar * comment;
  //gchar * annotation;
  //GList * marks;
  //...

  //char * unknown_properties;//from loaded files, to save back
} GoNode;

#endif
