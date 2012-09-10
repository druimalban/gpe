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
#include <stdlib.h>
#include "gpe/errorbox.h"
#include "gpe-sketchbook.h"
#include "sketchpad.h"
#include "sketchpad-cb.h"
#include "files.h"
#include "selector.h"
#include "selector-cb.h"


//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

Sketchpad sketchpad;

GtkWidget * drawing_area = NULL;
GdkPixmap * drawing_area_pixmap_buffer;

gboolean is_current_sketch_modified;

GdkGC * graphical_context = NULL;
GdkColormap * colormap;
GdkColor * current_color;
GdkColor white;
GdkColor black;
GdkColor red    = {0, 65535, 0, 0 };
GdkColor green  = {0, 0, 65535, 0 };
GdkColor blue   = {0, 0, 0, 65535 };
GdkColor yellow = {0, 65535, 65535, 0 };
GdkColor orange = {0, 65535, 32767, 0 };

gint drawing_area_width;
gint drawing_area_height;
gint prev_pos_x;
gint prev_pos_y;

gint tool;//current_tool
gint brush;//current_brush

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
GdkCursor * _eraser_cursors_new(unsigned char * bits, gint width, gint height);//gint diameter);

void    clear_drawing_area       ();

void sketchpad_init(){
  drawing_area_pixmap_buffer = NULL;
  is_current_sketch_modified = FALSE;
  
  /* set defaults, exact match is calculated later */
  drawing_area_width  = 100;
  drawing_area_height = 100;

  prev_pos_x = NO_PREV_POS;
  prev_pos_y = NO_PREV_POS;

  tool  = PEN;
  brush = MEDIUM;
}//sketchpad_init()

/*
 * Changes the drawing area's size, and ensures it's realized
 */
static void set_drawing_area_size(int width, int height){
  drawing_area_width = width;
  drawing_area_height = height;
  gtk_widget_set_usize(drawing_area, width, height);

  if(! GTK_WIDGET_REALIZED(drawing_area)){ //drawing_area->window == NULL
    gtk_widget_realize(drawing_area);
  }

  // If the window size has changed, we will need to create a new pixmap.
  if (drawing_area_pixmap_buffer){
    gint oldwidth, oldheight;
    gdk_drawable_get_size(GDK_DRAWABLE(drawing_area_pixmap_buffer),
                          &oldwidth, &oldheight);
    if (oldwidth != drawing_area_width || oldheight != drawing_area_height){
      gdk_pixmap_unref(drawing_area_pixmap_buffer);
      drawing_area_pixmap_buffer = NULL;
    }
  }

  if(! drawing_area_pixmap_buffer){
    drawing_area_pixmap_buffer = gdk_pixmap_new(drawing_area->window,
						drawing_area_width,
						drawing_area_height,
						-1);
  }
}


void window_sketchpad_init(GtkWidget * window_sketchpad){

  //--colors
  colormap = gdk_colormap_get_system();

  gdk_color_black(colormap, &black);
  gdk_color_white(colormap, &white);
  gdk_colormap_alloc_color(colormap, &red,  FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &green,FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &blue, FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &yellow, FALSE,TRUE);
  gdk_colormap_alloc_color(colormap, &orange, FALSE,TRUE);

  current_color = &black;

  //--cursors
  {//none cursor
    GdkPixmap * cursor_pixmap;
#ifdef DESKTOP
    static unsigned char cursor_none_data[] = { 0x01 }; //single spot
#else
    static unsigned char cursor_none_data[] = { 0x00 }; //empty
#endif
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

  //--defaults cursor
  cursor_pen     = cursor_none;
  cursor_eraser  = cursor_eraser_small;
  current_cursor = &cursor_pen;
}//window_sketchpad_init()

GdkCursor * _eraser_cursors_new(unsigned char * bits, gint width, gint height){//gint diameter){
  GdkCursor * cursor;
  GdkPixmap * cursor_pixmap;
  cursor_pixmap = gdk_bitmap_create_from_data(NULL, bits, width, height);
  cursor        = gdk_cursor_new_from_pixmap(cursor_pixmap, cursor_pixmap,
                                             &black, &white,
                                             width/2, height/2);
  gdk_pixmap_unref(cursor_pixmap);
  return cursor;
}//

//void sketchpad_set_tool (gint _tool)  { tool  = _tool;}
//void sketchpad_set_brush(gint _brush) { brush = _brush;}
void sketchpad_set_tool_s  (gchar * _tool) {
  if(g_ascii_strcasecmp(_tool, "eraser") == 0){
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
  if(g_ascii_strcasecmp(_brush, "medium") == 0){
    brush = MEDIUM;
    cursor_eraser = cursor_eraser_medium;
  }
  else if(g_ascii_strcasecmp(_brush, "large" ) == 0){
    brush = LARGE;
    cursor_eraser = cursor_eraser_large;
  }
  else if(g_ascii_strcasecmp(_brush, "xlarge") == 0){
    brush = XLARGE;
    cursor_eraser = cursor_eraser_xlarge;
  }
  else{//default size
    brush = SMALL;
    cursor_eraser = cursor_eraser_small;
  }
  if(drawing_area->window) gdk_window_set_cursor(drawing_area->window, *current_cursor);
}

void sketchpad_reset_title(){
  gchar * title;
  title = g_strdup_printf("%s [%d/%d]%s%s%s",
                          _("Sketch"),
                          (is_current_sketch_new)?sketch_list_size+1:current_sketch +1,
                          sketch_list_size,
                          (is_current_sketch_modified)?" *":"",
                          (is_current_sketch_new)?" ":"",
                          (is_current_sketch_new)?_("new"):"");
  gtk_window_set_title(GTK_WINDOW (sketchbook.window), title);
  g_free(title);
}

void sketchpad_open_file(gchar * fullpath_filename){
  //**/g_printerr("sketchpad open: %s\n", fullpath_filename);
  file_load(fullpath_filename);
  is_current_sketch_modified = FALSE;
  sketchpad_reset_title();
}//sketchpad_open_file()

void sketchpad_new_sketch(){
gint width,height;
  drawing_area_pixmap_buffer = NULL;
  is_current_sketch_modified = FALSE;

  /* calculate exact size for the drawing area */
  set_drawing_area_size(100, 100); /* turn off scrollbars */
  gtk_widget_realize(gtk_widget_get_toplevel(drawing_area)); /* make sure widgets allocate space */
  gtk_widget_realize(drawing_area->parent);
  gtk_widget_realize(drawing_area->parent->parent);
  gdk_drawable_get_size(GDK_DRAWABLE(drawing_area->parent->window), &width, &height);

  /* if we use the joypad to grow we reduce the size to avoid scrollbars */
  if (sketchbook.prefs.joypad_scroll || !sketchbook.prefs.grow_on_scroll)
  {
    width -= 4; /* used by the widget itself */
    height -= 4;
  }
  
  drawing_area_width  = width;
  drawing_area_height = height;
  reset_drawing_area();
  is_current_sketch_modified = FALSE;
  sketchpad_reset_title();
  sketchpad_refresh_drawing_area();
}//sketchpad_new_sketch()


/*
 * Creates a new sketch and opens the selected image when the fileselector's
 * OK button is clicked.
 */
static void import_from_filesel(GtkFileSelection *fs, GtkButton *ignored)
{
  const gchar *file = gtk_file_selection_get_filename(fs);

  if(! file_load(file)){
    gpe_error_box_fmt(_("Import of `%s' failed."), file);
    // leave the file selector open
  }
  else {
    current_sketch = SKETCH_NEW;
    is_current_sketch_modified = TRUE;	// so the user is prompted to save
    sketchpad_reset_title();
    gtk_widget_destroy(GTK_WIDGET(fs));

    // Switch to the sketchpad page if we were called from the image selector
    switch_to_page(PAGE_SKETCHPAD);
  }
}


// Opens a file selection dialog for the user to import an image
void sketchpad_import_image(void){
  GtkWidget *file_selector = gtk_file_selection_new(_("Image to import"));

  //gtk_file_selection_complete(GTK_FILE_SELECTION(file_selector), "*.png");

  gtk_signal_connect_object
      (GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->ok_button),
       "clicked", GTK_SIGNAL_FUNC(import_from_filesel), file_selector);

  gtk_signal_connect_object
      (GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->cancel_button),
       "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), file_selector);

  gtk_widget_show(file_selector);
}

/*
 * Expands the image vertically or horizontally by the given number of pixels.
 * If numPix is negative, expands on the top/left and moves everything over.
 */
void sketchpad_expand(GtkAdjustment *adj, gboolean isHoriz, int numPix){
  int oldWidth = drawing_area_width;
  int oldHeight = drawing_area_height;
  int n = abs(numPix);
  GdkPixmap *oldbuff;

  if(sketchbook.prefs.grow_on_scroll == FALSE) return;

  if (isHoriz)
    drawing_area_width += n;
  else
    drawing_area_height += n;

  // let reset_drawing_area() handle the resize & blank
  oldbuff = drawing_area_pixmap_buffer;
  gdk_pixmap_ref(oldbuff);
  reset_drawing_area();

  // copy in the old pixmap
  gdk_draw_drawable(GDK_DRAWABLE(drawing_area_pixmap_buffer), graphical_context,
                    GDK_DRAWABLE(oldbuff),
		    0, 0,
		    isHoriz ? (numPix > 0 ? 0 : n) : 0,	// xdest
		    isHoriz ? 0 : (numPix > 0 ? 0 : n),	// ydest
		    oldWidth, oldHeight);
  gdk_pixmap_unref(oldbuff);

  // If the image grew down or to the right, update the scroll bounds so
  // the image is scrolled all the way to the new bottom/right.
  if (numPix > 0){
    adj->upper += numPix;
    gtk_adjustment_changed(adj);
  }
  // don't mark the sketch as modified till they actually draw something in it
}

void sketchpad_set_drawing_area(GtkWidget * a_drawing_area){
  drawing_area = a_drawing_area;
}

/* Draw a circle on the screen */
void draw_point(gdouble x, gdouble y){
  GdkRectangle ellipse;
  GdkRectangle * updated_rectangle;

  //FIXME: to be done just when switching brush!
  ellipse.x      = x - brush/2;
  ellipse.y      = y - brush/2;
  ellipse.width  = brush;
  ellipse.height = brush;

  if(tool == ERASER){//FIXME: to be done just when switching tool/color!
    gdk_gc_set_foreground(graphical_context, &white);
  }
  else{
    gdk_gc_set_foreground(graphical_context, current_color);
  }

  gdk_draw_arc(drawing_area_pixmap_buffer,
               graphical_context,
               TRUE,
               ellipse.x,    ellipse.y,
               ellipse.width,ellipse.height,
               0, 23040);//23040 == 360*64

  updated_rectangle = & ellipse;
  gtk_widget_draw (drawing_area, updated_rectangle);
  if(is_current_sketch_modified == FALSE){
    is_current_sketch_modified = TRUE;
    sketchpad_reset_title();
  }
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
  if(is_current_sketch_modified == FALSE){
    is_current_sketch_modified = TRUE;
    sketchpad_reset_title();
  }
}


// Creates or recreates the drawing area buffer and paints it blank
void reset_drawing_area(void){
  set_drawing_area_size(drawing_area_width, drawing_area_height);

  //paint a white rectangle covering the whole area
  gdk_draw_rectangle (drawing_area_pixmap_buffer,
                      drawing_area->style->white_gc,
                      TRUE,
                      0, 0,
                      drawing_area_width,
                      drawing_area_height);
  if (graphical_context) 
	  gdk_gc_unref(graphical_context);
  graphical_context = gdk_gc_new(drawing_area->window);
  gdk_window_set_cursor(drawing_area->window, *current_cursor);
}

void sketchpad_refresh_drawing_area(void){
  gdk_draw_pixmap(drawing_area->window,
                  drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
                  drawing_area_pixmap_buffer,
                  0, 0, 0, 0,
                  drawing_area_width, drawing_area_height);  
}

GdkPixbuf * sketchpad_get_current_sketch_pixbuf(){
  return gdk_pixbuf_get_from_drawable(NULL,//GdkPixbuf *dest,
                                      drawing_area_pixmap_buffer,//GdkDrawable *src,
                                      colormap,//GdkColormap *cmap,
                                      0,//int src_x,
                                      0,//int src_y,
                                      0,//int dest_x,
                                      0,//int dest_y,
                                      drawing_area_width,//int width,
                                      drawing_area_height);//int height);
}

void sketchpad_set_current_sketch_from_pixbuf(GdkPixbuf * pixbuf){
  drawing_area_width  = gdk_pixbuf_get_width (pixbuf);
  drawing_area_height = gdk_pixbuf_get_height(pixbuf);

  set_drawing_area_size(gdk_pixbuf_get_width(pixbuf),
                        gdk_pixbuf_get_height(pixbuf));

  gdk_pixbuf_render_to_drawable(pixbuf,//GdkPixbuf *pixbuf,
                                drawing_area_pixmap_buffer,//GdkDrawable *drawable,
                                graphical_context,//GdkGC *gc,
                                0,//int src_x,
                                0,//int src_y,
                                0,//int dest_x,
                                0,//int dest_y,
                                drawing_area_width,//int width,
                                drawing_area_height,//int height,
                                GDK_RGB_DITHER_NONE,//GdkRgbDither dither,
                                0,//int x_dither,
                                0);//int y_dither);
  sketchpad_refresh_drawing_area();
}
