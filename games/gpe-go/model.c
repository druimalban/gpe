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
  node = g_node_first_child(go.history);
  found = FALSE;
  while(node && !found){
    citem = node->data;
    if(is_same_hitem(hitem, citem)){
      go.history = node; //and that's it
      found = TRUE;
    }
    node = node->next;
  }
  if(!found) go.history = g_node_append_data(go.history, hitem);

}

gboolean delete_hitem(GNode * node, gpointer unused_data){
  if(node->data) free_hitem(node->data);
  return FALSE;
}

GoItem color_to_play(){
  return (go.turn%2)?WHITE_STONE:BLACK_STONE;
}

void update_last_played_mark_to(int next_col, int next_row);
void pass_turn(){
  append_hitem(PASS, color_to_play(), 0, 0);
  go.turn++;
  update_last_played_mark_to(0, 0);

  status_update_current();
}

gboolean is_on_main_branch(GNode * node){
  GNode * n = go.main_branch;
  while(n != go.history_root){
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

  old_game_size = go.game_size;
  go.game_size = game_size;

  TRACE("init NEW GAME %d", go.game_size);

  status_update_fmt("New game %dx%d", go.game_size, go.game_size);

  //dimensions
  go.board_size = BOARD_SIZE;
  go.stone_space = 1;
  go.grid_stroke = 1;

  go.cell_size = go.board_size / go.game_size;
  if ((go.cell_size - go.grid_stroke - go.stone_space) %2) go.cell_size--;

  free_space = go.board_size - go.game_size * go.cell_size;
  if(free_space < 2){
    go.cell_size--;
    free_space += go.game_size;
  }
  go.margin = free_space / 2;
  
  go.stone_size = go.cell_size - go.stone_space;

  //stones grid
  if(go.grid != NULL) free_table(go.grid, old_game_size);
  go.grid = new_table(go.game_size);

  //stamps grid
  if(go.stamps != NULL) free_table(go.stamps, old_game_size);
  go.stamps = new_table(go.game_size);

  //--init game
  if(go.history_root != NULL){
    g_node_traverse ( go.history_root,
                      G_PRE_ORDER,
                      G_TRAVERSE_ALL,
                      -1, //gint max_depth,
                      delete_hitem,
                      NULL);
    g_node_destroy (go.history_root);
  }
  go.history_root = g_node_new (NULL);
  go.history      = go.history_root;
  go.main_branch  = go.history_root;

  go.white_captures = 0;
  go.black_captures = 0;
  update_capture_label();

  go.turn = 0;

  go.last_col = 0;
  go.last_row = 0;

  go.lock_variation_choice = FALSE;

  scale_graphics();
  paint_board(go.drawing_area);
}


void _print_grid(){
  int i,j;
  g_print("     ");
  for(j=1; j<= go.game_size; j++) g_print("%2d ", j);
  g_print("\n");
  for(j=1; j<= go.game_size; j++){
    g_print("%2d : ", j);
    for(i=1; i<=go.game_size; i++){
      if(go.grid[i][j] == 0){
        if(go.stamps[i][j]) g_print(" * ");
        else                g_print(" - ");
      }
      else{
        if(go.stamps[i][j]) g_print("*%d*",go.grid[i][j]);
        else g_print("%2d ",go.grid[i][j]);
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
  g_node_traverse ( go.history_root,
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
  if(go.last_col && go.grid[go.last_col][go.last_row] != EMPTY){
    paint_stone(go.last_col, go.last_row, go.grid[go.last_col][go.last_row]);
  }
  if(next_col) paint_mark(next_col, next_row, MARK_SQUARE, &red);
  go.last_col = next_col;
  go.last_row = next_row;
}

void put_stone(GoItem color, int col, int row){
  go.grid[col][row] = color;
  paint_stone(col, row, color);
}

void remove_stone(int col, int row){
  if(go.grid[col][row] == EMPTY) return;
  go.grid[col][row] = EMPTY;
  unpaint_stone(col, row);
}


void clear_grid(){
  int i,j;
  for(i=0; i<= go.game_size; i++)
    for(j=0; j<=go.game_size; j++)
      go.grid[i][j] = EMPTY;
}

gboolean is_stamped (int col, int row){return go.stamps[col][row];}
void stamp(int col, int row){
  if(!go.stamps[col][row]) go.stamps[col][row] = 1;
}
void unstamp(int col, int row){
  if(go.stamps[col][row]) go.stamps[col][row] = 0;
}
void clear_stamps(){
  int i,j;
  for(j=1; j<= go.game_size; j++){
    for(i=1; i<=go.game_size; i++){
      if(go.stamps[i][j]) go.stamps[i][j] = 0;
    }
  }
}

void stamp_group_of(int col, int row){
  int x,y;

  int my_color;
  my_color = go.grid[col][row];

  stamp(col,row);

  x = col;
  y = row - 1;
  if(row>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    stamp_group_of(x,y);
  }
  x = col + 1;
  y = row;
  if(col<go.game_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    stamp_group_of(x,y);
  }
  x = col;
  y = row + 1;
  if(row<go.game_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    stamp_group_of(x,y);
  }
  x = col - 1;
  y = row;
  if(col>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    stamp_group_of(x,y);
  }
}

gboolean is_alive_group_of(int col, int row){
  int my_color;
  int x,y;
  my_color = go.grid[col][row];

  stamp(col,row);

  x = col;     //NORTH
  y = row - 1;
  if(row>1 && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = col + 1; //EAST
  y = row;
  if(col<go.game_size && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = col;     //SOUTH
  y = row + 1;
  if(row<go.game_size && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = col - 1; //WEST
  y = row;
  if(col>1 && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
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

  for(i=1; i<= go.game_size; i++){
    for(j=1; j<=go.game_size; j++){
      if(go.stamps[i][j]){

        append_hitem(CAPTURE, go.grid[i][j], i, j);

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
  if( col < 1 || go.game_size < col ||
      row < 1 || go.game_size < row ){
    return;
  }
  if(go.lock_variation_choice){
    if(!is_stamped(col, row)) return;
    redo_turn();
    return;
  }
  if(go.grid[col][row] != EMPTY){
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

    if(go.grid[col][row] == WHITE_STONE){
      opp_color = BLACK_STONE;
      captured  = & go.white_captures;
    }
    else{
      opp_color = WHITE_STONE;
      captured  = & go.black_captures;
    }

    clear_stamps();
    x = col;
    y = row - 1;
    if(row>1 && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = col + 1;
    y = row;
    if(col<go.game_size && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = col;
    y = row + 1;
    if(row<go.game_size && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = col - 1;
    y = row;
    if(col>1 && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
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
      GNode * node = go.history;
      go.history = go.history->parent;
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
    go.turn++;
  }

}

void variation_choice(){
  int i;
  GNode * child;
  int children;
  children = g_node_n_children(go.history);
  //--display possible variations, and restrict input to them (lock)
  
  clear_stamps();
  for(i=0; i<children; i++){
    Hitem * item;
    child = g_node_nth_child (go.history, i);
    item = child->data;
    
    stamp(item->col, item->row);
    paint_mark(item->col, item->row, MARK_SPOT, &red);
    if(is_on_main_branch(child)){
      //add extra mark on main branch's move
      paint_mark(item->col, item->row, MARK_SQUARE, &blue);
      go.variation_main_branch_node = child;
    }
  }
  go.lock_variation_choice = TRUE;
  gtk_statusbar_push(GTK_STATUSBAR(go.status), 1, _("Variation choice"));
}

void clear_variation_choice(){
    int i;
    int children;
    GNode * child;

    children = g_node_n_children(go.history);
    //unpaint SPOTS
    for(i=0; i<children; i++){
      Hitem * item;
      child = g_node_nth_child (go.history, i);
      item = child->data;
      unpaint_stone(item->col, item->row);
    }
    clear_stamps();
    go.lock_variation_choice = FALSE;  
    gtk_statusbar_pop (GTK_STATUSBAR(go.status), 1);
}

void undo_turn(){
  Hitem * hitem;

  if(go.lock_variation_choice){
    clear_variation_choice();
    return;
  }

  if(!go.history || go.turn == 0) return;

  //keep a ref to the leaf of the main branch
  if(G_NODE_IS_LEAF(go.history)) go.main_branch = go.history;

  hitem = go.history->data;

  while(hitem->action == CAPTURE){
    put_stone(hitem->item, hitem->col, hitem->row);

    if(hitem->item == BLACK_STONE) go.white_captures--;
    else go.black_captures--;

    if(go.history->parent) go.history = go.history->parent;
    hitem = go.history->data;
  }
  update_capture_label();

  if(hitem->action == PLAY){
    remove_stone(hitem->col, hitem->row);
    go.turn--;
  }
  else if(hitem->action == PASS){
    go.turn--;
  }

  if(go.history->parent){
    go.history = go.history->parent;
    
    if(!G_NODE_IS_ROOT(go.history)){
      GNode * hist;
      Hitem * item;
      hist = go.history;
      item = hist->data;
      while(item->action == CAPTURE){
        hist = hist->parent;
        item = hist->data;
      } 
      update_last_played_mark_to(item->col, item->row);
    }
  }

  status_update_current();

  if(g_node_n_children(go.history) > 1){
    variation_choice();
  }
}

void redo_turn(){
  int children;
  Hitem * hitem = NULL;

  if(!go.history || G_NODE_IS_LEAF(go.history)){
    TRACE("================do nothing");
    return;
  }

  if(G_NODE_IS_ROOT(go.history)) go.history = go.history->children;

  //check variations
  children = g_node_n_children(go.history);
  if(go.turn >= 1 && children >= 1){

    GNode * choosen_child;
    choosen_child = g_node_first_child (go.history);

    if(children > 1){
      if(go.lock_variation_choice == FALSE){
        variation_choice();
        return;
      }
      else{
        int i;
        GNode * child;

        //select the child        
        if(!is_stamped(go.col, go.row)){
          //play the one on the main branch
          choosen_child = go.variation_main_branch_node;
        }
        else{
          for(i=0; i<children; i++){
            Hitem * item;
            child = g_node_nth_child (go.history, i);
            item = child->data;
            if(item->col == go.col && item->row == go.row){
              choosen_child = child;
              i = children;
            }
          }
          //change the main branch
          go.main_branch = get_first_leaf(choosen_child);
        }

        clear_variation_choice();
      }
    }//if variations
    
    go.history = choosen_child;
  }

  hitem = go.history->data;

  if(hitem->action == PASS){
    TRACE("***> rePASS");
    update_last_played_mark_to(0, 0);
    go.turn++;
  }
  else if(hitem->action == PLAY){
    TRACE("***> rePLAY %d %d", hitem->col, hitem->row);
    put_stone(hitem->item, hitem->col, hitem->row);

    update_last_played_mark_to(hitem->col, hitem->row);

    go.turn++;

    {//revert capture if there are.
      GNode * node;
      if(g_node_n_children(go.history) >= 1){

        node = g_node_nth_child (go.history, 0);
        hitem = node->data;

        while(hitem && hitem->action == CAPTURE){
          TRACE("***> remove %d %d", hitem->col, hitem->row);
          remove_stone(hitem->col, hitem->row);
          if(hitem->item == BLACK_STONE) go.white_captures++;
          else go.black_captures++;
          
          go.history = node;
          if(g_node_n_children(go.history) >= 1){
            node = g_node_nth_child (go.history, 0);
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
  if(G_NODE_IS_LEAF(go.history)) go.main_branch = go.history;

  go.history = go.history_root;

  {
    go.white_captures = 0;
    go.black_captures = 0;    
    go.turn = 0;
    go.last_col = 0;
    go.last_row = 0;
    go.lock_variation_choice = FALSE;

    clear_grid();
  }

  paint_board(go.drawing_area);
}

void goto_last(){
  GNode * node;
  Hitem * hitem;

  if(G_NODE_IS_LEAF(go.history)) return;
  do{
    node = go.history->children;
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
    node = go.history;
  }while(!G_NODE_IS_LEAF(node));
}
