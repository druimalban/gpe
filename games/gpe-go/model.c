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
#include <glib.h>
#include <stdlib.h> //malloc() free()

#include "gpe-go.h"
#include "model.h"
#include "board.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext(_x)

#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif

Hitem * new_hitem(){
  Hitem * hitem;
  hitem = (Hitem *) malloc (sizeof(Hitem));
  return hitem;
}

void free_hitem(Hitem * hitem){
  if(hitem){
    if(hitem->comment) free(hitem->comment);
    free(hitem);
  }
}

gboolean is_same_hitem(Hitem * a, Hitem * b){
  if(1
     && a->col    == b->col
     && a->row    == b->row
     && a->item   == b->item
     && a->action == b->action) return TRUE;
  return FALSE;
}

void append_hitem(GoAction action, GoItem item, int col, int row){
  Hitem * hitem;
  Hitem * citem; //item to compare to
  GNode * node;
  gboolean found;

  hitem = new_hitem();

  hitem->action = action;
  hitem->item   = item;
  hitem->col   = col;
  hitem->row   = row;
  hitem->comment = NULL;

  // if the node/data exist, move to this node,
  // do NOT create a new branch (child) (== no twins!)
  // (in case undo, then replay on the same place)
  node = g_node_first_child(go.game.history);
  found = FALSE;
  while(node && !found){
    citem = node->data;
    if(is_same_hitem(hitem, citem)){
      go.game.history = node; //and that's it
      found = TRUE;
    }
    node = node->next;
  }
  if(!found) go.game.history = g_node_append_data(go.game.history, hitem);

}

gboolean delete_hitem(GNode * node, gpointer unused_data){
  if(node->data) free_hitem(node->data);
  return FALSE;
}

GoItem color_to_play(){
  return (go.game.turn%2)?WHITE_STONE:BLACK_STONE;
}

void update_last_played_mark_to(int next_col, int next_row);
void pass_turn(){
  append_hitem(PASS, color_to_play(), 0, 0);
  go.game.turn++;
  update_last_played_mark_to(0, 0);

  status_update_current();
}

gboolean is_on_main_branch(GNode * node){
  GNode * n = go.game.main_branch;
  while(n != go.game.history_root){
    if(n == node) return TRUE;
    n = n->parent;
  }
  return FALSE;
}

GNode * get_first_leaf(GNode * node){
  while(!G_NODE_IS_LEAF(node)){
    node = node->children;//children point on the first child.
  }
  return node;
}

char ** new_table(int size){
  int i,j;
  char ** table;
  //one unused row/col to have index from *1* to grid_size.
  table = (char **) malloc ((size +1) * sizeof(char *));
  for (i=0; i<= size; i++){
    table[i] = (char *) malloc ((size +1) * sizeof(char));
  }
  for(i=1; i<= size; i++) for(j=1; j<=size; j++) table[i][j]=0;
  return table;
}

void free_table(char ** table, int size){
  int i;
  for(i=0; i<= size; i++){
    if(table[i]) free(table[i]);
  }
  if(table) free(table);
}

void init_new_game(int game_size){
  int free_space;
  int old_game_size;

  old_game_size = go.game.size;
  go.game.size = game_size;

  TRACE("init NEW GAME %d", go.game.size);

  status_update_fmt("New game %dx%d", go.game.size, go.game.size);

  //dimensions //CLEAN move to go_board_init()
  go.board.size = BOARD_SIZE;
  go.board.stone_space = 1;
  go.board.grid_stroke = 1;

  go.board.cell_size = go.board.size / go.game.size;
  if ((go.board.cell_size - go.board.grid_stroke - go.board.stone_space) %2) go.board.cell_size--;

  free_space = go.board.size - go.game.size * go.board.cell_size;
  if(free_space < 2){
    go.board.cell_size--;
    free_space += go.game.size;
  }
  go.board.margin = free_space / 2;
  
  go.board.stone_size = go.board.cell_size - go.board.stone_space;

  //stones grid
  if(go.game.grid != NULL) free_table(go.game.grid, old_game_size);
  go.game.grid = new_table(go.game.size);

  //stamps grid
  if(go.game.stamps != NULL) free_table(go.game.stamps, old_game_size);
  go.game.stamps = new_table(go.game.size);

  //--init game
  if(go.game.history_root != NULL){
    g_node_traverse ( go.game.history_root,
                      G_PRE_ORDER,
                      G_TRAVERSE_ALL,
                      -1, //gint max_depth,
                      delete_hitem,
                      NULL);
    g_node_destroy (go.game.history_root);
  }
  go.game.history_root = g_node_new (NULL);
  go.game.history      = go.game.history_root;
  go.game.main_branch  = go.game.history_root;

  go.game.white_captures = 0;
  go.game.black_captures = 0;
  update_capture_label();

  go.game.turn = 0;

  go.game.last_col = 0;
  go.game.last_row = 0;

  go.board.lock_variation_choice = FALSE;

  scale_graphics();
  paint_board(go.board.drawing_area);//CLEAN give the board
}


void _print_grid(){
  int i,j;
  g_print("     ");
  for(j=1; j<= go.game.size; j++) g_print("%2d ", j);
  g_print("\n");
  for(j=1; j<= go.game.size; j++){
    g_print("%2d : ", j);
    for(i=1; i<=go.game.size; i++){
      if(go.game.grid[i][j] == 0){
        if(go.game.stamps[i][j]) g_print(" * ");
        else                g_print(" - ");
      }
      else{
        if(go.game.stamps[i][j]) g_print("*%d*",go.game.grid[i][j]);
        else g_print("%2d ",go.game.grid[i][j]);
      }
    }
    g_print("\n");
  }
}

void _print_hitem(Hitem * hitem){
  char action;
  char item;

  if(!hitem){
    g_print("hitem NULL");
    return;
  }

  switch(hitem->action){
  case PASS   : action = '='; break;
  case PLAY   : action = '+'; break;
  case CAPTURE: action = '-'; break;
  default: action = ' ';
  }
  switch(hitem->item){
  case BLACK_STONE: item = 'B'; break;
  case WHITE_STONE: item = 'W'; break;
  default: item = ' ';
  }
  g_print("%c %c: %d %d", action, item, hitem->col, hitem->row);
}

gboolean _print_node(GNode *node, gpointer unused_data){
  int i;
  int depth;
  depth = g_node_depth(node);
  for(i=1; i<depth; i++) g_print("| ");
  _print_hitem(node->data);
  g_print ("\n");
  return FALSE;
}

void _print_history(){
  g_node_traverse ( go.game.history_root,
                    //G_IN_ORDER, G_PRE_ORDER, G_POST_ORDER, or G_LEVEL_ORDER
                    G_PRE_ORDER,
                    //G_TRAVERSE_ALL, G_TRAVERSE_LEAFS and G_TRAVERSE_NON_LEAFS
                    G_TRAVERSE_ALL,
                    -1, //gint max_depth,
                    _print_node,
                    NULL);
}

void update_last_played_mark_to(int next_col, int next_row){
  TRACE("update LAST PLAYED mark");
  if(go.game.last_col && go.game.grid[go.game.last_col][go.game.last_row] != EMPTY){
    paint_stone(go.game.last_col, go.game.last_row, go.game.grid[go.game.last_col][go.game.last_row]);
  }
  if(next_col) paint_mark(next_col, next_row, MARK_SQUARE, &red);
  go.game.last_col = next_col;
  go.game.last_row = next_row;
}

void put_stone(GoItem color, int col, int row){
  go.game.grid[col][row] = color;
  paint_stone(col, row, color);
}

void remove_stone(int col, int row){
  if(go.game.grid[col][row] == EMPTY) return;
  go.game.grid[col][row] = EMPTY;
  unpaint_stone(col, row);
}


void clear_grid(){
  int i,j;
  for(i=0; i<= go.game.size; i++)
    for(j=0; j<=go.game.size; j++)
      go.game.grid[i][j] = EMPTY;
}

gboolean is_stamped (int col, int row){return go.game.stamps[col][row];}
void stamp(int col, int row){
  if(!go.game.stamps[col][row]) go.game.stamps[col][row] = 1;
}
void unstamp(int col, int row){
  if(go.game.stamps[col][row]) go.game.stamps[col][row] = 0;
}
void clear_stamps(){
  int i,j;
  for(j=1; j<= go.game.size; j++){
    for(i=1; i<=go.game.size; i++){
      if(go.game.stamps[i][j]) go.game.stamps[i][j] = 0;
    }
  }
}

void stamp_group_of(int col, int row){
  int x,y;

  int my_color;
  my_color = go.game.grid[col][row];

  stamp(col,row);

  x = col;
  y = row - 1;
  if(row>1 && go.game.grid[x][y] == my_color && !go.game.stamps[x][y]){
    stamp_group_of(x,y);
  }
  x = col + 1;
  y = row;
  if(col<go.game.size && go.game.grid[x][y] == my_color && !go.game.stamps[x][y]){
    stamp_group_of(x,y);
  }
  x = col;
  y = row + 1;
  if(row<go.game.size && go.game.grid[x][y] == my_color && !go.game.stamps[x][y]){
    stamp_group_of(x,y);
  }
  x = col - 1;
  y = row;
  if(col>1 && go.game.grid[x][y] == my_color && !go.game.stamps[x][y]){
    stamp_group_of(x,y);
  }
}

gboolean is_alive_group_of(int col, int row){
  int my_color;
  int x,y;
  my_color = go.game.grid[col][row];

  stamp(col,row);

  x = col;     //NORTH
  y = row - 1;
  if(row>1 && !go.game.stamps[x][y]){
    if(go.game.grid[x][y] == EMPTY) return TRUE;
    else if( go.game.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = col + 1; //EAST
  y = row;
  if(col<go.game.size && !go.game.stamps[x][y]){
    if(go.game.grid[x][y] == EMPTY) return TRUE;
    else if( go.game.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = col;     //SOUTH
  y = row + 1;
  if(row<go.game.size && !go.game.stamps[x][y]){
    if(go.game.grid[x][y] == EMPTY) return TRUE;
    else if( go.game.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = col - 1; //WEST
  y = row;
  if(col>1 && !go.game.stamps[x][y]){
    if(go.game.grid[x][y] == EMPTY) return TRUE;
    else if( go.game.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }

  return FALSE;
}

/* returns the number of stones killed */
int kill_group_of(int col, int row){
  int count;
  int i,j;

  count = 0;
  clear_stamps();
  stamp_group_of(col, row);

  for(i=1; i<= go.game.size; i++){
    for(j=1; j<=go.game.size; j++){
      if(go.game.stamps[i][j]){

        append_hitem(CAPTURE, go.game.grid[i][j], i, j);

        remove_stone(i,j);

        count++;
      }
    }
  }
  clear_stamps();
  return count;
}

void redo_turn();
void play_at(int col, int row){
  if( col < 1 || go.game.size < col ||
      row < 1 || go.game.size < row ){
    return;
  }
  if(go.board.lock_variation_choice){
    if(!is_stamped(col, row)) return;
    redo_turn();
    return;
  }
  if(go.game.grid[col][row] != EMPTY){
    //NOTE: according to SGF, a stone must be placed,
    //      "no matter what was there before"
    //      ==> check != EMPTY on GUI event level
    //          to allow such thing from SGF file
    return;
  }

  
  {//--play the stone
    GoItem color;
    color = color_to_play();
    put_stone(color, col, row);
    append_hitem(PLAY, color, col, row);
  }

  {//--check if groups to kill
    int opp_color;
    int x, y;

    int * captured;

    if(go.game.grid[col][row] == WHITE_STONE){
      opp_color = BLACK_STONE;
      captured  = & go.game.white_captures;
    }
    else{
      opp_color = WHITE_STONE;
      captured  = & go.game.black_captures;
    }

    clear_stamps();
    x = col;
    y = row - 1;
    if(row>1 && go.game.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = col + 1;
    y = row;
    if(col<go.game.size && go.game.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = col;
    y = row + 1;
    if(row<go.game.size && go.game.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = col - 1;
    y = row;
    if(col>1 && go.game.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
  }
  update_capture_label();

  //--Check for suicide
  if(!is_alive_group_of(col, row)){
    //gpe_error_box("SUICIDE");

    //disallow suicides:
    remove_stone(col, row);
    {
      GNode * node = go.game.history;
      go.game.history = go.game.history->parent;
      delete_hitem(node, NULL);
      g_node_destroy(node);
    }
  }
  else{ //--play the stone
    GoItem color;
    color = color_to_play();
    update_last_played_mark_to(col, row);

    status_update_current();

    //--next turn
    go.game.turn++;
  }

}

void variation_choice(){
  int i;
  GNode * child;
  int children;
  children = g_node_n_children(go.game.history);
  //--display possible variations, and restrict input to them (lock)
  
  clear_stamps();
  for(i=0; i<children; i++){
    Hitem * item;
    child = g_node_nth_child (go.game.history, i);
    item = child->data;
    
    stamp(item->col, item->row);
    paint_mark(item->col, item->row, MARK_SPOT, &red);
    if(is_on_main_branch(child)){
      //add extra mark on main branch's move
      paint_mark(item->col, item->row, MARK_SQUARE, &blue);
      go.game.variation_main_branch_node = child;
    }
  }
  go.board.lock_variation_choice = TRUE;
  gtk_statusbar_push(GTK_STATUSBAR(go.ui.status), 1, _("Variation choice"));
}

void clear_variation_choice(){
    int i;
    int children;
    GNode * child;

    children = g_node_n_children(go.game.history);
    //unpaint SPOTS
    for(i=0; i<children; i++){
      Hitem * item;
      child = g_node_nth_child (go.game.history, i);
      item = child->data;
      unpaint_stone(item->col, item->row);
    }
    clear_stamps();
    go.board.lock_variation_choice = FALSE;  
    gtk_statusbar_pop (GTK_STATUSBAR(go.ui.status), 1);
}

void undo_turn(){
  Hitem * hitem;

  if(go.board.lock_variation_choice){
    clear_variation_choice();
    return;
  }

  if(!go.game.history || go.game.turn == 0) return;

  //keep a ref to the leaf of the main branch
  if(G_NODE_IS_LEAF(go.game.history)) go.game.main_branch = go.game.history;

  hitem = go.game.history->data;

  while(hitem->action == CAPTURE){
    put_stone(hitem->item, hitem->col, hitem->row);

    if(hitem->item == BLACK_STONE) go.game.white_captures--;
    else go.game.black_captures--;

    if(go.game.history->parent) go.game.history = go.game.history->parent;
    hitem = go.game.history->data;
  }
  update_capture_label();

  if(hitem->action == PLAY){
    remove_stone(hitem->col, hitem->row);
    go.game.turn--;
  }
  else if(hitem->action == PASS){
    go.game.turn--;
  }

  if(go.game.history->parent){
    go.game.history = go.game.history->parent;
    
    if(!G_NODE_IS_ROOT(go.game.history)){
      GNode * hist;
      Hitem * item;
      hist = go.game.history;
      item = hist->data;
      while(item->action == CAPTURE){
        hist = hist->parent;
        item = hist->data;
      } 
      update_last_played_mark_to(item->col, item->row);
    }
  }

  status_update_current();

  if(g_node_n_children(go.game.history) > 1){
    variation_choice();
  }
}

void redo_turn(){
  int children;
  Hitem * hitem = NULL;

  if(!go.game.history || G_NODE_IS_LEAF(go.game.history)){
    TRACE("================do nothing");
    return;
  }

  if(G_NODE_IS_ROOT(go.game.history)) go.game.history = go.game.history->children;

  //check variations
  children = g_node_n_children(go.game.history);
  if(go.game.turn >= 1 && children >= 1){

    GNode * choosen_child;
    choosen_child = g_node_first_child (go.game.history);

    if(children > 1){
      if(go.board.lock_variation_choice == FALSE){
        variation_choice();
        return;
      }
      else{
        int i;
        GNode * child;

        //select the child        
        if(!is_stamped(go.game.col, go.game.row)){
          //play the one on the main branch
          choosen_child = go.game.variation_main_branch_node;
        }
        else{
          for(i=0; i<children; i++){
            Hitem * item;
            child = g_node_nth_child (go.game.history, i);
            item = child->data;
            if(item->col == go.game.col && item->row == go.game.row){
              choosen_child = child;
              i = children;
            }
          }
          //change the main branch
          go.game.main_branch = get_first_leaf(choosen_child);
        }

        clear_variation_choice();
      }
    }//if variations
    
    go.game.history = choosen_child;
  }

  hitem = go.game.history->data;

  if(hitem->action == PASS){
    TRACE("***> rePASS");
    update_last_played_mark_to(0, 0);
    go.game.turn++;
  }
  else if(hitem->action == PLAY){
    TRACE("***> rePLAY %d %d", hitem->col, hitem->row);
    put_stone(hitem->item, hitem->col, hitem->row);

    update_last_played_mark_to(hitem->col, hitem->row);

    go.game.turn++;

    {//revert capture if there are.
      GNode * node;
      if(g_node_n_children(go.game.history) >= 1){

        node = g_node_nth_child (go.game.history, 0);
        hitem = node->data;

        while(hitem && hitem->action == CAPTURE){
          TRACE("***> remove %d %d", hitem->col, hitem->row);
          remove_stone(hitem->col, hitem->row);
          if(hitem->item == BLACK_STONE) go.game.white_captures++;
          else go.game.black_captures++;
          
          go.game.history = node;
          if(g_node_n_children(go.game.history) >= 1){
            node = g_node_nth_child (go.game.history, 0);
            hitem = node->data;
          }
          else hitem = NULL;
        }
        update_capture_label();
      }
    }

  }
  status_update_current();
}

void goto_first(){
  //keep a ref to the leaf of the main branch
  if(G_NODE_IS_LEAF(go.game.history)) go.game.main_branch = go.game.history;

  go.game.history = go.game.history_root;

  {
    go.game.white_captures = 0;
    go.game.black_captures = 0;    
    go.game.turn = 0;
    go.game.last_col = 0;
    go.game.last_row = 0;
    go.board.lock_variation_choice = FALSE;

    clear_grid();
  }

  paint_board(go.board.drawing_area);
}

void goto_last(){
  GNode * node;
  Hitem * hitem;

  if(G_NODE_IS_LEAF(go.game.history)) return;
  do{
    node = go.game.history->children;
    while(!is_on_main_branch(node)){
      node = node->next;
    }
    hitem = node->data;
    if(hitem->action == PASS){
      pass_turn();
    }
    else{
      play_at(hitem->col, hitem->row);
    }
    node = go.game.history;
  }while(!G_NODE_IS_LEAF(node));
}
