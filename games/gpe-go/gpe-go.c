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

//--GPE libs
#include "gpe/init.h"
#include "gpe/pixmaps.h"
#include "gpe/errorbox.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext(_x)

//#define TURN_LABEL
#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
//g_printerr("DEBUG *** %s, line %u: ", __FILE__, __LINE__);
#else
#define TRACE(message...) 
#endif

static struct gpe_icon my_icons[] = {
  { "this_app_icon", PREFIX "/share/pixmaps/gpe-go.png" },

  //{ "new",        "new"   },
  //{ "prefs",      "preferences"  },

  //{ "remove",     "gpe-go/stock_clear_24"},

  { NULL, NULL }
};

#define BOARD_SIZE 240 //pixels

//colors
GdkGC       * gc;
GdkColormap * colormap;
GdkColor red = {0, 65535, 0, 0};

typedef struct _go {
  //--ui
  GtkWidget * window;
  GdkPixmap * drawing_area_pixmap_buffer;
  GtkWidget * drawing_area;

  GdkPixbuf * pixbuf_black_stone;
  GdkPixbuf * pixbuf_white_stone;
  GdkPixmap * pixmap_empty_board;

  GtkWidget * capture_label;
  char      * capture_string;

  GtkWidget * file_selector;

#ifdef TURN_LABEL
  GtkWidget * turn_label;
  char      * turn_string;
#endif

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
  int last_turn;

  //last played
  int last_col;
  int last_row;

  //current position
  int col;
  int row;

  GNode * history_root; //root node
  GNode * history;      //current pointer

  gboolean lock_variation_choice;

} Go;

Go go;

typedef enum {
  EMPTY = 0,
  BLACK_STONE,
  WHITE_STONE,
} GoItem;

typedef enum {
  NO_MARK,
  MARK_SQUARE,
  MARK_SPOT,
  //MARK_TRIANGLE
} GoMark;

typedef enum {
  PASS,
  PLAY,
  CAPTURE,
} GoAction;

typedef struct _hist_item{
  GoAction action;
  GoItem   item;
  int posx;
  int posy;
  //GList * captured; FIXME: to use
} Hitem;

Hitem * new_hitem(){
  Hitem * hitem;
  hitem = (Hitem *) malloc (sizeof(Hitem));
  return hitem;
}

void free_hitem(Hitem * hitem){
  if(hitem) free(hitem);
}

gboolean is_same_hitem(Hitem * a, Hitem * b){
  if(1
     && a->posx   == b->posx
     && a->posy   == b->posy
     && a->item   == b->item
     && a->action == b->action) return TRUE;
  return FALSE;
}

void append_hitem(GoAction action, GoItem item, int gox, int goy){
  Hitem * hitem;
  Hitem * citem; //item to compare to
  GNode * node;
  gboolean found;

  hitem = new_hitem();

  hitem->action = action;
  hitem->item   = item;
  hitem->posx   = gox;
  hitem->posy   = goy;

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

  go.last_turn = g_node_depth(go.history);
}

void app_init(int argc, char ** argv){
  int i,j;
  int free_space;

  //--grid size
  if(argc > 1){//first arg is board size if numeric
    int size;
    if(sscanf(argv[1], "%d", &size) == 1) go.game_size = size;
  }
  else go.game_size = 9;//default size

  go.board_size = BOARD_SIZE;//240
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

  //--init grid
  //one unused row/col to have index from *1* to grid_size.
  go.grid = (char **) malloc ((go.game_size +1) * sizeof(char *));
  for (i=0; i<=go.game_size; i++){
    go.grid[i] = (char *) malloc ((go.game_size +1) * sizeof(char));
  }
  for(i=1; i<= go.game_size; i++) for(j=1; j<=go.game_size; j++) go.grid[i][j]=0;

  //stamps grid
  go.stamps = (char **) malloc ((go.game_size +1) * sizeof(char *));
  for (i=0; i<=go.game_size; i++){
    go.stamps[i] = (char *) malloc ((go.game_size +1) * sizeof(char));
  }
  for(i=1; i<= go.game_size; i++) for(j=1; j<=go.game_size; j++) go.stamps[i][j]=0;

  //--init game
  go.history_root = g_node_new (NULL);
  go.history = go.history_root;

  go.white_captures = 0;
  go.black_captures = 0;

  go.turn = 0;
  go.last_turn = 0;

  go.last_col = 0;
  go.last_row = 0;

  go.lock_variation_choice = FALSE;

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
  g_print("%c %c: %d %d", action, item, hitem->posx, hitem->posy);
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

int main (int argc, char ** argv){

  if (gpe_application_init (&argc, &argv) == FALSE) exit (1);
  if (gpe_load_icons (my_icons) == FALSE) exit (1);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
  setlocale (LC_ALL, "");

  app_init (argc, argv);
  gui_init ();

  gtk_main ();

  return 0;
}//main()


#define PIXMAPS_SRC  PREFIX "/share/gpe/pixmaps/default/" PACKAGE "/"

void load_graphics(){
  GdkPixbuf * stone_image;
  GdkPixbuf * stone_scaled;

  int stone_size;

  stone_size = go.stone_size;

  //--black stone
  stone_image = gdk_pixbuf_new_from_file(PIXMAPS_SRC "black.png", NULL);
  if(stone_image != NULL){
    stone_scaled = gdk_pixbuf_scale_simple (stone_image,
                                            stone_size,
                                            stone_size,
                                            GDK_INTERP_BILINEAR);
    gdk_pixbuf_unref(stone_image);
    go.pixbuf_black_stone = stone_scaled;

  }
  else {
    //gdk_draw_arc(go.drawing_area_pixmap_buffer,
    //             widget->style->black_gc,
    //             TRUE,
    //             0, 0,
    //             stone_size, stone_size,
    //             0, 23040);//23040 == 360*64
  }

  //--white stone
  stone_image = gdk_pixbuf_new_from_file(PIXMAPS_SRC "white.png", NULL);
  if(stone_image != NULL){
    stone_scaled = gdk_pixbuf_scale_simple (stone_image,
                                            stone_size,
                                            stone_size,
                                            GDK_INTERP_BILINEAR);
    gdk_pixbuf_unref(stone_image);
    go.pixbuf_white_stone = stone_scaled;
  }
  else {
    //gdk_draw_arc(go.drawing_area_pixmap_buffer,
    //             widget->style->white_gc,
    //             TRUE,
    //             rect.x, rect.y,
    //             rect.width, rect.height,
    //             0, 23040);//23040 == 360*64
    //gdk_draw_arc(go.drawing_area_pixmap_buffer, //black border
    //             widget->style->black_gc,
    //             FALSE,
    //             rect.x, rect.y,
    //             rect.width, rect.height,
    //               0, 23040);//23040 == 360*64
  }

  //--colors
  gc = gdk_gc_new(go.drawing_area->window);
  colormap = gdk_colormap_get_system();
  gdk_colormap_alloc_color(colormap, &red,  FALSE,TRUE);

}


void paint_board(GtkWidget * widget){
  int i;
  int cell_size;
  int border_trans; //grid's translation from the border of the board

  GdkPixbuf * bg_image;
  GdkPixbuf * bg_scaled;


  go.pixmap_empty_board = gdk_pixmap_new (widget->window,
                                          BOARD_SIZE,
                                          BOARD_SIZE,
                                          -1);

  //--background
  bg_image = gdk_pixbuf_new_from_file(PIXMAPS_SRC "board", NULL);
  if(bg_image != NULL){
    bg_scaled = gdk_pixbuf_scale_simple (bg_image,
                                         BOARD_SIZE, BOARD_SIZE,
                                         GDK_INTERP_BILINEAR);
    gdk_pixbuf_unref(bg_image);

    gdk_draw_pixbuf(go.pixmap_empty_board,
                    widget->style->white_gc,
                    bg_scaled,
                    0, 0, 0, 0,
                    -1, -1,
                    GDK_RGB_DITHER_NONE, //GdkRgbDither dither,
                    0, //gint x_dither,
                    0); //gint y_dither);

    gdk_pixbuf_unref(bg_scaled);
  }
  else {
    gdk_draw_rectangle (go.pixmap_empty_board,
                        widget->style->white_gc,//FIXME: use better color
                        TRUE,
                        0, 0,
                        BOARD_SIZE,
                        BOARD_SIZE);
  }

  //--grid
  cell_size = go.cell_size;
  border_trans = go.margin + go.cell_size / 2;
  
  for(i=0; i< go.game_size; i++){
    //vertical lines
    gdk_draw_line(go.pixmap_empty_board,
                  widget->style->black_gc,
                  border_trans + cell_size * i, border_trans,
                  border_trans + cell_size * i, BOARD_SIZE - border_trans);
    //horizontal lines
    gdk_draw_line(go.pixmap_empty_board,
                  widget->style->black_gc,
                  border_trans, border_trans + cell_size * i,
                  BOARD_SIZE - border_trans, border_trans + cell_size * i);
  }
  //--advantage points

#define ADVANTAGE_POINT_SIZE 3
#define paint_point(x,y) \
      gdk_draw_rectangle(go.pixmap_empty_board, \
               widget->style->black_gc, TRUE, \
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

  
  gdk_draw_drawable (go.drawing_area_pixmap_buffer,
                     widget->style->black_gc,
                     go.pixmap_empty_board,
                     0, 0, 0, 0, BOARD_SIZE, BOARD_SIZE);
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

void put_stone(GoItem color, int gox, int goy){
  if(go.grid[gox][goy] != EMPTY) return;
  go.grid[gox][goy] = color;
  paint_stone(gox, goy, color);
}

void remove_stone(int gox, int goy){
  if(go.grid[gox][goy] == EMPTY) return;
  go.grid[gox][goy] = EMPTY;
  unpaint_stone(gox, goy);
}


//------- drawing area callbacks
gboolean
on_drawing_area_configure_event (GtkWidget         * widget,
                                 GdkEventConfigure * event,
                                 gpointer            user_data){
  TRACE("configure event");
  //when window is created or resized
  //if (pixmap)  gdk_pixmap_unref(pixmap);
  if(!go.drawing_area_pixmap_buffer){
    go.drawing_area_pixmap_buffer = gdk_pixmap_new(widget->window,
                                                   BOARD_SIZE,
                                                   BOARD_SIZE,
                                                   -1);
    paint_board(widget);
  }
  return TRUE;
}


gboolean
on_drawing_area_expose_event (GtkWidget       *widget,
                              GdkEventExpose  *event,
                              gpointer         user_data){
  //TRACE("expose event (%d,%d)", event->area.x, event->area.y);
  //refresh the part outdated by the event
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  go.drawing_area_pixmap_buffer,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);
  return FALSE;
}


gboolean
on_drawing_area_motion_notify_event(GtkWidget       *widget,
                                    GdkEventMotion  *event,
                                    gpointer         user_data){
  int x, y;

  GdkModifierType state;

  if (event->is_hint){
    gdk_window_get_pointer (event->window, &x, &y, &state);
  }
  else{
    x = event->x;
    y = event->y;
    state = event->state;
  }

  go.col = (x - go.margin) / go.cell_size + 1;
  go.row = (y - go.margin) / go.cell_size + 1;
  //TRACE("[%d,%d] (%d,%d) %d", go.col, go.row, x, y, state);


  if (state & GDK_BUTTON1_MASK){
  }

  return FALSE;
}


gboolean
on_drawing_area_button_press_event(GtkWidget       *widget,
                                   GdkEventButton  *event,
                                   gpointer         user_data){
  //TRACE("press");
  return FALSE;
}

gboolean is_stamped (int gox, int goy){return go.stamps[gox][goy];}
void stamp(int gox, int goy){
  if(!go.stamps[gox][goy]) go.stamps[gox][goy] = 1;
}
void unstamp(int gox, int goy){
  if(go.stamps[gox][goy]) go.stamps[gox][goy] = 0;
}
void clear_stamps(){
  int i,j;
  for(j=1; j<= go.game_size; j++){
    for(i=1; i<=go.game_size; i++){
      if(go.stamps[i][j]) go.stamps[i][j] = 0;
    }
  }
}

void stamp_group_of(int gox, int goy){
  //**/char c;
  int x,y;

  int my_color;
  my_color = go.grid[gox][goy];

  stamp(gox,goy);

  //TRACE("Examining %d %d (%d)",gox,goy, my_color);
  //**/_print_grid();
  //**/scanf("%c",&c);scanf("%c",&c);

  x = gox;
  y = goy - 1;
  if(goy>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //TRACE("Now examining %d %d (%d)",x,y, go.grid[x][y]);
    stamp_group_of(x,y);
  }
  x = gox + 1;
  y = goy;
  if(gox<go.game_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //TRACE("Now examining %d %d (%d)",x,y, go.grid[x][y]);
    stamp_group_of(x,y);
  }
  x = gox;
  y = goy + 1;
  if(goy<go.game_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //TRACE("Now examining %d %d (%d)",x,y, go.grid[x][y]);
    stamp_group_of(x,y);
  }
  x = gox - 1;
  y = goy;
  if(gox>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //TRACE("Now examining %d %d (%d)",x,y, go.grid[x][y]);
    stamp_group_of(x,y);
  }
}

int size_group_of(int gox, int goy){
  //**/char c;
  int count = 1;
  int my_color;
  int x,y;
  my_color = go.grid[gox][goy];

  stamp(gox,goy);

  //TRACE("I am          %d %d (%d)",gox,goy, my_color);
  //**/_print_grid();
  //**/scanf("%c",&c);scanf("%c",&c);

  x = gox;
  y = goy - 1;
  if(goy>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    count += size_group_of(x,y);
  }
  x = gox + 1;
  y = goy;
  if(gox<go.game_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    count += size_group_of(x,y);
  }
  x = gox;
  y = goy + 1;
  if(goy<go.game_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    count += size_group_of(x,y);
  }
  x = gox - 1;
  y = goy;
  if(gox>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    count += size_group_of(x,y);
  }
  return count;
}

gboolean is_alive_group_of(int gox, int goy){
  //**/char c;
  int my_color;
  int x,y;
  my_color = go.grid[gox][goy];

  stamp(gox,goy);

  //TRACE("I am          %d %d (%d)",gox,goy, my_color);
  //**/_print_grid();
  //**/scanf("%c",&c);scanf("%c",&c);

  x = gox;     //NORTH
  y = goy - 1;
  if(goy>1 && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = gox + 1; //EAST
  y = goy;
  if(gox<go.game_size && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = gox;     //SOUTH
  y = goy + 1;
  if(goy<go.game_size && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = gox - 1; //WEST
  y = goy;
  if(gox>1 && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }

  return FALSE;
}

int kill_group_of(int gox, int goy){//returns number of stones killed
  int count;
  int i,j;

  count = 0;
  clear_stamps();
  stamp_group_of(gox, goy);

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

void update_capture_label(){
  free(go.capture_string);
  go.capture_string = (char *) malloc (20 * sizeof(char));
  sprintf(go.capture_string, _("B:%d  W:%d "), go.black_captures, go.white_captures);
  //TRACE("Capture points: %s", go.capture_string);
  gtk_label_set_text (GTK_LABEL (go.capture_label), go.capture_string);
}

#ifdef TURN_LABEL
void update_turn_label(){
  free(go.turn_string);
  go.turn_string = (char *) malloc (20 * sizeof(char));
  sprintf(go.turn_string, "%d/%d", go.turn, go.last_turn);
  gtk_label_set_text (GTK_LABEL (go.turn_label), go.turn_string);
}
#endif

void redo_turn();
void play_at(int gox, int goy){
  //TRACE("Play at %d %d", gox, goy);
  if( gox < 1 || go.game_size < gox ||
      goy < 1 || go.game_size < goy ){
    //TRACE("--> out of bounds");
    return;
  }
  if(go.lock_variation_choice){//variation choice
    if(!is_stamped(gox, goy)) return;
    redo_turn();
    return;
  }
  if(go.grid[gox][goy] != 0){
    //TRACE("--> not empty");
    return;
  }

  {
    GoItem color;
    color = (go.turn%2)?WHITE_STONE:BLACK_STONE;

    put_stone(color, gox, goy);

    update_last_played_mark_to(gox, goy);

    append_hitem(PLAY, color, gox, goy);
  }

  //--check if groups to kill
  {
    int opp_color;
    int x, y;

    int * captured;

    if(go.grid[gox][goy] == WHITE_STONE){
      opp_color = BLACK_STONE;
      captured  = & go.white_captures;
    }
    else{
      opp_color = WHITE_STONE;
      captured  = & go.black_captures;
    }

    clear_stamps();
    x = gox;
    y = goy - 1;
    if(goy>1 && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = gox + 1;
    y = goy;
    if(gox<go.game_size && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = gox;
    y = goy + 1;
    if(goy<go.game_size && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = gox - 1;
    y = goy;
    if(gox>1 && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
  }
  update_capture_label();

  //--next turn
  go.turn++;
  go.last_turn++;
#ifdef TURN_LABEL
  update_turn_label();
#endif
}


void undo_turn(){
  Hitem * hitem;

  if(go.lock_variation_choice) return;
  if(!go.history || go.turn == 0) return;

  hitem = go.history->data;

  while(hitem->action == CAPTURE){
    put_stone(hitem->item, hitem->posx, hitem->posy);//FIXME: don't update screen each time!
    if(hitem->item == BLACK_STONE) go.white_captures--;
    else go.black_captures--;

    if(go.history->parent) go.history = go.history->parent;
    hitem = go.history->data;
  }
  update_capture_label();

  if(hitem->action == PLAY){
    remove_stone(hitem->posx, hitem->posy);
    go.turn--;
  }
  else if(hitem->action == PASS){
    go.turn--;
  }
#ifdef TURN_LABEL
  update_turn_label();
#endif

  if(go.history->parent && !G_NODE_IS_ROOT(go.history->parent)){
    go.history = go.history->parent;

    {
      GNode * hist;
      Hitem * item;
      hist = go.history;
      item = hist->data;
      while(item->action == CAPTURE){
        hist = hist->parent;
        item = hist->data;
      } 
      update_last_played_mark_to(item->posx, item->posy);
    }
  }
}

void redo_turn(){
  int children;
  Hitem * hitem = NULL;

  if(!go.history || G_NODE_IS_LEAF(go.history)){
    TRACE("================do nothing");
    return;
  }

  //check variations
  children = g_node_n_children(go.history);
  if(go.turn >= 1 && children >= 1){

    GNode * choosen_child;
    choosen_child = g_node_first_child (go.history);

    if(children > 1){//variations
      if(go.lock_variation_choice == FALSE){
        int i;
        GNode * child;
        TRACE("VARIATIONS: %d", children -1);
        
        clear_stamps();
        for(i=0; i<children; i++){
          Hitem * item;
          child = g_node_nth_child (go.history, i);
          item = child->data;
          
          stamp(item->posx, item->posy);
          paint_mark(item->posx, item->posy, MARK_SPOT, &red);
        }
        
        go.lock_variation_choice = TRUE;
        return;
      }
      else{
        int i;
        GNode * child;

        if(!is_stamped(go.col, go.row)) return;
        //select the child
        
        for(i=0; i<children; i++){
          Hitem * item;
          child = g_node_nth_child (go.history, i);
          item = child->data;
          if(item->posx == go.col && item->posy == go.row){
            choosen_child = child;
          }
          //unpaint SPOTS
          unpaint_stone(item->posx, item->posy);
        }
        clear_stamps();
        go.lock_variation_choice = FALSE;
      }
    }//if variations
    
    go.history = choosen_child;
  }

  hitem = go.history->data;

  //TRACE("================try");
  ///**/_print_hitem(hitem);
  //TRACE("================");

  if(hitem->action == PASS){
    TRACE("***> rePASS");
    update_last_played_mark_to(0, 0);
    go.turn++;
  }
  else if(hitem->action == PLAY){
    TRACE("***> rePLAY %d %d", hitem->posx, hitem->posy);
    put_stone(hitem->item, hitem->posx, hitem->posy);

    update_last_played_mark_to(hitem->posx, hitem->posy);

    go.turn++;

    {//revert capture if there are.
      GNode * node;
      if(g_node_n_children(go.history) >= 1){

        node = g_node_nth_child (go.history, 0);
        hitem = node->data;
      
        while(hitem && hitem->action == CAPTURE){
          TRACE("***> remove %d %d", hitem->posx, hitem->posy);
          remove_stone(hitem->posx, hitem->posy);//FIXME: don't update screen each time!
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
#ifdef TURN_LABEL
  update_turn_label();
#endif

}

gboolean
on_drawing_area_button_release_event(GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     gpointer         user_data){
  TRACE("release");

  play_at(go.col, go.row);

  return FALSE;
}

/*
  Property index of FF[1]-FF[4]
  http://www.red-bean.com/sgf/proplist_ff.html
*/

void _save_hitem_sgf(FILE * f, Hitem * hitem){
//  ;B[am]BL[1791];W[al]WL[1799]
  char item;

  if(!hitem) return;
  if(hitem->item == BLACK_STONE) item = 'B'; else item = 'W';
  if     (hitem->action == PASS) fprintf(f, ";%c[tt]", item);//FIXME: support "[]"
  else if(hitem->action == PLAY) fprintf(f, ";%c[%c%c]", item,
                                         hitem->posx + 'a' -1,
                                         hitem->posy + 'a' -1);
}

void _save_tree_to_sgf_from (GNode *node, FILE * f){
  gboolean has_siblings = FALSE;

  if(node->parent && g_node_n_children(node->parent) > 1){
    has_siblings = TRUE;
    fprintf(f, "\n  (");
  }

  if(node->data) _save_hitem_sgf(f, node->data);

  node = node->children;
  while (node){
      _save_tree_to_sgf_from (node, f);
      node = node->next;
  }

  if(has_siblings) fprintf(f, ")\n");  
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
    //error
    gtk_widget_hide (go.file_selector);
    gpe_error_box_fmt (_("Cannot save the game into %s."), filename_sgf);
    return;
  }
  g_free(filename_sgf);


  //--Save history

  //headers
  fprintf(f, "(;GM[1]FF[3]\n");
  fprintf(f, "RU[Japanese]SZ[%d]H[%d]KM[%f]\n", go.game_size, 0, 0.5/*komi*/);
  fprintf(f, "PW[%s]\n", "white");
  fprintf(f, "PB[%s]\n", "black");
  fprintf(f, "GN[%s]\n", "the game of the year");
  fprintf(f, "DT[%s]\n", "yyyy-mm-dd");

  //history tree
  _save_tree_to_sgf_from (go.history_root, f);

  //footer
  fprintf(f, "\n)\n");

  fclose(f);
  gtk_widget_hide (go.file_selector);
}

void on_button_save_pressed (void){
  gtk_widget_show (go.file_selector);
}

void on_button_pref_pressed (void){
}
void on_button_prev_pressed (void){
  TRACE("prev");
  undo_turn();
}
void on_button_next_pressed (void){
  TRACE("next");
  redo_turn();
}

void on_button_pass_clicked (void){
  TRACE("pass");

  if(go.lock_variation_choice) return;

  append_hitem(PASS, EMPTY, 0, 0);
  go.turn++;
  go.last_turn++;

  update_last_played_mark_to(0, 0);
}

void gui_init(){
  GtkWidget * widget;

  GtkWidget * window;
  GtkWidget * vbox;

  GtkWidget * toolbar;
#ifdef PREFS
  GdkPixbuf * pixbuf;
#endif
  GtkWidget * image;

  GtkWidget * capture_label;
#ifdef TURN_LABEL
  GtkWidget * turn_label;
#endif

  GtkWidget   * drawing_area;


  //--toplevel window
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  go.window = window;

#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 280);
  //gtk_window_set_position     (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
#endif
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (app_quit), NULL);

  gpe_set_window_icon (window, "this_app_icon");


  //--toolbar
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar),
                              GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style      (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Undo"), _("Undo"), _("Undo"),
			   image, GTK_SIGNAL_FUNC (on_button_prev_pressed), NULL);

#ifdef TURN_LABEL
  //Turn label
  turn_label = gtk_label_new("0/0");
  
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), turn_label, NULL, NULL);
  go.turn_label = turn_label;
#endif

  image = gtk_image_new_from_stock (GTK_STOCK_REDO, GTK_ICON_SIZE_SMALL_TOOLBAR);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Redo"), _("Redo"), _("Redo"),
			   image, GTK_SIGNAL_FUNC (on_button_next_pressed), NULL);
 
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));


  //Capture label, 
  //let put it in the toolbar. Will find a better place later
  capture_label = gtk_label_new(_("B:0  W:0 "));
  
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), capture_label, NULL, NULL);
  go.capture_label = capture_label;

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

#ifdef PREFS
  //[PREFS] button
  pixbuf = gpe_find_icon ("prefs");
  image = gtk_image_new_from_pixbuf(pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   NULL, NULL, NULL,
			   image, GTK_SIGNAL_FUNC (on_button_pref_pressed), NULL);
  gdk_pixbuf_unref(pixbuf);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
#endif

  {// [PASS] button
    GtkWidget * button;
    /* TRANSLATORS: button's label, use a short word.
     * proposals for a meaningful icon are warmly welcome! */
    button = gtk_button_new_with_label (_("PASS"));
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_pass_clicked), NULL);
    //FIXME: add tooltip!
    gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), button, NULL, NULL);
  }

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  //[SAVE] button
  image = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Save"), _("Save"), _("Save"),
			   image, GTK_SIGNAL_FUNC (on_button_save_pressed), NULL);

  //file selector
  widget = gtk_file_selection_new (_("Save as..."));

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(widget)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (save_game), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(widget)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_hide),
		             (gpointer) widget);
  //gtk_file_selection_complete(GTK_FILE_SELECTION(widget), "*.sgf"); //FIXME...
  go.file_selector = widget;

  //--drawing area (Go Board)
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawing_area, BOARD_SIZE, BOARD_SIZE);
  gtk_widget_set_events (drawing_area, 0
                         | GDK_EXPOSURE_MASK
                         | GDK_POINTER_MOTION_MASK
                         | GDK_POINTER_MOTION_HINT_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         //| GDK_LEAVE_NOTIFY_MASK
                         );
  go.drawing_area = drawing_area;
  go.drawing_area_pixmap_buffer = NULL;

  gtk_signal_connect (GTK_OBJECT (drawing_area), "configure_event",
                      GTK_SIGNAL_FUNC (on_drawing_area_configure_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
                      GTK_SIGNAL_FUNC (on_drawing_area_expose_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "motion_notify_event",
                      GTK_SIGNAL_FUNC (on_drawing_area_motion_notify_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
                      GTK_SIGNAL_FUNC (on_drawing_area_button_press_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "button_release_event",
                      GTK_SIGNAL_FUNC (on_drawing_area_button_release_event),
                      NULL);


  //--packing
  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar,      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE,  TRUE,  0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_show_all (window);

  load_graphics();

}//gui_init()

