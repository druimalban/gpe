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
#include <gtk/gtk.h>
#include <stdlib.h> //exit()
#include <stdio.h>  //sscanf()
#include <string.h> //strlen()

#include "model.h"
#include "sgf.h"

//--GPE libs
#include "gpe/init.h"
#include "gpe/pixmaps.h"
#include "gpe/errorbox.h"
#include "gpe/question.h"
#include "gpe/popup_menu.h"
#include "gpe/picturebutton.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext(_x)

#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif

static struct gpe_icon my_icons[] = {
  { "this_app_icon", PREFIX "/share/pixmaps/gpe-go.png" },

  { NULL, NULL }
};

#define BOARD_SIZE 240 //pixels

//colors
GdkGC       * gc;
GdkColormap * colormap;
GdkColor red  = {0, 65535, 0,     0};
GdkColor blue = {0,     0, 0, 65535};

enum page_names{
  PAGE_BOARD = 0,
  PAGE_GAME_SETTINGS,
  PAGE_COMMENT_EDITOR,
};

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

void status_update_current();

GoItem color_to_play(){ return (go.turn%2)?WHITE_STONE:BLACK_STONE;}
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

void status_update_fmt(const char * format, ...);
void update_capture_label();
void scale_graphics();
void paint_board(GtkWidget * widget);

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

void app_init(int argc, char ** argv){
  int size = 19;//default value

  if(argc > 1){//first arg is board size if numeric
    sscanf(argv[1], "%d", &size);
  }
  init_new_game(size);
}

void app_quit(){
  gtk_exit (0);
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

void gui_init();

void load_graphics();
int main (int argc, char ** argv){

  if (gpe_application_init (&argc, &argv) == FALSE) exit (1);
  if (gpe_load_icons (my_icons) == FALSE) exit (1);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
  setlocale (LC_ALL, "");

  gui_init ();
  load_graphics();
  app_init (argc, argv);

  gtk_main ();

  return 0;
}


#define PIXMAPS_SRC  PREFIX "/share/gpe/pixmaps/default/" PACKAGE "/"

void load_graphics(){
  //--board and stones
  go.loaded_board       = gdk_pixbuf_new_from_file(PIXMAPS_SRC "board", NULL);
  go.loaded_black_stone = gdk_pixbuf_new_from_file(PIXMAPS_SRC "black.png", NULL);
  go.loaded_white_stone = gdk_pixbuf_new_from_file(PIXMAPS_SRC "white.png", NULL);

  //--colors
  gc = gdk_gc_new(go.drawing_area->window);
  colormap = gdk_colormap_get_system();
  gdk_colormap_alloc_color(colormap, &red ,  FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &blue,  FALSE,TRUE);
}

void scale_graphics(){
  int stone_size = go.stone_size;
  int i;
  int cell_size;
  int border_trans; //grid's translation from the border of the board

  //--black stone
  if(go.pixbuf_black_stone != NULL){
    gdk_pixbuf_unref(go.pixbuf_black_stone);
  }
  if(go.loaded_black_stone != NULL){
    go.pixbuf_black_stone = gdk_pixbuf_scale_simple (go.loaded_black_stone,
                                            stone_size,
                                            stone_size,
                                            GDK_INTERP_BILINEAR);
  }
  else {//draw it yourself
    //gdk_draw_arc(_black_stone,
    //             widget->style->black_gc,
    //             TRUE,
    //             0, 0,
    //             stone_size, stone_size,
    //             0, 23040);//23040 == 360*64
  }
  TRACE("scaled black stone");

  //--white stone
  if(go.pixbuf_white_stone != NULL){
    gdk_pixbuf_unref(go.pixbuf_white_stone);
  }
  if(go.loaded_white_stone != NULL){
    go.pixbuf_white_stone = gdk_pixbuf_scale_simple (go.loaded_white_stone,
                                            stone_size,
                                            stone_size,
                                            GDK_INTERP_BILINEAR);
  }
  else {//draw it yourself
    //gdk_draw_arc(_white_stone,
    //             widget->style->white_gc,
    //             TRUE,
    //             rect.x, rect.y,
    //             rect.width, rect.height,
    //             0, 23040);//23040 == 360*64
    //gdk_draw_arc(_white_stone, //black border
    //             widget->style->black_gc,
    //             FALSE,
    //             rect.x, rect.y,
    //             rect.width, rect.height,
    //               0, 23040);//23040 == 360*64
  }
  TRACE("scaled white stone");

  //--Board
  if(go.pixmap_empty_board == NULL){
    go.pixmap_empty_board = gdk_pixmap_new (go.drawing_area->window,
                                            BOARD_SIZE,
                                            BOARD_SIZE,
                                            -1);
  }
  if(go.loaded_board != NULL){
    GdkPixbuf * scaled_bg;

    scaled_bg = gdk_pixbuf_scale_simple (go.loaded_board,
                                         BOARD_SIZE, BOARD_SIZE,
                                         GDK_INTERP_BILINEAR);
    gdk_draw_pixbuf(go.pixmap_empty_board,
                    go.drawing_area->style->white_gc,
                    scaled_bg,
                    0, 0, 0, 0,
                    -1, -1,
                    GDK_RGB_DITHER_NONE, //GdkRgbDither dither,
                    0, //gint x_dither,
                    0); //gint y_dither);
    gdk_pixbuf_unref(scaled_bg);
  }
  else{//draw it yourself
    gdk_draw_rectangle (go.pixmap_empty_board,
                        go.drawing_area->style->white_gc,//FIXME: use better color
                        TRUE,
                        0, 0,
                        BOARD_SIZE,
                        BOARD_SIZE);
  }
  TRACE("scaled board");

  //grid
  cell_size = go.cell_size;
  border_trans = go.margin + go.cell_size / 2;
  
  for(i=0; i< go.game_size; i++){
    //vertical lines
    gdk_draw_line(go.pixmap_empty_board,
                  go.drawing_area->style->black_gc,
                  border_trans + cell_size * i, border_trans,
                  border_trans + cell_size * i, BOARD_SIZE - border_trans);
    //horizontal lines
    gdk_draw_line(go.pixmap_empty_board,
                  go.drawing_area->style->black_gc,
                  border_trans, border_trans + cell_size * i,
                  BOARD_SIZE - border_trans, border_trans + cell_size * i);
  }
  TRACE("scaled grid");

  //advantage points for 9/13/19 boards

#define ADVANTAGE_POINT_SIZE 3
#define paint_point(x,y) \
      gdk_draw_rectangle(go.pixmap_empty_board, \
               go.drawing_area->style->black_gc, TRUE, \
               border_trans + cell_size * (x-1) - ADVANTAGE_POINT_SIZE/2, \
               border_trans + cell_size * (y-1) - ADVANTAGE_POINT_SIZE/2, \
               ADVANTAGE_POINT_SIZE, ADVANTAGE_POINT_SIZE);

  if(go.game_size == 9){
    paint_point(3, 3);
    paint_point(7, 3);
    paint_point(5, 5);
    paint_point(3, 7);
    paint_point(7, 7);
  }
  else if(go.game_size == 13){
    paint_point( 4,  4);
    paint_point( 4,  7);
    paint_point( 4, 10);
    paint_point( 7,  4);
    paint_point( 7,  7);
    paint_point( 7, 10);
    paint_point(10,  4);
    paint_point(10,  7);
    paint_point(10, 10);
  }
  else if(go.game_size == 19){
    paint_point( 4,  4);
    paint_point( 4, 10);
    paint_point( 4, 16);
    paint_point(10,  4);
    paint_point(10, 10);
    paint_point(10, 16);
    paint_point(16,  4);
    paint_point(16, 10);
    paint_point(16, 16);
  }

}


void paint_board(GtkWidget * widget){
  GdkRectangle rect;
  gdk_draw_drawable (go.drawing_area_pixmap_buffer,
                     widget->style->black_gc,
                     go.pixmap_empty_board,
                     0, 0, 0, 0, BOARD_SIZE, BOARD_SIZE);
  rect.x = rect.y = 0;
  rect.width = rect.height = BOARD_SIZE;
  gtk_widget_draw (go.drawing_area, &rect);
}

void unpaint_stone(int col, int row){
  GdkRectangle rect;

  rect.x = (col -1) * go.cell_size + go.margin;
  rect.y = (row -1) * go.cell_size + go.margin;
  rect.width = rect.height = go.cell_size;

  gdk_draw_drawable (go.drawing_area_pixmap_buffer,
                     go.drawing_area->style->black_gc,
                     go.pixmap_empty_board,
                     rect.x, rect.y, rect.x, rect.y,
                     rect.width, rect.height);
  gtk_widget_draw (go.drawing_area, &rect);
}

void paint_mark(int col, int row, GoMark mark, GdkColor * color){
  GdkRectangle rect;
  int position;
  int size;

  rect.x = go.margin + (col -1) * go.cell_size + go.stone_space;
  rect.y = go.margin + (row -1) * go.cell_size + go.stone_space;
  rect.width = rect.height = go.stone_size;

  TRACE("mark (%d %d) %s", col, row,
        (go.grid[col][row] == BLACK_STONE)?"black":"white");

  gdk_gc_set_foreground(gc, color);
  switch(mark){
    case MARK_SQUARE:
      position = go.stone_size / 4;
      size = go.stone_size / 2;
      if( !((size - go.grid_stroke)%2) ) size++;

      gdk_draw_rectangle (go.drawing_area_pixmap_buffer,
                          gc,
                          FALSE,
                          rect.x + position, rect.y + position,
                          size, size
                          );
      break;
    case MARK_SPOT:
      size = go.stone_size / 1.5;
      if( !((size - go.grid_stroke)%2) ) size++;
      position = (go.cell_size - size ) / 2 -1;

      gdk_draw_arc(go.drawing_area_pixmap_buffer,
                   gc,
                   TRUE,//filled
                   rect.x + position,
                   rect.y + position,
                   size, size,
                   0, 23040);//23040 == 360*64
      break;
    case NO_MARK:
    default:
      break;
  }
  gtk_widget_draw (go.drawing_area, &rect);
}//paint_mark()

void paint_stone(int col, int row, GoItem item){
  GdkRectangle rect;

  rect.x = (col -1) * go.cell_size + go.margin + go.stone_space;
  rect.y = (row -1) * go.cell_size + go.margin + go.stone_space;
  rect.width = rect.height = go.stone_size;

  TRACE("paint");
 
  switch (item){
  case BLACK_STONE:
    gdk_draw_pixbuf(go.drawing_area_pixmap_buffer,
                    go.drawing_area->style->white_gc,
                    go.pixbuf_black_stone,
                    0, 0, rect.x, rect.y,
                    -1, -1,
                    GDK_RGB_DITHER_NONE, 0, 0);
    break;
  case WHITE_STONE:
    gdk_draw_pixbuf(go.drawing_area_pixmap_buffer,
                    go.drawing_area->style->white_gc,
                    go.pixbuf_white_stone,
                    0, 0, rect.x, rect.y,
                    -1, -1,
                    GDK_RGB_DITHER_NONE, 0, 0);
    break;
  case EMPTY:
    break;
  }//switch item
  gtk_widget_draw (go.drawing_area, &rect);
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

gboolean
on_drawing_area_configure_event (GtkWidget         * widget,
                                 GdkEventConfigure * event,
                                 gpointer            user_data){
  //when window is created or resized
  if(!go.drawing_area_pixmap_buffer){
    go.drawing_area_pixmap_buffer = gdk_pixmap_new(widget->window,
                                                   BOARD_SIZE,
                                                   BOARD_SIZE,
                                                   -1);
  }
  return TRUE;
}


gboolean
on_drawing_area_expose_event (GtkWidget       *widget,
                              GdkEventExpose  *event,
                              gpointer         user_data){
  //refresh the part outdated by the event
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  go.drawing_area_pixmap_buffer,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);
  return FALSE;
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

void status_update(char * message){
  g_strdelimit(message, "\n\t\r", ' ');//make message a "flat" string
  gtk_statusbar_pop (GTK_STATUSBAR(go.status), 0);
  gtk_statusbar_push(GTK_STATUSBAR(go.status), 0, message);
}
#include <stdarg.h>
void status_update_fmt(const char * format, ...){
  gchar * message;
  va_list ap;
  va_start (ap, format);
  va_start (ap, format);
  vasprintf (&message, format, ap);
  va_end (ap);

  status_update(message);
}

void status_update_current(){
  Hitem * hitem;
  char item;
  gchar * message;

  hitem = go.history->data;
  if(!hitem){
    return;
  }

  switch(hitem->item){
    case BLACK_STONE: item = 'B'; break;
    case WHITE_STONE: item = 'W'; break;
    default: item = '-';
  }

  if(hitem->action == PASS){
    message = g_strdup_printf(" %c PASS", item);
  }
  else /*if(hitem->action == PLAY)*/{
    message = g_strdup_printf(" %c (%d,%d) ", item, hitem->row, hitem->col);
  }

  if(hitem->comment){
    status_update_fmt("%s - %s", message, hitem->comment);
  }
  else{
    status_update(message);
  }
}

void update_capture_label(){
  free(go.capture_string);
  go.capture_string = (char *) malloc (20 * sizeof(char));
  /* TRANSLATORS: B = Black stones, W = White stones */
  sprintf(go.capture_string, _("B:%d  W:%d "), go.black_captures, go.white_captures);
  gtk_label_set_text (GTK_LABEL (go.capture_label), go.capture_string);
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

gboolean
on_drawing_area_button_release_event(GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     gpointer         user_data){
  TRACE("release");
  go.col = (event->x - go.margin) / go.cell_size + 1;
  go.row = (event->y - go.margin) / go.cell_size + 1;
  play_at(go.col, go.row);

  return FALSE;
}

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

void save_game(){
  FILE * f;
  char * filename;
  char * filename_sgf;

  //--Open file
  filename = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (go.file_selector)));
  if(g_str_has_suffix(filename, ".sgf") == FALSE){
    filename_sgf = g_strconcat (filename, ".sgf", NULL);
    g_free(filename);
  }
  else{
    filename_sgf = filename;
  }
  f = fopen(filename_sgf, "w");
  if(!f){
    gtk_widget_hide (go.file_selector);
    gpe_error_box_fmt (_("Cannot save the game into %s."), filename_sgf);
    return;
  }
  g_free(filename_sgf);


  //--Save history
  fprintf(f, "(;GM[1]FF[3]\n");
  fprintf(f, "RU[Japanese]SZ[%d]\n", go.game_size);
  fprintf(f, "PW[%s]\n", "white");
  fprintf(f, "PB[%s]\n", "black");

  _save_tree_to_sgf_from (go.history_root, f);

  fprintf(f, "\n)\n");

  fclose(f);
}

void load_game(){
  char * filename;
  char * filename_sgf;

  filename = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (go.file_selector)));
  if(g_str_has_suffix(filename, ".sgf") == FALSE){
    filename_sgf = g_strconcat (filename, ".sgf", NULL);
    g_free(filename);
  }
  else{
    filename_sgf = filename;
  }

  //gtk_widget_hide (go.file_selector);
  //loading... dialog

  load_sgf_file(filename_sgf);

  g_free(filename_sgf);
}

void on_file_selection_ok(GtkWidget *widget, gpointer file_selector){
  if(go.save_game) save_game();
  else load_game();
}

void on_button_first_pressed(GtkWidget *widget, gpointer unused){
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

void on_button_last_pressed(GtkWidget *widget, gpointer unused){
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

void on_button_prev_pressed (void){
  undo_turn();
}
void on_button_next_pressed (void){
  redo_turn();
}

void on_button_pass_clicked (void){
  if(go.lock_variation_choice) return;
  pass_turn();
}

void on_button_newgame_cancel_clicked (void){
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

void on_button_newgame_ok_clicked (void){
  init_new_game(go.selected_game_size);
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

void on_button_game_new_clicked(GtkButton *button, gpointer unused){
  popup_menu_close(go.game_popup_button);
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_GAME_SETTINGS);
}
void on_button_game_save_clicked(GtkButton *button, gpointer unused){
  popup_menu_close(go.game_popup_button);
  go.save_game = TRUE;
  gtk_window_set_title( GTK_WINDOW(go.file_selector), _("Save game as..."));
  gtk_widget_show (go.file_selector);
}
void on_button_game_load_clicked(GtkButton *button, gpointer unused){
  popup_menu_close(go.game_popup_button);//FIXME: connect function on "clicked"
  go.save_game = FALSE;
  gtk_window_set_title( GTK_WINDOW(go.file_selector), _("Load game..."));
  gtk_widget_show (go.file_selector);
}

void on_radiobutton_size_clicked (GtkButton *button, gpointer size){
  if(size == NULL){
    go.selected_game_size =
      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(go.game_size_spiner));
  }
  else{
    go.selected_game_size = GPOINTER_TO_INT(size);
  }
}

void on_spinbutton_value_changed(GtkSpinButton *spinbutton,
                                 gpointer radiobutton){
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton), TRUE);
  go.selected_game_size = gtk_spin_button_get_value_as_int (spinbutton);
}

GtkWidget * build_new_game_dialog(){
  GtkWidget * top_level_container;

  //containers
  GtkWidget * title;
  GtkWidget * game_size;
  GtkWidget * buttons;

  //
  GtkWidget * label;
  GtkWidget * vbox;
  GtkWidget * radiobutton1;
  GtkWidget * radiobutton3;
  GtkWidget * radiobutton4;
  GSList    * group_game_size_group = NULL;
  GtkWidget * hbox;
  GtkWidget * radiobutton2;
  GtkObject * spinbutton1_adj;
  GtkWidget * spinbutton1;

  GtkWidget * frame;
  GtkWidget * image;
  GdkPixbuf * pixbuf;

  //--title
  label = gtk_label_new (NULL);
  /* TRANSLATORS: keep the <big><b> tags */
  gtk_label_set_markup (GTK_LABEL (label), _("<big><b>New Game</b></big>"));

  //image
  pixbuf = gpe_find_icon ("this_app_icon");
  image = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(G_OBJECT(pixbuf));
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (frame), image);

  //packing
  hbox = gtk_hbox_new (FALSE, 20);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  title = hbox;


  //--size choice
  vbox = gtk_vbox_new (FALSE, 5);

  label = gtk_label_new (NULL);
  /* TRANSLATORS: keep the <b> tags */
  gtk_label_set_markup (GTK_LABEL (label), _("<b>Game Size</b>"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  radiobutton1 = gtk_radio_button_new_with_label (group_game_size_group, "9");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
  gtk_box_pack_start (GTK_BOX (vbox), radiobutton1, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton1), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), GINT_TO_POINTER(9));

  radiobutton3 = gtk_radio_button_new_with_label (group_game_size_group, "13");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
  gtk_box_pack_start (GTK_BOX (vbox), radiobutton3, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton3), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), GINT_TO_POINTER(13));

  radiobutton4 = gtk_radio_button_new_with_label (group_game_size_group, "19");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton4));
  gtk_box_pack_start (GTK_BOX (vbox), radiobutton4, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton4), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), GINT_TO_POINTER(19));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  radiobutton2 = gtk_radio_button_new_with_label (group_game_size_group, "");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
  gtk_box_pack_start (GTK_BOX (hbox), radiobutton2, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton2), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), NULL);

  spinbutton1_adj = gtk_adjustment_new (21, 4, 26, 1, 10, 10);
  spinbutton1 = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton1_adj), 1, 0);
  g_signal_connect (G_OBJECT (spinbutton1), "value-changed",
                    G_CALLBACK (on_spinbutton_value_changed), radiobutton2);

  gtk_box_pack_start (GTK_BOX (hbox), spinbutton1, FALSE, FALSE, 0);

  //default
  go.selected_game_size = 19;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton4), TRUE);

  go.game_size_spiner = spinbutton1;

  game_size = vbox;

  //[OK] button
  {
    GtkWidget * button;
    GtkWidget * hbox;

    hbox = gtk_hbox_new(TRUE, 0);
    
    button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_newgame_cancel_clicked), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    button = gtk_button_new_from_stock (GTK_STOCK_OK);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_newgame_ok_clicked), NULL);
    
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    buttons = hbox;
  }
    
  //--Packing
  top_level_container = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (top_level_container), 5);

  gtk_box_pack_start (GTK_BOX (top_level_container), title, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), game_size, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), gtk_hseparator_new (), FALSE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (top_level_container), buttons, FALSE, FALSE, 4);

  return top_level_container;
}

void on_button_edit_comment_clicked(){
  Hitem * hitem;

  if(go.lock_variation_choice) return;

  hitem = go.history->data;
  if(hitem && hitem->comment){
    gtk_text_buffer_set_text (go.comment_buffer, hitem->comment, -1);
  }
  else{
    gtk_text_buffer_set_text (go.comment_buffer, "", -1);
  }
  go.comment_edited = FALSE;
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_COMMENT_EDITOR);
}

void on_textbuffer_changed (GtkTextBuffer * textbuffer, gpointer unused){
  go.comment_edited = TRUE;
}

void on_button_comment_cancel_clicked (void){
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

void on_button_comment_ok_clicked (void){
  if(go.comment_edited){
    gchar * s;
    GtkTextIter it_start;
    GtkTextIter it_end;
    
    gtk_text_buffer_get_bounds   (go.comment_buffer, &it_start, &it_end);
    s = gtk_text_buffer_get_text (go.comment_buffer, &it_start, &it_end, FALSE);
    TRACE("Got -->%s<--", s);

    g_strstrip(s);

    if( strlen(s) > 0 && go.history->data ){
      Hitem * hitem;
      hitem = go.history->data;
      if(hitem->comment) free(hitem->comment);
      hitem->comment = s;
    }
    status_update_current();
  }
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

GtkWidget * build_comment_editor(){
  GtkWidget * top_level_container;

  //containers
  GtkWidget * title;
  GtkWidget * buttons;

  //
  GtkWidget * label;
  GtkWidget * hbox;

  GtkWidget * frame;
  GtkWidget * image;
  GdkPixbuf * pixbuf;

  GtkWidget * comment_text_view;
  GtkWidget * scrolled_window;

  //--title
  label = gtk_label_new (NULL);
  /* TRANSLATORS: keep the <big><b> tags */
  gtk_label_set_markup (GTK_LABEL (label), _("<big><b>Comment editor</b></big>"));

  //image
  pixbuf = gpe_find_icon ("this_app_icon");
  image = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(G_OBJECT(pixbuf));
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (frame), image);

  //packing
  hbox = gtk_hbox_new (FALSE, 20);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  title = hbox;

  //--Text editor
  comment_text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(comment_text_view), GTK_WRAP_CHAR);
  go.comment_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (comment_text_view));
  g_signal_connect (G_OBJECT (go.comment_buffer), "changed",
                    G_CALLBACK (on_textbuffer_changed), NULL);

  //scrolled window
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled_window), comment_text_view);

  //--[CANCEL][OK] buttons
  {
    GtkWidget * button;
    GtkWidget * hbox;

    hbox = gtk_hbox_new(TRUE, 0);
    
    button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_comment_cancel_clicked), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    button = gtk_button_new_from_stock (GTK_STOCK_OK);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_comment_ok_clicked), NULL);
    
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    buttons = hbox;
  }

  //--Packing
  top_level_container = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (top_level_container), 5);

  gtk_box_pack_start (GTK_BOX (top_level_container), title, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), buttons, FALSE, FALSE, 4);

  return top_level_container;
}

GtkWidget * _game_popup_menu_new (GtkWidget *parent_button){
  GtkWidget *vbox;//to return

  GtkWidget * button_new;
  GtkWidget * button_load;
  GtkWidget * button_save;

  GtkStyle * style = go.window->style;

  button_new  = gpe_picture_button_aligned (style, _("New...") , "!gtk-new" , GPE_POS_LEFT);
  button_save = gpe_picture_button_aligned (style, _("Save..."), "!gtk-save", GPE_POS_LEFT);
  button_load = gpe_picture_button_aligned (style, _("Load..."), "!gtk-open", GPE_POS_LEFT);

#define _BUTTON_SETUP(action) \
              gtk_button_set_relief (GTK_BUTTON (button_ ##action), GTK_RELIEF_NONE);\
              g_signal_connect (G_OBJECT (button_ ##action), "clicked",\
              G_CALLBACK (on_button_game_ ##action ##_clicked), NULL);

  _BUTTON_SETUP(new);
  _BUTTON_SETUP(save);
  _BUTTON_SETUP(load);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_new,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_save, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_load, FALSE, FALSE, 0);

  return vbox;
}


void gui_init(){
  GtkWidget * widget;

  GtkWidget * window;
  GtkWidget * vbox;

  GtkWidget * new_game_dialog;
  GtkWidget * comment_editor;

  GtkWidget * toolbar;
  GtkWidget * image;

  GtkWidget * capture_label;

  GtkWidget * drawing_area;


  //--toplevel window
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  go.window = window;

#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 280);
#endif
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (app_quit), NULL);

  gpe_set_window_icon (window, "this_app_icon");


  //--toolbar
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar),
                              GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style      (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  //[game] popup menu
  widget = popup_menu_button_new_from_stock (GTK_STOCK_NEW,
                                             _game_popup_menu_new, NULL);
  gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar), widget,
                            _("Game menu"), _("Game menu"));
  go.game_popup_button = widget;

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  //[FIRST] button
  image = gtk_image_new_from_stock (GTK_STOCK_GOTO_FIRST, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("First"),
                           _("Go to the beginning of the game"),
                           _("Go to the beginning of the game"),
                           image, GTK_SIGNAL_FUNC (on_button_first_pressed), NULL);

  //[PREV] button
  image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Prev"),
                           _("Go to the previous step"),
                           _("Go to the previous step"),
                           image, GTK_SIGNAL_FUNC (on_button_prev_pressed), NULL);

  //[NEXT] button
  image = gtk_image_new_from_stock (GTK_STOCK_REDO, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Next"),
                           _("Go to the next step"),
                           _("Go to the next step"),
                           image, GTK_SIGNAL_FUNC (on_button_next_pressed), NULL);

  //[LAST] button
  image = gtk_image_new_from_stock (GTK_STOCK_GOTO_LAST, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Last"),
                           _("Go to the end of the game"),
                           _("Go to the end of the game"),
                           image, GTK_SIGNAL_FUNC (on_button_last_pressed), NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));


  //Capture label, 
  //let put it in the toolbar. Will find a better place later
  capture_label = gtk_label_new("");
  
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), capture_label, NULL, NULL);
  go.capture_label = capture_label;

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  // [PASS] button
  image = gtk_image_new_from_stock (GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Pass"), _("Pass your turn"), _("Pass your turn"),
                           image, GTK_SIGNAL_FUNC (on_button_pass_clicked), NULL);

  //--file selector
  widget = gtk_file_selection_new (NULL);

  g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
                    "clicked",
                    G_CALLBACK (on_file_selection_ok),
                    (gpointer) widget);

  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_hide), 
                            (gpointer) widget); 

  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (widget)->cancel_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_hide),
                            (gpointer) widget);

  //gtk_file_selection_complete(GTK_FILE_SELECTION(widget), "*.sgf");
  go.file_selector = widget;

  //--drawing area (Go Board)
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawing_area, BOARD_SIZE, BOARD_SIZE);
  gtk_widget_set_events (drawing_area, 0
                         | GDK_EXPOSURE_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         );
  go.drawing_area = drawing_area;
  go.drawing_area_pixmap_buffer = NULL;

  g_signal_connect (G_OBJECT (drawing_area), "configure_event",
                    G_CALLBACK (on_drawing_area_configure_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
                    G_CALLBACK (on_drawing_area_expose_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "button_release_event",
                    G_CALLBACK (on_drawing_area_button_release_event), NULL);


  //--Status bar
  widget = gtk_statusbar_new();
  gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(widget), FALSE);
  go.status = widget;

  {//Status expansion
    GtkWidget * event_box;
    event_box = gtk_event_box_new();
    gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect (G_OBJECT (event_box), "button_press_event",
                      G_CALLBACK (on_button_edit_comment_clicked), NULL);

    widget = gtk_label_new("... ");
    gtk_container_add (GTK_CONTAINER (event_box), widget);
    gtk_box_pack_start (GTK_BOX (go.status), event_box, FALSE, FALSE, 0);
  }

  //--Comment editor
  comment_editor = build_comment_editor();

  //--New game settings page
  new_game_dialog = build_new_game_dialog();


  //--packing

  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar,      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE,  TRUE,  0);
  gtk_box_pack_start (GTK_BOX (vbox), go.status,    FALSE, FALSE, 0);

  widget = gtk_notebook_new();
  go.notebook = widget;
  gtk_notebook_set_show_border(GTK_NOTEBOOK(widget), FALSE);
  gtk_notebook_set_show_tabs  (GTK_NOTEBOOK(widget), FALSE);

  gtk_notebook_insert_page(GTK_NOTEBOOK(widget), vbox,  NULL, PAGE_BOARD);
  gtk_notebook_insert_page(GTK_NOTEBOOK(widget), new_game_dialog, NULL, PAGE_GAME_SETTINGS);
  gtk_notebook_insert_page(GTK_NOTEBOOK(widget), comment_editor, NULL, PAGE_COMMENT_EDITOR);

  gtk_container_add (GTK_CONTAINER (window), widget);

  gtk_widget_show_all (window);
  gtk_notebook_set_page(GTK_NOTEBOOK(widget), PAGE_BOARD);

}//gui_init()

