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
#include <stdio.h>  //sscanf()

#include "gpe-go.h"//CLEAN do not include. work with GoGame as parameter!
#include "sgf-handler.h"

#include "sgf.h"
#include "model.h"

#include "gpe/errorbox.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext(_x)

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

/* ################################################################ */
/* ########################## load game ########################### */

GQueue * branches;
gboolean is_first_branch;
GNode * loaded_main_branch;//to avoid overwritting by undo()/play_at() calls

void load_game(const char * filename){
  //catch the potential errors
  load_sgf_file(filename);
}

void sgf_parsed_init(){
  is_first_branch = TRUE;
  loaded_main_branch = NULL;
  branches = NULL;
}
void sgf_parsed_end(){
  is_first_branch = TRUE;
  go.game.main_branch = loaded_main_branch;
}

void sgf_parsed_open_gametree(){
  if(branches == NULL) branches = g_queue_new();
  g_queue_push_head(branches, go.game.history);
}

void sgf_parsed_close_gametree(){
  GNode * hist;
  hist = g_queue_pop_head(branches);

  if(is_first_branch){ is_first_branch = FALSE;}

  while(go.game.history != hist){
    undo_turn();  
    //stop when closing the first node (depth == 2)
    if(g_node_depth(go.game.history) == 2) return;
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
    if(go.game.history && go.game.history->data){
      Hitem * hitem;
      hitem = go.game.history->data;
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
      play_at(col - 'a' + 1, row - 'a' + 1);/* [a..z] --> [1..26] */
    }
    //keep a ref to the main branch
    if(is_first_branch){
      loaded_main_branch = go.game.history;
    }
    break;
  default:
    //unknown or not handled property.
    break;
  }
}

/* ################################################################ */
/* ########################## save game ########################### */

void _save_hitem_sgf(FILE * f, Hitem * hitem){
//  ;B[am]BL[1791];W[al]WL[1799]
  char item;

  if(!hitem) return;
  if(hitem->item == BLACK_STONE) item = 'B'; else item = 'W';
  if     (hitem->action == PASS) fprintf(f, ";%c[tt]", item);//FIXME: support "[]"
  else if(hitem->action == PLAY) fprintf(f, ";%c[%c%c]", item,
                                         hitem->col + 'a' -1,
                                         hitem->row + 'a' -1);
  if(hitem->comment){
    gchar * s;
    GString * string;

    string = g_string_sized_new(1000);

    g_string_append(string, "C[");

    s = hitem->comment;
    while(*s){
      switch(*s){
        case '[':
        case ']':
        case '\\':
          g_string_append_c (string, '\\');
      }
      g_string_append_c(string, *s);
      s++;
    }

    g_string_append_c(string, ']');

    fprintf(f, "%s", string->str);

    g_string_free(string, TRUE);
  }
}

void _save_tree_to_sgf_from (GNode *node, FILE * f){
  gboolean has_siblings = FALSE;

  if(node->parent && g_node_n_children(node->parent) > 1){
    has_siblings = TRUE;
    fprintf(f, "\n (");
  }

  if(node->data) _save_hitem_sgf(f, node->data);

  node = node->children;
  while (node){
      _save_tree_to_sgf_from (node, f);
      node = node->next;
  }

  if(has_siblings) fprintf(f, " )\n");  
}

void save_game(const char * filename){
  FILE * f;

  f = fopen(filename, "w");
  if(!f){
    gpe_error_box_fmt (_("Cannot save the game into %s."), filename);
    return;
  }

  /* header */
  fprintf(f, "(;GM[1]FF[3]\n");
  fprintf(f, "RU[Japanese]SZ[%d]\n", go.game.size);
  fprintf(f, "PW[%s]\n", "white");
  fprintf(f, "PB[%s]\n", "black");

  /* tree */
  _save_tree_to_sgf_from (go.game.history_root, f);

  /* footer */
  fprintf(f, "\n)\n");

  fclose(f);
}
