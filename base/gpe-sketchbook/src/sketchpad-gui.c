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
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gpe-sketchbook.h"
#include "sketchpad.h"
#include "sketchpad-gui.h"
#include "sketchpad-cb.h"

#include "dock.h"
#include "gpe/pixmaps.h"
#include "gpe/render.h"
#include "gpe/popup_menu.h"
#include "gpe/picturebutton.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

GtkWidget * sketchpad_build_scrollable_drawing_area(gint width, gint height);
GtkWidget * sketchpad_build_drawing_toolbar(GtkWidget * window);
GtkWidget * sketchpad_build_files_toolbar(GtkWidget * window);
void        sketchpad_fill_files_toolbar(GtkWidget * toolbar, GtkWidget * window);

/**
 * Builds a top level window, integrating:
 * - a drawing area,
 * - a files toolbar,
 * - a drawing toolbar
 * returns the window.
 **/
GtkWidget * sketchpad_build_window(){
  GtkWidget * window_sketchpad;
  GtkWidget * sketchpad;

  window_sketchpad = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window_sketchpad), 240, 280);
  gtk_window_set_position     (GTK_WINDOW (window_sketchpad), GTK_WIN_POS_CENTER);
#endif
  gtk_signal_connect (GTK_OBJECT (window_sketchpad), "delete_event",
                      GTK_SIGNAL_FUNC (on_window_sketchpad_destroy), NULL);

  sketchpad = sketchpad_gui(window_sketchpad);
  gtk_container_add (GTK_CONTAINER (window_sketchpad), sketchpad);

  return window_sketchpad;
}

GtkWidget * sketchpad_gui(GtkWidget * toplevel_window){
  GpeDock   * dock;

  GtkWidget * scrollable_drawing_area;
  GtkWidget * drawing_toolbar;
  //GtkWidget * files_toolbar; temporarily disabled (all buttons in drawing_toolbar)

  dock = gpe_dock_new();
	gtk_signal_connect (GTK_OBJECT(toplevel_window), "size-allocate",
                      GTK_SIGNAL_FUNC (on_window_size_allocate),
                      dock);

  drawing_toolbar         = sketchpad_build_drawing_toolbar(toplevel_window);
  //files_toolbar           = sketchpad_build_files_toolbar(toplevel_window);
  scrollable_drawing_area = sketchpad_build_scrollable_drawing_area(drawing_area_width,
                                                                    drawing_area_height);

  // Watch for scroll-worthy keypresses
  // Oddly, home/end/pgup/pgdown get to the vscrollbar even without this
  // but those are useless on an ipaq...
  gtk_signal_connect (GTK_OBJECT (toplevel_window), "key-press-event",
                      GTK_SIGNAL_FUNC (on_key_press), scrollable_drawing_area);

  gpe_dock_add_app_content(dock, scrollable_drawing_area);
  gpe_dock_add_toolbar    (dock, (GtkToolbar*)drawing_toolbar, GTK_ORIENTATION_HORIZONTAL);

  gtk_widget_show_all(gpe_dock(dock));
  return gpe_dock(dock);
}

/*
 * Scrollbars (GtkRange) emit an adjust_bounds signal whenever the arrow
 * is clicked or the thumb is drug, before the position is actually moved.
 * If the scrollbar is already at the end, grow the current pixmap.
 */
static gboolean sketchpad_scroll_adjust_bounds(GtkRange *range,
					       gdouble newVal, gpointer data){
  GtkAdjustment *adj = gtk_range_get_adjustment(range);
  gboolean isHoriz = GPOINTER_TO_INT(data);

  // Only make the adjustment if an arrow button was pressed
  // (i.e. we are trying to move step_increment - not dragging the scrollbar).
  // Take rounding errors into account
  #define ROUGHLY_EQUAL(x, y) ((x) - (y) >= -0.02 && (x) - (y) <= 0.02)

  if (adj->value + adj->page_size >= adj->upper
      && ROUGHLY_EQUAL(newVal, adj->value + adj->step_increment)){
    sketchpad_expand(adj, isHoriz, adj->step_increment);
  }
  else if (adj->value <= adj->lower
           && ROUGHLY_EQUAL(newVal, adj->value - adj->step_increment)){
    sketchpad_expand(adj, isHoriz, -adj->step_increment);
  }
  return FALSE;		// keep propagating the signal
}

GtkWidget * sketchpad_build_scrollable_drawing_area(gint width, gint height){
  GtkScrolledWindow *scrolledwindow;
  GtkWidget *drawing_area;

  //--scroled window
  scrolledwindow = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new (NULL, NULL));
  gtk_scrolled_window_set_policy (scrolledwindow,
                                  GTK_POLICY_ALWAYS,
                                  GTK_POLICY_ALWAYS);

  gtk_signal_connect (GTK_OBJECT (scrolledwindow->vscrollbar), "adjust_bounds",
                      GTK_SIGNAL_FUNC (sketchpad_scroll_adjust_bounds),
		      GINT_TO_POINTER(FALSE));
  gtk_signal_connect (GTK_OBJECT (scrolledwindow->hscrollbar), "adjust_bounds",
                      GTK_SIGNAL_FUNC (sketchpad_scroll_adjust_bounds),
		      GINT_TO_POINTER(TRUE));

  //--drawing area
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawing_area, width, height);
  gtk_widget_set_events (drawing_area, 0
                         | GDK_EXPOSURE_MASK
                         | GDK_POINTER_MOTION_MASK
                         | GDK_POINTER_MOTION_HINT_MASK
                         | GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         | GDK_LEAVE_NOTIFY_MASK);

  //--signals connections
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
  gtk_scrolled_window_add_with_viewport(scrolledwindow, drawing_area);

  //--set a ref to the drawing area
  sketchpad_set_drawing_area(drawing_area);

  return GTK_WIDGET(scrolledwindow);
}//sketchpad_build_scrollable_drawing_area()

GtkWidget * brushbox_new();
GtkWidget * colorbox_new();

GtkWidget * sketchpad_build_drawing_toolbar(GtkWidget * window){
  //draw toolbar
  GtkWidget * toolbar;

  GdkPixbuf * pixbuf;
  GtkWidget * pixmap;

  //tools
  GSList    * tool_group;
  GtkWidget * radiobutton_tool_eraser;
  GtkWidget * radiobutton_tool_pencil;

  //brushes
  GtkWidget * brushbox;
  GtkWidget * button_brushes;
  GtkWidget * button_brushes_pixmap;

  //colors
  GtkWidget * colorbox;
  GtkWidget * button_colors;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  //gtk_toolbar_set_space_size    (GTK_TOOLBAR (toolbar), 0);

  /**///NOTE: use temporarily a single toolbar, until the dock may host several
  /**/sketchpad_fill_files_toolbar (toolbar, window);//filetools
  /**/gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

#define TOSTRINGBASE(s) #s
#define TOSTRING(s) TOSTRINGBASE(s)
#define MAKE_ITEM_RADIOBUTTON(TYPE, ITEM, SIG_PARAM)  \
  radiobutton_ ##TYPE ##_ ##ITEM    = gtk_radio_button_new (TYPE ##_group);\
  TYPE ##_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_ ##TYPE ##_ ##ITEM));\
  \
  pixbuf = gpe_find_icon (  TOSTRING(TYPE ##_ ##ITEM)  );\
  pixmap = gtk_image_new_from_pixbuf (pixbuf);\
  gtk_button_set_relief (GTK_BUTTON (radiobutton_ ##TYPE ##_ ##ITEM), GTK_RELIEF_NONE);\
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_ ##TYPE ##_ ##ITEM), FALSE);\
  \
  gtk_container_add (GTK_CONTAINER (radiobutton_ ##TYPE ##_ ##ITEM), pixmap);\
  \
  gtk_signal_connect (GTK_OBJECT (radiobutton_ ##TYPE ##_ ##ITEM), "clicked",\
                      GTK_SIGNAL_FUNC (on_radiobutton_ ##TYPE ##_clicked), SIG_PARAM);\
  //**/g_printerr("--> %s\n", TOSTRING(TYPE ##_group));


  //--toolbox
  tool_group = NULL;
  MAKE_ITEM_RADIOBUTTON (tool, pencil, "pencil");
  MAKE_ITEM_RADIOBUTTON (tool, eraser, "eraser");

  //keep a ref to set default //FIXME: get default from preference (when implemented!)
  sketchpad.button_tools_pencil  = radiobutton_tool_pencil;
  sketchpad.button_tools_eraser  = radiobutton_tool_eraser;

  //--brushbox
  button_brushes = gtk_button_new();
  gtk_button_set_relief (GTK_BUTTON (button_brushes), GTK_RELIEF_NONE);
  gtk_widget_set_usize (button_brushes,  20, 20);
  pixbuf = gpe_find_icon ("brush_medium");//default value.
  button_brushes_pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_object_set_data((GtkObject *) button_brushes, "pixmap", button_brushes_pixmap);
  gtk_container_add (GTK_CONTAINER (button_brushes), button_brushes_pixmap);

  brushbox = brushbox_new();
  //**/gtk_window_set_transient_for(GTK_WINDOW(brushbox), GTK_WINDOW(window));
  gtk_object_set_data((GtkObject *) brushbox, "calling_button", button_brushes);
  gtk_signal_connect (GTK_OBJECT (button_brushes), "clicked",
                      GTK_SIGNAL_FUNC (on_button_brushes_clicked), brushbox);

  //--colorbox
  button_colors = gtk_button_new();
  gtk_button_set_relief (GTK_BUTTON (button_colors), GTK_RELIEF_NONE);
  gtk_widget_set_usize (button_colors,  20, 20);
  colorbox_button_set_color(button_colors, &black);//default color

  colorbox = colorbox_new();
  //**/gtk_window_set_transient_for(GTK_WINDOW(colorbox), GTK_WINDOW(window));
  gtk_object_set_data((GtkObject *) colorbox, "calling_button", button_colors);
  gtk_signal_connect (GTK_OBJECT (button_colors), "clicked",
                      GTK_SIGNAL_FUNC (on_button_colors_clicked), colorbox);

  //--packing
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar),
                            radiobutton_tool_eraser, _("Select eraser"), _("Select eraser"));
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar),
                            radiobutton_tool_pencil, _("Select pencil"), _("Select pencil"));
  //gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar), button_brushes, _("Brush size"), _("Brush size"));
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar), button_colors,  _("Brush color"), _("Brush color"));

  return toolbar;
}//sketchpad_build_drawing_toolbar()


void colorbox_button_set_color(GtkWidget * button, GdkColor * color){
  GtkWidget * event_box;
  GtkStyle  * style;

  if(GTK_BIN (button)->child == NULL){
    event_box = gtk_event_box_new();
    gtk_container_add (GTK_CONTAINER (button), event_box);
  }
  else{
    event_box = GTK_BIN (button)->child;
  }
  style = gtk_style_copy(event_box->style);
  style->bg[0] = * color;
  style->bg[1] = * color;
  style->bg[2] = * color;
  style->bg[3] = * color;
  style->bg[4] = * color;
  gtk_widget_set_style(event_box, style);
  gtk_widget_show_now(event_box);
}

GtkWidget * _popupwindow (GtkWidget * widget){
  GtkWidget * popup_window;
  GtkWidget * frame;

  popup_window = gtk_window_new(GTK_WINDOW_POPUP);
  //gtk_window_set_modal   (GTK_WINDOW (popup_window), TRUE);
  gtk_window_set_position(GTK_WINDOW (popup_window), GTK_WIN_POS_MOUSE);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
  gtk_container_add (GTK_CONTAINER (popup_window), frame);

  gtk_container_add (GTK_CONTAINER (frame), widget);

  return popup_window;
}//_popupwindow()

GtkWidget * brushbox_new(){
  GtkWidget * brushbox;
  GtkWidget * window;//dummy alias to brushbox (for the generic macro (...))

  GtkWidget * table_brushes;
  GSList    * brush_group;
  GtkWidget * radiobutton_brush_medium;
  GtkWidget * radiobutton_brush_large; 
  GtkWidget * radiobutton_brush_xlarge;
  GtkWidget * radiobutton_brush_small; 

  GdkPixbuf * pixbuf;
  GtkWidget * pixmap;

  table_brushes = gtk_table_new (2, 2, FALSE);
  brushbox = _popupwindow (table_brushes);
  window = brushbox;

  brush_group = NULL;
  MAKE_ITEM_RADIOBUTTON (brush, small , "small" );
  MAKE_ITEM_RADIOBUTTON (brush, medium, "medium");
  MAKE_ITEM_RADIOBUTTON (brush, large , "large" );
  MAKE_ITEM_RADIOBUTTON (brush, xlarge, "xlarge");

#define PACK_BRUSH_RADIOBUTTON(size, x, y, a, b) \
  gtk_table_attach (GTK_TABLE (table_brushes), radiobutton_brush_ ##size, x, y, a, b, \
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);

  PACK_BRUSH_RADIOBUTTON (small,  0, 1, 1, 2);
  PACK_BRUSH_RADIOBUTTON (medium, 0, 1, 0, 1);
  PACK_BRUSH_RADIOBUTTON (large,  1, 2, 0, 1);
  PACK_BRUSH_RADIOBUTTON (xlarge, 1, 2, 1, 2);

  //keep a ref to set default //FIXME: get default from preference (when implemented!)
  sketchpad.button_brush_small  = radiobutton_brush_small;
  sketchpad.button_brush_medium = radiobutton_brush_medium;
  sketchpad.button_brush_large  = radiobutton_brush_large;
  sketchpad.button_brush_xlarge = radiobutton_brush_xlarge;

  return brushbox;
}//brushbox_new()

GtkWidget * colorbox_new(){
  GtkWidget * colorbox;

  GtkWidget * table_colors;
  GSList    * color_group;
  GtkWidget * radiobutton_color_blue; 
  GtkWidget * radiobutton_color_green;
  GtkWidget * radiobutton_color_red;  
  GtkWidget * radiobutton_color_black;

  table_colors = gtk_table_new (2, 2, FALSE);
  colorbox = _popupwindow (table_colors);

  //NOTE: as window close as soon as one is clicked, normal buttons are enough
#define MAKE_COLOR_RADIOBUTTON(color, x, y, a, b)  \
  radiobutton_color_ ##color = gtk_radio_button_new (color_group);\
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_ ##color));\
  \
  gtk_widget_set_usize (radiobutton_color_ ##color, 20, 20);\
  colorbox_button_set_color(radiobutton_color_ ##color, &color);\
  gtk_button_set_relief (GTK_BUTTON (radiobutton_color_ ##color), GTK_RELIEF_NONE);\
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_ ##color), FALSE);\
  \
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_ ##color), "clicked",\
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked), &color);\
  \
  gtk_table_attach (GTK_TABLE (table_colors), radiobutton_color_ ##color, x, y, a, b, \
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);

  color_group = NULL;
  MAKE_COLOR_RADIOBUTTON (black, 0, 1, 0, 1);
  MAKE_COLOR_RADIOBUTTON (red  , 0, 1, 1, 2);
  MAKE_COLOR_RADIOBUTTON (green, 1, 2, 1, 2);
  MAKE_COLOR_RADIOBUTTON (blue , 1, 2, 0, 1);

  //keep a ref to set default //FIXME: get default from preference (when implemented!)
  sketchpad.button_color_blue  = radiobutton_color_blue; 
  sketchpad.button_color_green = radiobutton_color_green;
  sketchpad.button_color_red   = radiobutton_color_red;  
  sketchpad.button_color_black = radiobutton_color_black;

  return colorbox;
}//colorbox_new()

GtkWidget * sketchpad_build_files_toolbar(GtkWidget * window){
  //file toolbar
  GtkWidget * toolbar;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  //gtk_toolbar_set_space_size    (GTK_TOOLBAR (toolbar), 0);

  sketchpad_fill_files_toolbar(toolbar, window);

  return toolbar;
}//sketchpad_build_files_toolbar()


GtkWidget * _files_popup_new (GtkWidget *parent_button);

void sketchpad_fill_files_toolbar(GtkWidget * toolbar, GtkWidget * window){
  GdkPixbuf *pixbuf;
  GtkWidget *pixmap;

  GtkWidget * files_popup_button;

  pixbuf = gpe_find_icon ("new");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  files_popup_button = popup_menu_button_new (pixmap, _files_popup_new, NULL/* callback func */);
  //files_popup_button = popup_menu_button_new_from_stock (GTK_STOCK_NEW, _files_popup_new, NULL);
  gtk_button_set_relief (GTK_BUTTON (files_popup_button), GTK_RELIEF_NONE);
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar),
                            files_popup_button, _("Sketch menu"), _("Sketch menu"));
  //keep a ref
  sketchpad.files_popup_button = files_popup_button;

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  pixbuf = gpe_find_icon ("list");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("Show list"), _("Show list"),
                           pixmap, GTK_SIGNAL_FUNC(on_button_list_view_clicked), NULL);
  pixbuf = gpe_find_icon ("left");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("Previous sketch"), _("Previous sketch"),
                           pixmap, GTK_SIGNAL_FUNC(on_button_file_prev_clicked), NULL);
  pixbuf = gpe_find_icon ("right");
  pixmap = gtk_image_new_from_pixbuf (pixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), NULL,
                           _("Next sketch"), _("Next sketch"),
                           pixmap, GTK_SIGNAL_FUNC(on_button_file_next_clicked), NULL);

}

GtkWidget * _files_popup_new (GtkWidget *parent_button){
  GtkWidget *vbox;//to return

  GtkWidget * button_new;
  GtkWidget * button_import;
  GtkWidget * button_save;
  GtkWidget * button_delete;
  //GtkWidget * button_properties;
  GtkWidget * button_exit;

  GtkStyle * style = sketchbook.window->style;

  button_new        = gpe_picture_button_aligned (style, _("New"), "new", GPE_POS_LEFT);
  button_import     = gpe_picture_button_aligned (style, _("Import"), "import", GPE_POS_LEFT);
  button_save       = gpe_picture_button_aligned (style, _("Save"), "save", GPE_POS_LEFT);
  button_delete     = gpe_picture_button_aligned (style, _("Delete"), "delete", GPE_POS_LEFT);
  //button_properties = gpe_picture_button_aligned (style, _("Properties"), "properties", GPE_POS_LEFT);
  button_exit       = gpe_picture_button_aligned (style, _("Exit"),   "exit",   GPE_POS_LEFT);

#define _BUTTON_SETUP(action) \
              gtk_button_set_relief (GTK_BUTTON (button_ ##action), GTK_RELIEF_NONE);\
              g_signal_connect (G_OBJECT (button_ ##action), "clicked",\
              G_CALLBACK (on_button_file_ ##action ##_clicked), NULL);

  _BUTTON_SETUP(new);
  _BUTTON_SETUP(import);
  _BUTTON_SETUP(save);
  _BUTTON_SETUP(delete);
  //_BUTTON_SETUP(properties);
  _BUTTON_SETUP(exit);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_new,        FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_save,       FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_delete,     FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_import,     FALSE, FALSE, 0);
  //gtk_box_pack_start (GTK_BOX (vbox), button_properties, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_exit,     FALSE, FALSE, 0);

  return vbox;
}
