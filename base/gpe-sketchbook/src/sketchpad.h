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

extern GtkWidget * window_sketchpad;
extern GtkWidget * drawing_area;

#define SKETCHPAD_TITLE_NEW "new"

void sketchpad_init();
void window_sketchpad_init(GtkWidget * window_sketchpad);

//--event handlers //FIXME: to move to sketchpad-cb
gint configure_event_handler     (GtkWidget *_drawing_area, GdkEventConfigure *event);
gint expose_event_handler        (GtkWidget *_drawing_area, GdkEventExpose *event);
gint button_press_event_handler  (GtkWidget *_drawing_area, GdkEventButton *event);
gint button_release_event_handler(GtkWidget *_drawing_area, GdkEventButton *event );
gint motion_notify_event_handler (GtkWidget *_drawing_area, GdkEventMotion *event);

void sketchpad_refresh_drawing_area();

void sketchpad_open_file(gchar * fullpath_filename, const gchar * title);
void sketchpad_new_sketch();
void sketchpad_set_title(const gchar * name);

//--drawing functions
void draw_brush(gdouble x, gdouble y);
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
extern gint color;
#define WHITE  0
#define BLACK  1
#define RED    3
#define GREEN  4
#define BLUE   5
#define YELLOW 6
extern gint brush;
#define SMALL  0
#define MEDIUM 2
#define LARGE  5
#define XLARGE 20

extern GdkColormap * colormap;
extern GdkColor white;
extern GdkColor black;
extern GdkColor red;
extern GdkColor green;
extern GdkColor blue;

//void sketchpad_set_tool    (gint    tool);
//void sketchpad_set_color   (gint    color);
//void sketchpad_set_brush   (gint    brush);
void sketchpad_set_tool_s  (gchar * tool);
void sketchpad_set_color_s (gchar * color);
void sketchpad_set_brush_s (gchar * brush);

#endif
