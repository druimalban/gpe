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
#include <gtk/gtk.h>

#include "gpe-go.h"//CLEAN do not include, use GoGame GoBoard parameters
#include "board.h"
#include "model.h"

#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif

//colors
GdkGC       * gc;
GdkColormap * colormap;
GdkColor red  = {0, 65535, 0,     0};
GdkColor blue = {0,     0, 0, 65535};


#define PIXMAPS_SRC  PREFIX "/share/gpe/pixmaps/default/" PACKAGE "/"

void load_graphics(){
  //--board and stones
  go.board.loaded_board       = gdk_pixbuf_new_from_file(PIXMAPS_SRC "board", NULL);
  go.board.loaded_black_stone = gdk_pixbuf_new_from_file(PIXMAPS_SRC "black.png", NULL);
  go.board.loaded_white_stone = gdk_pixbuf_new_from_file(PIXMAPS_SRC "white.png", NULL);

  //--colors
  gc = gdk_gc_new(go.board.drawing_area->window);
  colormap = gdk_colormap_get_system();
  gdk_colormap_alloc_color(colormap, &red ,  FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &blue,  FALSE,TRUE);
}

void scale_graphics(){
  int stone_size = go.board.stone_size;
  int i;
  int cell_size;
  int border_trans; //grid's translation from the border of the board

  //--black stone
  if(go.board.pixbuf_black_stone != NULL){
    gdk_pixbuf_unref(go.board.pixbuf_black_stone);
  }
  if(go.board.loaded_black_stone != NULL){
    go.board.pixbuf_black_stone = gdk_pixbuf_scale_simple (go.board.loaded_black_stone,
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
  if(go.board.pixbuf_white_stone != NULL){
    gdk_pixbuf_unref(go.board.pixbuf_white_stone);
  }
  if(go.board.loaded_white_stone != NULL){
    go.board.pixbuf_white_stone = gdk_pixbuf_scale_simple (go.board.loaded_white_stone,
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
  if(go.board.pixmap_empty_board == NULL){
    go.board.pixmap_empty_board = gdk_pixmap_new (go.board.drawing_area->window,
                                            BOARD_SIZE,
                                            BOARD_SIZE,
                                            -1);
  }
  if(go.board.loaded_board != NULL){
    GdkPixbuf * scaled_bg;

    scaled_bg = gdk_pixbuf_scale_simple (go.board.loaded_board,
                                         BOARD_SIZE, BOARD_SIZE,
                                         GDK_INTERP_BILINEAR);
    gdk_draw_pixbuf(go.board.pixmap_empty_board,
                    go.board.drawing_area->style->white_gc,
                    scaled_bg,
                    0, 0, 0, 0,
                    -1, -1,
                    GDK_RGB_DITHER_NONE, //GdkRgbDither dither,
                    0, //gint x_dither,
                    0); //gint y_dither);
    gdk_pixbuf_unref(scaled_bg);
  }
  else{//draw it yourself
    gdk_draw_rectangle (go.board.pixmap_empty_board,
                        go.board.drawing_area->style->white_gc,//FIXME: use better color
                        TRUE,
                        0, 0,
                        BOARD_SIZE,
                        BOARD_SIZE);
  }
  TRACE("scaled board");

  //grid
  cell_size = go.board.cell_size;
  border_trans = go.board.margin + go.board.cell_size / 2;
  
  for(i=0; i< go.game.size; i++){
    //vertical lines
    gdk_draw_line(go.board.pixmap_empty_board,
                  go.board.drawing_area->style->black_gc,
                  border_trans + cell_size * i, border_trans,
                  border_trans + cell_size * i, BOARD_SIZE - border_trans);
    //horizontal lines
    gdk_draw_line(go.board.pixmap_empty_board,
                  go.board.drawing_area->style->black_gc,
                  border_trans, border_trans + cell_size * i,
                  BOARD_SIZE - border_trans, border_trans + cell_size * i);
  }
  TRACE("scaled grid");

  //advantage points for 9/13/19 boards

#define ADVANTAGE_POINT_SIZE 3
#define paint_point(x,y) \
      gdk_draw_rectangle(go.board.pixmap_empty_board, \
               go.board.drawing_area->style->black_gc, TRUE, \
               border_trans + cell_size * (x-1) - ADVANTAGE_POINT_SIZE/2, \
               border_trans + cell_size * (y-1) - ADVANTAGE_POINT_SIZE/2, \
               ADVANTAGE_POINT_SIZE, ADVANTAGE_POINT_SIZE);

  if(go.game.size == 9){
    paint_point(3, 3);
    paint_point(7, 3);
    paint_point(5, 5);
    paint_point(3, 7);
    paint_point(7, 7);
  }
  else if(go.game.size == 13){
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
  else if(go.game.size == 19){
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


void paint_board(){
  GdkRectangle rect;
  gdk_draw_drawable (go.board.drawing_area_pixmap_buffer,
                     go.board.drawing_area->style->black_gc,
                     go.board.pixmap_empty_board,
                     0, 0, 0, 0, BOARD_SIZE, BOARD_SIZE);
  rect.x = rect.y = 0;
  rect.width = rect.height = BOARD_SIZE;
  gdk_window_invalidate_rect(go.board.drawing_area->window, &rect, FALSE);
}

void unpaint_stone(int col, int row){
  GdkRectangle rect;

  rect.x = (col -1) * go.board.cell_size + go.board.margin;
  rect.y = (row -1) * go.board.cell_size + go.board.margin;
  rect.width = rect.height = go.board.cell_size;

  gdk_draw_drawable (go.board.drawing_area_pixmap_buffer,
                     go.board.drawing_area->style->black_gc,
                     go.board.pixmap_empty_board,
                     rect.x, rect.y, rect.x, rect.y,
                     rect.width, rect.height);
  gdk_window_invalidate_rect(go.board.drawing_area->window, &rect, FALSE);
}

/* optional last args for specific marks */
void paint_mark(int col, int row, GoMark mark, GdkColor * color, ...){
  GdkRectangle rect;
  int position;
  int size;

  rect.x = go.board.margin + (col -1) * go.board.cell_size + go.board.stone_space;
  rect.y = go.board.margin + (row -1) * go.board.cell_size + go.board.stone_space;
  rect.width = rect.height = go.board.stone_size;

  TRACE("mark (%d %d) %s", col, row,
        (go.game.grid[col][row] == BLACK_STONE)?"black":"white");

  gdk_gc_set_foreground(gc, color);

  switch(mark){
    case MARK_SQUARE:
      position = go.board.stone_size / 4;
      size = go.board.stone_size / 2;
      if( !((size - go.board.grid_stroke)%2) ) size++;

      gdk_draw_rectangle (go.board.drawing_area_pixmap_buffer,
                          gc,
                          FALSE,
                          rect.x + position, rect.y + position,
                          size, size
                          );
      break;
    case MARK_SPOT:
      size = go.board.stone_size / 1.5;
      if( !((size - go.board.grid_stroke)%2) ) size++;
      position = (go.board.cell_size - size ) / 2 -1;

      gdk_draw_arc(go.board.drawing_area_pixmap_buffer,
                   gc,
                   TRUE,//filled
                   rect.x + position,
                   rect.y + position,
                   size, size,
                   0, 23040);//23040 == 360*64
      break;
    case MARK_LABEL:
      {
        va_list ap;
        char * s;
        char * font_name;
        int    font_size;
        PangoFontDescription * font;
        PangoContext * context;
        PangoLayout  * layout;
        GdkGC * gc;
        int text_width;
        int text_heigh;

        //Pango context and layout
        context = gtk_widget_create_pango_context(go.board.drawing_area);
        //pango_context_set_
        layout = pango_layout_new (context);

        //font
        font_size = (int)(go.board.stone_size * (1 - 0.6)); //stone -x%
        if(font_size < 6)  font_size = 6; //min
        if(font_size > 10) font_size = 10;//max NOTE: check desktop default
        font_name = g_strdup_printf("Mono %d", font_size);
        TRACE("FONT %s (stone: %d)", font_name, go.board.stone_size);
        font = pango_font_description_from_string (font_name);
        pango_layout_set_font_description (layout, font);

        //label
        va_start (ap, color);
        s = va_arg(ap, char *);
        va_end (ap);
        TRACE("LABEL: ->%s<-\n", s);
        pango_layout_set_text (layout, s, -1);

        //color
        if(go.game.grid[col][row] == BLACK_STONE)
          gc = go.board.drawing_area->style->white_gc;
        else
          gc = go.board.drawing_area->style->black_gc;

        //rendering
        text_width = text_heigh = 0;
        pango_layout_get_pixel_size (layout, &text_width, &text_heigh);
        TRACE("LABEL w %d h %d", text_width, text_heigh);
        gdk_draw_layout(go.board.drawing_area_pixmap_buffer,
                        gc,
                        rect.x + (go.board.stone_size - text_width) / 2,//layout's left
                        rect.y + (go.board.stone_size - text_heigh) / 2,//layout's top
                        layout);
        g_free(context); g_free(layout);//NOTE: allocate once.
      }
      break;
    case NO_MARK:
    default:
      break;
  }
  gdk_window_invalidate_rect(go.board.drawing_area->window, &rect, FALSE);
}//paint_mark()

void paint_stone(int col, int row, GoItem item){
  GdkRectangle rect;

  rect.x = (col -1) * go.board.cell_size + go.board.margin + go.board.stone_space;
  rect.y = (row -1) * go.board.cell_size + go.board.margin + go.board.stone_space;
  rect.width = rect.height = go.board.stone_size;

  TRACE("paint");
 
  switch (item){
  case BLACK_STONE:
    gdk_draw_pixbuf(go.board.drawing_area_pixmap_buffer,
                    go.board.drawing_area->style->white_gc,
                    go.board.pixbuf_black_stone,
                    0, 0, rect.x, rect.y,
                    -1, -1,
                    GDK_RGB_DITHER_NONE, 0, 0);
    break;
  case WHITE_STONE:
    gdk_draw_pixbuf(go.board.drawing_area_pixmap_buffer,
                    go.board.drawing_area->style->white_gc,
                    go.board.pixbuf_white_stone,
                    0, 0, rect.x, rect.y,
                    -1, -1,
                    GDK_RGB_DITHER_NONE, 0, 0);
    break;
  case EMPTY:
    break;
  }//switch item
  gdk_window_invalidate_rect(go.board.drawing_area->window, &rect, FALSE);

}


gboolean
on_drawing_area_configure_event (GtkWidget         * widget,
                                 GdkEventConfigure * event,
                                 gpointer            user_data){
  //when window is created or resized
  if(!go.board.drawing_area_pixmap_buffer){
    go.board.drawing_area_pixmap_buffer = gdk_pixmap_new(widget->window,
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
                  go.board.drawing_area_pixmap_buffer,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);
  return FALSE;
}

gboolean
on_drawing_area_button_release_event(GtkWidget       *widget,
                                     GdkEventButton  *event,
                                     gpointer         user_data){
  TRACE("release");
  go.game.col = (event->x - go.board.margin) / go.board.cell_size + 1;
  go.game.row = (event->y - go.board.margin) / go.board.cell_size + 1;
  play_at(go.game.col, go.game.row);

  return FALSE;
}


void go_board_init(){
  GtkWidget * drawing_area;

  //--drawing area (Go Board)
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawing_area, BOARD_SIZE, BOARD_SIZE);
  gtk_widget_set_events (drawing_area, 0
                         | GDK_EXPOSURE_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         );

  go.board.drawing_area_pixmap_buffer = NULL;

  g_signal_connect (G_OBJECT (drawing_area), "configure_event",
                    G_CALLBACK (on_drawing_area_configure_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
                    G_CALLBACK (on_drawing_area_expose_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "button_release_event",
                    G_CALLBACK (on_drawing_area_button_release_event), NULL);

  go.board.drawing_area = drawing_area;
  go.board.widget       = drawing_area;//the top level widget

}

void go_board_resize(int game_size){
  int free_space;

  go.board.size = BOARD_SIZE;
  go.board.stone_space = 1;
  go.board.grid_stroke = 1;

  go.board.cell_size = go.board.size / game_size;
  if ((go.board.cell_size - go.board.grid_stroke - go.board.stone_space) %2) go.board.cell_size--;

  free_space = go.board.size - game_size * go.board.cell_size;
  if(free_space < 2){
    go.board.cell_size--;
    free_space += game_size;
  }
  go.board.margin = free_space / 2;
  
  go.board.stone_size = go.board.cell_size - go.board.stone_space;
}
