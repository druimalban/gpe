/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "sketchpad.h"
#include "sketchpad-cb.h"
#include "files.h"
#include "selector.h"

GtkWidget * window_sketchpad;
GtkWidget * drawing_area;
GdkPixmap * drawing_area_pixmap_buffer;

GdkGC * graphical_context;
GdkColormap * colormap;
GdkColor * current_color;//FIXME: use that to ref the current color
GdkColor white;
GdkColor black;
GdkColor red    = {0, 65535, 0, 0 };
GdkColor green  = {0, 0, 65535, 0 };
GdkColor blue   = {0, 0, 0, 65535 };

gint drawing_area_width;
gint drawing_area_height;
gint prev_pos_x;
gint prev_pos_y;
gint tool;
gint color;
gint brush;

//--Cursors:
GdkCursor ** current_cursor;//ref the current one
GdkCursor * cursor_pen;     //ref the pen cursor (none)
GdkCursor * cursor_eraser;  //ref the eraser cursor
GdkCursor * cursor_none;
#include "cursor_eraser_data.h"
GdkCursor * cursor_eraser_small;
GdkCursor * cursor_eraser_medium;
GdkCursor * cursor_eraser_large;
GdkCursor * cursor_eraser_xlarge;
GdkCursor * _eraser_cursors_new(gchar * bits, gint width, gint height);//gint diameter);

void   _reset_drawing_area       ();
void    clear_drawing_area       ();

void sketchpad_init(){
  drawing_area_pixmap_buffer = NULL;

  drawing_area_width  = 230;
  drawing_area_height = 250;

  prev_pos_x = NO_PREV_POS;
  prev_pos_y = NO_PREV_POS;

  tool  = PEN;
  color = BLACK;
  brush = MEDIUM;
}//sketchpad_init()

void window_sketchpad_init(GtkWidget * window_sketchpad){

  //--colors
  colormap = gdk_colormap_get_system();
  gdk_color_black(colormap, &black);
  gdk_color_white(colormap, &white);
  gdk_colormap_alloc_color(colormap, &red,  FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &green,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &blue, FALSE,TRUE);

  //--cursors
  {//none cursor
    GdkPixmap * cursor_pixmap;
    static unsigned char cursor_none_data[] = { 0x01 }; //single spot
    //static unsigned char cursor_none_data[] = { 0x00 }; //empty
    cursor_pixmap = gdk_bitmap_create_from_data(NULL, cursor_none_data, 1, 1);
    cursor_none   = gdk_cursor_new_from_pixmap(cursor_pixmap, cursor_pixmap,
                                               &black, &white, 1, 1);
    gdk_pixmap_unref(cursor_pixmap);
  }
  //eraser cursors
  cursor_eraser_small  = _eraser_cursors_new(small_bits,  small_width,  small_height);
  cursor_eraser_medium = _eraser_cursors_new(medium_bits, medium_width, medium_height);
  cursor_eraser_large  = _eraser_cursors_new(large_bits,  large_width,  large_height);
  cursor_eraser_xlarge = _eraser_cursors_new(xlarge_bits, xlarge_width, xlarge_height);

  //defaults cursor
  cursor_pen     = cursor_none;
  cursor_eraser  = cursor_eraser_small;
  current_cursor = &cursor_pen;
  
}//window_sketchpad_init()

GdkCursor * _eraser_cursors_new(gchar * bits, gint width, gint height){//gint diameter){
  GdkCursor * cursor;
  GdkPixmap * cursor_pixmap;
  //diameter++;
  //cursor_pixmap = gdk_pixmap_new (drawing_area->window, 16, 16, 1);
  //gdk_draw_arc(cursor_pixmap, graphical_context, FALSE,
  //             8, 8, MIN(16,diameter), MIN(16,diameter),
  //             0, 23040);//23040 == 360*64
  //cursor = gdk_cursor_new_from_pixmap(cursor_pixmap, cursor_pixmap,
  //                                    &black, &white, diameter/2, diameter/2);
  cursor_pixmap = gdk_bitmap_create_from_data(NULL, bits, width, height);
  cursor        = gdk_cursor_new_from_pixmap(cursor_pixmap, cursor_pixmap,
                                             &black, &white,
                                             width/2, height/2);
  gdk_pixmap_unref(cursor_pixmap);
  return cursor;
}//

//void sketchpad_set_tool (gint _tool)  { tool  = _tool;}
//void sketchpad_set_color(gint _color) { color = _color;}
//void sketchpad_set_brush(gint _brush) { brush = _brush;}
void sketchpad_set_tool_s  (gchar * _tool) {
  if(g_strcasecmp(_tool, "eraser") == 0){
    tool   = ERASER;
    current_cursor = &cursor_eraser;
  }
  else{//default tool
    tool   = PEN;
    current_cursor = &cursor_pen;
  }
  gdk_window_set_cursor(drawing_area->window, *current_cursor);
}

void sketchpad_set_brush_s (gchar * _brush){
  if(g_strcasecmp(_brush, "medium") == 0){
    brush = MEDIUM;
    cursor_eraser = cursor_eraser_medium;
  }
  else if(g_strcasecmp(_brush, "large" ) == 0){
    brush = LARGE;
    cursor_eraser = cursor_eraser_large;
  }
  else if(g_strcasecmp(_brush, "xlarge") == 0){
    brush = XLARGE;
    cursor_eraser = cursor_eraser_xlarge;
  }
  else{//default size
    brush = SMALL;
    cursor_eraser = cursor_eraser_small;
  }
  gdk_window_set_cursor(drawing_area->window, *current_cursor);
}

void sketchpad_set_color_s (gchar * _color){
  //**/g_printerr("color -->%s<--\n", _color);
  if     (g_strcasecmp(_color, "black") == 0) color = BLACK;
  else if(g_strcasecmp(_color, "red")   == 0) color = RED;
  else if(g_strcasecmp(_color, "green") == 0) color = GREEN;
  else if(g_strcasecmp(_color, "blue")  == 0) color = BLUE;
  else color = BLACK;
}

void sketchpad_set_title(const gchar * name){
  gchar * title;
  title = g_strdup_printf("[%d/%d] %s",
                          (is_current_sketch_new)?sketch_list_size+1:current_sketch +1,
                          sketch_list_size, 
                          name);
  gtk_window_set_title(GTK_WINDOW (window_sketchpad), title);
  g_free(title);
}

void sketchpad_open_file(gchar * fullpath_filename, const gchar * name){
  //**/g_printerr("sketchpad open: %s\n", fullpath_filename);
  file_load(fullpath_filename);
  sketchpad_set_title(name);
}//sketchpad_open_file()

void sketchpad_new_sketch(){
  _reset_drawing_area(drawing_area);
  sketchpad_set_title(SKETCHPAD_TITLE_NEW);
  sketchpad_refresh_drawing_area(drawing_area);
}//sketchpad_new_sketch()

void sketchpad_set_drawing_area(GtkWidget * a_drawing_area){
  drawing_area = a_drawing_area;
}

/* Draw a circle on the screen */
void draw_point(gdouble x, gdouble y){
  GdkRectangle ellipse;
  GdkRectangle * updated_rectangle;

  ellipse.x      = x - brush/2;
  ellipse.y      = y - brush/2;
  ellipse.width  = brush;
  ellipse.height = brush;

  if(tool == ERASER){
    gdk_gc_set_foreground(graphical_context, &white);
  }
  else switch(color){
    case BLACK : gdk_gc_set_foreground(graphical_context, &black); break;
    case RED   : gdk_gc_set_foreground(graphical_context, &red);   break;
    case GREEN : gdk_gc_set_foreground(graphical_context, &green); break;
    case BLUE  : gdk_gc_set_foreground(graphical_context, &blue);  break;
    default    : gdk_gc_set_foreground(graphical_context, &black);
  }

  gdk_draw_arc(drawing_area_pixmap_buffer,
               graphical_context,
               TRUE,
               ellipse.x,    ellipse.y,
               ellipse.width,ellipse.height,
               0, 23040);//23040 == 360*64

  updated_rectangle = & ellipse;
  gtk_widget_draw (drawing_area, updated_rectangle);
}//draw_point()

void draw_line (gdouble x1, gdouble y1,
                gdouble x2, gdouble y2){

  GdkRectangle updated_rectangle;

  gdk_gc_set_line_attributes(graphical_context,
                             brush,
                             GDK_LINE_SOLID,
                             GDK_CAP_ROUND,
                             GDK_JOIN_ROUND);

  gdk_draw_line(drawing_area_pixmap_buffer,
                graphical_context, x1, y1, x2, y2);

  updated_rectangle.x      = MIN(x1,x2) - brush;
  updated_rectangle.y      = MIN(y1,y2) - brush;
  updated_rectangle.width  = ABS(x1 - x2) + 2* brush + 1;
  updated_rectangle.height = ABS(y1 - y2) + 2* brush + 1;
  gtk_widget_draw (drawing_area, &updated_rectangle);
}

void _reset_drawing_area(){
  if(drawing_area_pixmap_buffer) gdk_pixmap_unref(drawing_area_pixmap_buffer);
  drawing_area_pixmap_buffer = gdk_pixmap_new(drawing_area->window,
                                              drawing_area_width,
                                              drawing_area_height,
                                              -1);
  //paint a white rectangle covering the whole area
  gdk_draw_rectangle (drawing_area_pixmap_buffer,
                      drawing_area->style->white_gc,
                      TRUE,
                      0, 0,
                      drawing_area_width,
                      drawing_area_height);
  graphical_context = gdk_gc_new(drawing_area->window);
}

void sketchpad_refresh_drawing_area(){
  gdk_draw_pixmap(drawing_area->window,
                  drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
                  drawing_area_pixmap_buffer,
                  0, 0, 0, 0,
                  drawing_area_width, drawing_area_height);  
}

void clear_drawing_area(){
  _reset_drawing_area();
  sketchpad_refresh_drawing_area();
}

gint configure_event_handler(GtkWidget *_drawing_area, GdkEventConfigure *event){
  //when window is created or resized
  if(!drawing_area_pixmap_buffer){
    _reset_drawing_area(_drawing_area);
  }
  return TRUE;
}//configure_event_handler()

gint expose_event_handler(GtkWidget * _drawing_area, GdkEventExpose *event){
  //refresh the part outdated by the event
  gdk_draw_pixmap(_drawing_area->window,
                  _drawing_area->style->fg_gc[GTK_WIDGET_STATE (_drawing_area)],
                  drawing_area_pixmap_buffer,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);
  return FALSE;
}//expose_event_handler()

gint button_press_event_handler(GtkWidget *_drawing_area, GdkEventButton *event ){
  if (event->button == 1 && drawing_area_pixmap_buffer != NULL){
    draw_point (event->x, event->y);
    prev_pos_x = event->x;
    prev_pos_y = event->y;
  }
  return TRUE;
}//button_press_event_handler()

gint button_release_event_handler(GtkWidget *_drawing_area, GdkEventButton *event ){
  prev_pos_x = prev_pos_y = NO_PREV_POS;
  return TRUE;
}//button_release_event_handler()

gint motion_notify_event_handler(GtkWidget *_drawing_area, GdkEventMotion *event){
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
  if (state & GDK_BUTTON1_MASK){// && drawing_area_pixmap_buffer != NULL){
    if(prev_pos_x != NO_PREV_POS){
      draw_line(prev_pos_x, prev_pos_y, x, y);
    }
    prev_pos_x = x;
    prev_pos_y = y;
  }
  return TRUE;
}//motion_notify_event_handler()

