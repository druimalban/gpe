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


//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

static struct gpe_icon my_icons[] = {
  { "this_app_icon", PREFIX "/share/pixmaps/gpe-go.png" },

  { "new",        "new"   },
  { "prefs",      "preferences"  },
  { "left",       "left"  },
  { "right",      "right" },

  //{ "remove",     "gpe-go/stock_clear_24"},

  { NULL, NULL }
};

#define BOARD_SIZE 240 //pixels

typedef struct _go {
  //--ui
  GtkWidget * window;
  GdkPixmap * drawing_area_pixmap_buffer;
  GtkWidget * drawing_area;

  GdkPixbuf * pixbuf_black_stone;
  GdkPixbuf * pixbuf_white_stone;
  GdkPixmap * pixmap_empty_board;

  GtkWidget * capture_label;

  int grid_margin;//px
  int grid_space; //px

  //--current position
  int x;
  int y;

  //--game
  int grid_size; // 9x9 13x13 19x19

  char ** grid;   //current state of the board
  char ** stamps;

  int white_captures; //stones captured by white!
  int black_captures; 

  int turn;

  //GList * history;

} Go;

Go go;


typedef enum {
  EMPTY = 0,
  BLACK_STONE,
  WHITE_STONE,
} GoItem;



void app_init(int argc, char ** argv){
  int i,j;

  //--grid size
  if(argc > 1){//first arg is board size if numeric
    int size;
    if(sscanf(argv[1], "%d", &size) == 1) go.grid_size = size;
  }
  else go.grid_size = 9;//default size

  //--init grid
  //one unused row/col to have index from *1* to grid_size.
  go.grid = (char **) malloc ((go.grid_size +1) * sizeof(char *));
  for (i=0; i<=go.grid_size; i++){
    go.grid[i] = (char *) malloc ((go.grid_size +1) * sizeof(char));
  }
  for(i=1; i<= go.grid_size; i++) for(j=1; j<=go.grid_size; j++) go.grid[i][j]=0;

  //stamps grid
  go.stamps = (char **) malloc ((go.grid_size +1) * sizeof(char *));
  for (i=0; i<=go.grid_size; i++){
    go.stamps[i] = (char *) malloc ((go.grid_size +1) * sizeof(char));
  }
  for(i=1; i<= go.grid_size; i++) for(j=1; j<=go.grid_size; j++) go.stamps[i][j]=0;

  //--init game
  go.white_captures = 0;
  go.black_captures = 0;
  go.turn = 0;

  //--some ui init... FIXME: to move 
  go.grid_space  = BOARD_SIZE / (go.grid_size - 1 + 1.5/*= margin prop*/);
  go.grid_margin = (BOARD_SIZE - (go.grid_size - 1) * go.grid_space ) / 2;

}

void app_quit(){
  gtk_exit (0);
}

void _print_gird(){
  int i,j;
  g_printerr("     ");
  for(j=1; j<= go.grid_size; j++) g_printerr("%2d ", j);
  g_printerr("\n");
  for(j=1; j<= go.grid_size; j++){
    g_printerr("%2d : ", j);
    for(i=1; i<=go.grid_size; i++){
      if(go.grid[i][j] == 0) g_printerr(" %c ",'-');
      else{
        if(go.stamps[i][j]) g_printerr("*%d*",go.grid[i][j]);
        else g_printerr("%2d ",go.grid[i][j]);
      }
    }
    g_printerr("\n");
  }
}

void gui_init();

int main (int argc, char ** argv){

  if (gpe_application_init (&argc, &argv) == FALSE) exit (1);
  if (gpe_load_icons (my_icons) == FALSE) exit (1);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
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

  stone_size = go.grid_space - 2;

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


}


void paint_board(GtkWidget * widget){
  int i;
  int space;
  int margin;

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
  space  = go.grid_space ;
  margin = go.grid_margin;
  g_printerr("space: %d, margin: %d\n", space, margin);
  
  for(i=0; i< go.grid_size; i++){
    gdk_draw_line(go.pixmap_empty_board,
                  widget->style->black_gc,
                  space * i + margin, margin,
                  space * i + margin, BOARD_SIZE - margin);
    gdk_draw_line(go.pixmap_empty_board,
                  widget->style->black_gc,
                  margin, space * i + margin,
                  BOARD_SIZE - margin, space * i + margin);
    //**/g_printerr("%2d (%3d,%3d) (%3d,%3d)\n",
    //**/            i + 1,
    //**/            space * i, margin + space,
    //**/            space * i, BOARD_SIZE - margin - space);
  }
  //--advantage points

#define paint_point(w,x,y) \
      gdk_draw_rectangle(go.pixmap_empty_board, \
               widget->style->black_gc, TRUE, \
               margin + space * (x-1) - w/2, \
               margin + space * (y-1) - w/2, \
               w, w);

  if(go.grid_size == 9){
    paint_point(3, 3, 3);
    paint_point(3, 7, 3);
    paint_point(3, 5, 5);
    paint_point(3, 3, 7);
    paint_point(3, 7, 7);
  }
  else if(go.grid_size == 13){
    paint_point(3, 4, 4);
    paint_point(3, 4, 7);
    paint_point(3, 4, 10);
    paint_point(3, 7, 4);
    paint_point(3, 7, 7);
    paint_point(3, 7, 10);
    paint_point(3,10, 4);
    paint_point(3,10, 7);
    paint_point(3,10, 10);
  }
  else if(go.grid_size == 19){
    paint_point(3,  4,  4);
    paint_point(3,  4, 10);
    paint_point(3,  4, 16);
    paint_point(3, 10,  4);
    paint_point(3, 10, 10);
    paint_point(3, 10, 16);
    paint_point(3, 16,  4);
    paint_point(3, 16, 10);
    paint_point(3, 16, 16);
  }

  
  gdk_draw_drawable (go.drawing_area_pixmap_buffer,
                     widget->style->black_gc,
                     go.pixmap_empty_board,
                     0, 0, 0, 0, BOARD_SIZE, BOARD_SIZE);
}



void remove_stone(int gox, int goy){
  GdkRectangle rect;

  if(go.grid[gox][goy] == 0) return;

  rect.x = (gox -1) * go.grid_space + go.grid_margin - go.grid_space / 2;
  rect.y = (goy -1) * go.grid_space + go.grid_margin - go.grid_space / 2;
  rect.width = rect.height = go.grid_space -1;
  gdk_draw_drawable (go.drawing_area_pixmap_buffer,
                     go.drawing_area->style->black_gc,
                     go.pixmap_empty_board,
                     rect.x, rect.y, rect.x, rect.y,
                     rect.width, rect.height);
  gtk_widget_draw (go.drawing_area, &rect);

  go.grid[gox][goy] = 0;
}


void paint_stone(GtkWidget * widget, int x, int y, GoItem item){
  GdkRectangle rect;

  rect.x = x +1;
  rect.y = y +1;
  rect.width = rect.height = go.grid_space -2;

  /**/g_printerr("paint\n");
 
  switch (item){
  case BLACK_STONE:
      gdk_draw_pixbuf(go.drawing_area_pixmap_buffer,
                      widget->style->white_gc,
                      go.pixbuf_black_stone,
                      0, 0, rect.x, rect.y,
                      -1, -1,
                      GDK_RGB_DITHER_NONE, 0, 0);
    break;
  case WHITE_STONE:
      gdk_draw_pixbuf(go.drawing_area_pixmap_buffer,
                      widget->style->white_gc,
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


//------- drawing area callbacks
gboolean
on_drawing_area_configure_event (GtkWidget         * widget,
                                 GdkEventConfigure * event,
                                 gpointer            user_data){
  /**/g_printerr("configure event\n");
  //when window is created or resized
  //if (pixmap)  gdk_pixmap_unref(pixmap);
  if(!go.drawing_area_pixmap_buffer){
    go.drawing_area_pixmap_buffer = gdk_pixmap_new(widget->window,
                                                   BOARD_SIZE,
                                                   BOARD_SIZE,
                                                   -1);
    //    /**/gdk_draw_rectangle (go.drawing_area_pixmap_buffer,
    //                            widget->style->white_gc,
    //                            TRUE,
    //                            0, 0,
    //                            BOARD_SIZE,
    //                            BOARD_SIZE);
    paint_board(widget);
  }
  return TRUE;
}


gboolean
on_drawing_area_expose_event (GtkWidget       *widget,
                              GdkEventExpose  *event,
                              gpointer         user_data){
  //**/g_printerr("expose event (%d,%d)\n", event->area.x, event->area.y);
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

  go.x = (x - go.grid_margin - go.grid_space / 2 + 0.1) / go.grid_space +2;
  go.y = (y - go.grid_margin - go.grid_space / 2 + 0.1) / go.grid_space +2;
  //**/g_printerr("[%d,%d] (%d,%d) %d\n", go.x, go.y, x, y, state);


  if (state & GDK_BUTTON1_MASK){
  }

  return FALSE;
}


gboolean
on_drawing_area_button_press_event(GtkWidget       *widget,
                                   GdkEventButton  *event,
                                   gpointer         user_data){
  //**/g_printerr("press\n");
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
  for(j=1; j<= go.grid_size; j++){
    for(i=1; i<=go.grid_size; i++){
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

  //**/g_printerr("Examining %d %d (%d)\n",gox,goy, my_color);
  //**/_print_gird();
  //**/scanf("%c",&c);scanf("%c",&c);

  x = gox;
  y = goy - 1;
  if(goy>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //**/g_printerr("Now examining %d %d (%d)\n",x,y, go.grid[x][y]);
    stamp_group_of(x,y);
  }
  x = gox + 1;
  y = goy;
  if(gox<go.grid_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //**/g_printerr("Now examining %d %d (%d)\n",x,y, go.grid[x][y]);
    stamp_group_of(x,y);
  }
  x = gox;
  y = goy + 1;
  if(goy<go.grid_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //**/g_printerr("Now examining %d %d (%d)\n",x,y, go.grid[x][y]);
    stamp_group_of(x,y);
  }
  x = gox - 1;
  y = goy;
  if(gox>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    //**/g_printerr("Now examining %d %d (%d)\n",x,y, go.grid[x][y]);
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

  //**/g_printerr("I am          %d %d (%d)\n",gox,goy, my_color);
  //**/_print_gird();
  //**/scanf("%c",&c);scanf("%c",&c);

  x = gox;
  y = goy - 1;
  if(goy>1 && go.grid[x][y] == my_color && !go.stamps[x][y]){
    count += size_group_of(x,y);
  }
  x = gox + 1;
  y = goy;
  if(gox<go.grid_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
    count += size_group_of(x,y);
  }
  x = gox;
  y = goy + 1;
  if(goy<go.grid_size && go.grid[x][y] == my_color && !go.stamps[x][y]){
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

  //**/g_printerr("I am          %d %d (%d)\n",gox,goy, my_color);
  //**/_print_gird();
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
  if(gox<go.grid_size && !go.stamps[x][y]){
    if(go.grid[x][y] == EMPTY) return TRUE;
    else if( go.grid[x][y] == my_color )
      if(is_alive_group_of(x,y)) return TRUE;
  }
  x = gox;     //SOUTH
  y = goy + 1;
  if(goy<go.grid_size && !go.stamps[x][y]){
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

  for(i=1; i<= go.grid_size; i++){
    for(j=1; j<=go.grid_size; j++){
      if(go.stamps[i][j]){
        remove_stone(i,j);
        count++;
      }
    }
  }
  clear_stamps();
  return count;
}


void play_at(int gox, int goy){
  /**/g_printerr("Play at %d %d", gox, goy);
  if( gox < 1 || go.grid_size < gox ||
      goy < 1 || go.grid_size < goy ){
    /**/g_printerr("--> out of bounds\n");
    return;
  }
  if(go.grid[gox][goy] != 0){
    /**/g_printerr("--> not empty\n");

    //g_printerr("KILLED: %d\n", kill_group_of(gox, goy));
    //g_printerr("ALIVE: %s\n", is_alive_group_of(gox, goy)?"TRUE":"FALSE");
    //g_printerr("COUNT: %d\n", size_group_of(gox, goy));
    //stamp_group_of(gox, goy);
    //remove_stone(gox, goy);
    clear_stamps();
    return;
  }
  /**/g_printerr("\n");

  paint_stone(go.drawing_area,
              (gox -1) * go.grid_space + go.grid_margin - go.grid_space / 2,
              (goy -1) * go.grid_space + go.grid_margin - go.grid_space / 2,
              (go.turn%2)?WHITE_STONE:BLACK_STONE);

  go.grid[gox][goy] = (go.turn%2)?WHITE_STONE:BLACK_STONE;

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
    if(gox<go.grid_size && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = gox;
    y = goy + 1;
    if(goy<go.grid_size && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
    clear_stamps();
    x = gox - 1;
    y = goy;
    if(gox>1 && go.grid[x][y] == opp_color && !is_alive_group_of(x,y)){
      *captured += kill_group_of(x,y);
    }
  }

  {//--update capture label
    char * new_label_string = NULL;
    new_label_string = (char *) malloc (100 * sizeof(char));
    g_printerr("Capture points: W %d - B %d\n", go.white_captures, go.black_captures);
    sprintf(new_label_string, "B: %d W: %d", go.black_captures, go.white_captures);
    g_printerr("Capture points: %s\n", new_label_string);
    gtk_label_set_text (GTK_LABEL (go.capture_label), new_label_string);
  }
  //--next turn
  go.turn++;
}

gboolean
on_drawing_area_button_release_event(GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     gpointer         user_data){
  /**/g_printerr("release\n");

  //**/paint_stone(widget,
  //**/            (go.x - 1) * go.grid_space + go.grid_margin - go.grid_space / 2,
  //**/            (go.y - 1) * go.grid_space + go.grid_margin - go.grid_space / 2,
  //**/            (go.turn%2)?WHITE_STONE:BLACK_STONE);
  //**/go.turn++;

  play_at(go.x, go.y);

  return FALSE;
}

void on_button_pref_pressed (void){
  /**/g_printerr("Preferences.\n");
  _print_gird();
}
void on_button_prev_pressed (void){
  /**/g_printerr("prev\n");
}
void on_button_next_pressed (void){
  /**/g_printerr("next\n");
}

void gui_init(){
  GtkWidget   * window;
  GtkWidget   * vbox;

  GtkWidget * toolbar;
  GdkPixbuf * pixbuf;
  GtkWidget * image;

  GtkWidget * capture_label;

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

  pixbuf = gpe_find_icon ("left");
  image = gtk_image_new_from_pixbuf(pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   NULL, NULL, NULL,
			   image, GTK_SIGNAL_FUNC (on_button_prev_pressed), NULL);
  gdk_pixbuf_unref(pixbuf);

  pixbuf = gpe_find_icon ("right");
  image = gtk_image_new_from_pixbuf(pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   NULL, NULL, NULL,
			   image, GTK_SIGNAL_FUNC (on_button_next_pressed), NULL);
  gdk_pixbuf_unref(pixbuf);

  pixbuf = gpe_find_icon ("prefs");
  image = gtk_image_new_from_pixbuf(pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   NULL, NULL, NULL,
			   image, GTK_SIGNAL_FUNC (on_button_pref_pressed), NULL);
  gdk_pixbuf_unref(pixbuf);

  //--Capture label
  capture_label = gtk_label_new(NULL);
  
  //let put it in the toolbar. Will find a better place later
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), capture_label, NULL, NULL);
  go.capture_label = capture_label;

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

