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

typedef struct {

  int size; //9, 13, 19, ...

  //CLEAN typedef char GridItem;
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
  GNode * history_root; //root node //CLEAN: GNode * tree;
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
} GoMark;

typedef enum {
  PASS,
  PLAY,
  CAPTURE
} GoAction;

typedef struct {//FIXME: match SGF specs
  GoAction action;
  GoItem   item;
  int col;
  int row;
  //GList * captured; FIXME: to use

  gchar * comment;
  //GList * marks;
  //...

  //char * unknown_properties;
} Hitem;//CLEAN: GoNode

#endif
