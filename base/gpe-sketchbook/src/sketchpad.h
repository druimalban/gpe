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
#ifndef SKETCHPAD_H
#define SKETCHPAD_H

#include <gtk/gtk.h>


typedef struct _sketchpad {
  //GtkWidget * scrollable_drawing_area;
  //GtkWidget * drawing_area;

  //GtkWidget * files_toolbar;
  GtkWidget * files_popup_button;

  //GtkWidget * drawing_toolbar;
  GtkWidget * button_tools_pencil;
  GtkWidget * button_tools_eraser;
  //GtkWidget * brush_selection_button;
  GtkWidget * button_brush_small;
  GtkWidget * button_brush_medium;
  GtkWidget * button_brush_large;
  GtkWidget * button_brush_xlarge;
  //GtkWidget * color_selection_button;
  GtkWidget * button_color_blue  ;
  GtkWidget * button_color_green ;
  GtkWidget * button_color_red   ;
  GtkWidget * button_color_black ;
} Sketchpad;
extern Sketchpad sketchpad;

extern GtkWidget * drawing_area;

extern gboolean is_current_sketch_modified;

void sketchpad_init();
void window_sketchpad_init(GtkWidget * window_sketchpad);

void reset_drawing_area(void);

void sketchpad_set_drawing_area(GtkWidget * a_drawing_area);
void sketchpad_refresh_drawing_area(void);
void sketchpad_set_current_sketch_from_pixbuf(GdkPixbuf * pixbuf);
GdkPixbuf * sketchpad_get_current_sketch_pixbuf();

void sketchpad_open_file(gchar * fullpath_filename);
void sketchpad_new_sketch();
void sketchpad_reset_title();
void sketchpad_import_image(void);
void sketchpad_expand(GtkAdjustment *adj, gboolean isHoriz, int numPix);

//--drawing functions
void draw_point(gdouble x, gdouble y);
void draw_line (gdouble x1, gdouble y1, gdouble x2, gdouble y2);

//--drawing area
extern GdkPixmap * drawing_area_pixmap_buffer;
extern gint drawing_area_width ;
extern gint drawing_area_height;

//--pointer
extern gint prev_pos_x;
extern gint prev_pos_y;
#define NO_PREV_POS -1

//--toolbar
extern gint tool;
#define PEN    1
#define ERASER 2

extern gint brush; //value = diameter of the brush (pixels)
#define SMALL  0   // 0 means 'minimum' (gdk uses a specific algo)
#define MEDIUM 2
#define LARGE  5
#define XLARGE 20

extern GdkColormap * colormap;
extern GdkColor * current_color;
extern GdkColor white;
extern GdkColor black;
extern GdkColor red;
extern GdkColor green;
extern GdkColor blue;
extern GdkColor yellow;
extern GdkColor orange;

//void sketchpad_set_tool    (gint    tool);
//void sketchpad_set_brush   (gint    brush);
void sketchpad_set_tool_s  (gchar * tool);
void sketchpad_set_brush_s (gchar * brush);

// Initial sketch sizes
#define SKETCH_WIDTH 320
#define SKETCH_HEIGHT 320

#endif
