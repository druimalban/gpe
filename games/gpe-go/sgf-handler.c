/* gpe-go
 *
 * Copyright (C) 2003 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <glib.h>

#include "sgf-handler.h"

#include "sgf.h"
#include "model.h"

#include "gpe/errorbox.h"

#ifdef DEBUG
void __print_hitem (Hitem * hitem);
#endif

/*
 (;GM[1]FF[3]
 RU[Japanese]SZ[19]
 PW[white]
 PB[black]
 ;B[ef]
   (;W[op];B[nd];W[dl];B[ip])
   (;W[jd];B[gd];W[nd])  
 )
*/
GQueue * branches;
gboolean is_first_branch;
GNode * loaded_main_branch;//to avoid overwritting by undo()/play_at() calls

void sgf_parsed_init(){
  is_first_branch = TRUE;
  loaded_main_branch = NULL;
  branches = NULL;
}
void sgf_parsed_end(){
  is_first_branch = TRUE;
  go.main_branch = loaded_main_branch;
}

void sgf_parsed_open_gametree(){
  if(branches == NULL) branches = g_queue_new();
  g_queue_push_head(branches, go.history);
}

void sgf_parsed_close_gametree(){
  GNode * hist;
  hist = g_queue_pop_head(branches);

  if(is_first_branch){ is_first_branch = FALSE;}

  while(go.history != hist){
    undo_turn();  
    //stop when closing the first node (depth == 2)
    if(g_node_depth(go.history) == 2) return;
  }  
}


void sgf_parsed_prop_int(PropIdent prop_id, int value){
  switch(prop_id){
  case SYMBOL_FF:
    //expected 1, 3 or 4
    break;
  case SYMBOL_GM:
    if(value != 1) gpe_error_box("Found GM != 1, continue parsing anyway");
    break;
  case SYMBOL_SZ:
    init_new_game(value);
    break;
  default:
    //unknown or not handled property.
    break;
  }
}

void sgf_parsed_prop_string(PropIdent prop_id, const char * s){
  switch(prop_id){
  case SYMBOL_C:
    if(go.history && go.history->data){
      Hitem * hitem;
      hitem = go.history->data;
      hitem->comment = g_strdup(s);
    }
    break;
  case SYMBOL_RU:
  case SYMBOL_PW:
  case SYMBOL_PB:
    break;
  default:
    //unknown or not handled property.
    break;
  }
}

void sgf_parsed_prop_move(PropIdent prop_id, char row, char col){
  switch(prop_id){
  case SYMBOL_W:
  case SYMBOL_B:
    if(col == 't' && row == 't'){
      pass_turn();
    }
    else {
      play_at(col - 'a' +1, row - 'a' + 1);
    }
    //keep a ref to the main branch
    if(is_first_branch){
      loaded_main_branch = go.history;
    }
    break;
  default:
    //unknown or not handled property.
    break;
  }
}


#ifdef DEBUG
void __print_hitem (Hitem * hitem)    {
  char action;
  char item;
  
  if(!hitem) return;
  
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
  g_print("***********> %c %c: %d %d\n", action, item, hitem->col, hitem->row);
}
#endif
