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

#include "sketchpad.h"
#include "sketchpad-gui.h"
#include "sketchpad-cb.h"

#include "gpe/pixmaps.h"
#include "gpe/render.h"

GtkWidget * sketchpad_build_scrollable_drawing_area(gint width, gint height);
GtkWidget * sketchpad_build_drawing_toolbar(GtkWidget * window);
GtkWidget * sketchpad_build_files_toolbar(GtkWidget * window);

/**
 * Builds a top level window, integrating:
 * - a drawing area,
 * - a files toolbar,
 * - a drawing toolbar
 * returns the window.
 **/
GtkWidget * sketchpad_build_window(){
  GtkWidget * window_sketchpad;

  GtkWidget * scrollable_drawing_area;
  GtkWidget * drawing_toolbar;
  GtkWidget * files_toolbar;

  GtkWidget * vbox;
  GtkWidget * hbox_toolbar;

  //--building
  window_sketchpad = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window_sketchpad), 240, 280);
#endif
  gtk_signal_connect (GTK_OBJECT (window_sketchpad), "destroy",
                      GTK_SIGNAL_FUNC (on_window_sketchpad_destroy),
                      NULL);

  drawing_toolbar         = sketchpad_build_drawing_toolbar(window_sketchpad);
  files_toolbar           = sketchpad_build_files_toolbar(window_sketchpad);
  scrollable_drawing_area = sketchpad_build_scrollable_drawing_area(drawing_area_width,
                                                                    drawing_area_height);

  //--packing
  hbox_toolbar = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_toolbar), files_toolbar,        FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_toolbar), gtk_vseparator_new(), FALSE, FALSE, 2);
  gtk_box_pack_end   (GTK_BOX (hbox_toolbar), drawing_toolbar,      FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox_toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrollable_drawing_area, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window_sketchpad), vbox);

  gtk_widget_show_all(vbox);
  return window_sketchpad;
}//create_window_sketchpad()

GtkWidget * sketchpad_build_scrollable_drawing_area(gint width, gint height){
  GtkWidget *scrolledwindow;
  GtkWidget *drawing_area;

  //--scroled window
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  //--drawing area
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawing_area, width, height);
  gtk_widget_set_events (drawing_area,
                         GDK_EXPOSURE_MASK
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
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwindow),
                                        drawing_area);

  //--set a ref to the drawing area
  sketchpad_set_drawing_area(drawing_area);

  return scrolledwindow;
}//sketchpad_build_scrollable_drawing_area()


GtkWidget * sketchpad_build_drawing_toolbar(GtkWidget * window){
  //draw toolbar
  GtkWidget * hbox_drawtools;

  GdkPixbuf *pixbuf;
  GtkWidget *pixmap;

  //tools
  GSList *tool_group;
  GtkWidget *radiobutton_tools_eraser;
  GtkWidget *radiobutton_tools_pen;   

  //brushes
  GSList *brush_group;
  GtkWidget *radiobutton_brush_medium;
  GtkWidget *radiobutton_brush_large; 
  GtkWidget *radiobutton_brush_xlarge;
  GtkWidget *radiobutton_brush_small; 
  GtkWidget *table2;

  //colors
  GSList *color_group;
  GtkWidget *radiobutton_color_blue; 
  GtkWidget *radiobutton_color_green;
  GtkWidget *radiobutton_color_red;  
  GtkWidget *radiobutton_color_black;
  GtkWidget *table1;

  //--tools
  tool_group = NULL;
  radiobutton_tools_pen    = gtk_radio_button_new (tool_group);
  tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_tools_pen));
  radiobutton_tools_eraser = gtk_radio_button_new (tool_group);
  tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_tools_eraser));

  //pixmaps
  pixbuf = gpe_find_icon ("eraser");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_tools_eraser), pixmap);
  pixbuf = gpe_find_icon ("pencil");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_tools_pen), pixmap);

  //--brushes
  brush_group = NULL;
  radiobutton_brush_small  = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_small));
  radiobutton_brush_medium = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_medium));
  radiobutton_brush_large  = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_large));
  radiobutton_brush_xlarge = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_xlarge));

  gtk_widget_set_usize (radiobutton_brush_small,  10, 10);
  gtk_widget_set_usize (radiobutton_brush_medium, 10, 10);
  gtk_widget_set_usize (radiobutton_brush_large,  10, 10);
  gtk_widget_set_usize (radiobutton_brush_xlarge, 10, 10);

  //pixmaps
  pixbuf = gpe_find_icon ("brush_small");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_small), pixmap);
  pixbuf = gpe_find_icon ("brush_medium");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_medium), pixmap);
  pixbuf = gpe_find_icon ("brush_large");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_large), pixmap);
  pixbuf = gpe_find_icon ("brush_xlarge");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_xlarge), pixmap);

  //pre-packing
  table2 = gtk_table_new (2, 2, FALSE);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_small, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_medium, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_large, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_xlarge, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);

  //--colors
  color_group = NULL;
  radiobutton_color_black = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_black));
  radiobutton_color_red   = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_red));
  radiobutton_color_green = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_green));
  radiobutton_color_blue  = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_blue));

  gtk_widget_set_usize (radiobutton_color_black, 10, 10);
  //what's about the others? ...

  //pixmaps
  pixbuf = gpe_find_icon ("color_black");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_black), pixmap);
  pixbuf = gpe_find_icon ("color_red");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_red), pixmap);
  pixbuf = gpe_find_icon ("color_green");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_green), pixmap);
  pixbuf = gpe_find_icon ("color_blue");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_blue), pixmap);

  //pre-packing
  table1 = gtk_table_new (2, 2, FALSE);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_blue, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_green, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_red, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_black, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

  //signals connection
  gtk_signal_connect (GTK_OBJECT (radiobutton_tools_eraser), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_tool_clicked), "eraser");
  gtk_signal_connect (GTK_OBJECT (radiobutton_tools_pen), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_tool_clicked), "pen");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_medium), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked), "medium");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_large), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked), "large");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_xlarge), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked), "xlarge");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_small), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked), "small");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_black), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked), "black");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_red), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked), "red");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_green), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked), "green");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_blue), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked), "blue");

  //don't show the toggle button
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_tools_pen),    FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_tools_eraser), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_small),  FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_medium), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_large),  FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_xlarge), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_black),  FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_red),    FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_green),  FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_blue),   FALSE);

  //no relief
  gtk_button_set_relief (GTK_BUTTON (radiobutton_tools_pen),    GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_tools_eraser), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_brush_small),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_brush_medium), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_brush_large),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_brush_xlarge), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_color_black),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_color_red),    GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_color_green),  GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (radiobutton_color_blue),   GTK_RELIEF_NONE);

  //--default tools //NOTE: maybe a preference
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_tools_pen),    TRUE);
  //FIXME: segfault is something else than "small" ...
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_brush_small), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_color_black),  TRUE);

  //--packing
  hbox_drawtools = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), radiobutton_tools_eraser, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), radiobutton_tools_pen,    FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), gtk_vseparator_new(),     FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), table2,                   FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), gtk_vseparator_new(),     FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), table1,                   FALSE, FALSE, 0);

  return hbox_drawtools;
}//sketchpad_build_drawing_toolbar()

GtkWidget * sketchpad_build_files_toolbar(GtkWidget * window){
  //file toolbar
  GtkWidget *hbox_filetools;

  GdkPixbuf *pixbuf;
  GtkWidget *pixmap;

  GtkWidget *button_file_save;  
  GtkWidget *button_file_new;   
  GtkWidget *button_file_delete;
  GtkWidget *button_file_prev;  
  GtkWidget *button_file_next;  
  GtkWidget *button_list_view;  

  //buttons
  button_file_save   = gtk_button_new ();
  button_file_new    = gtk_button_new ();
  button_file_delete = gtk_button_new ();
  button_file_prev   = gtk_button_new ();
  button_file_next   = gtk_button_new ();
  button_list_view   = gtk_button_new ();

  //pixmaps
  pixbuf = gpe_find_icon ("save");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_file_save), pixmap);
  pixbuf = gpe_find_icon ("new");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_file_new), pixmap);
  pixbuf = gpe_find_icon ("delete");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_file_delete), pixmap);
  pixbuf = gpe_find_icon ("left");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_file_prev), pixmap);
  pixbuf = gpe_find_icon ("right");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_file_next), pixmap);
  pixbuf = gpe_find_icon ("list");
  pixmap = gpe_render_icon (window->style, pixbuf);
  gtk_container_add (GTK_CONTAINER (button_list_view), pixmap);

  //no relief
  gtk_button_set_relief (GTK_BUTTON (button_file_save),   GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_file_new),    GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_file_delete), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_file_prev),   GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_file_next),   GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (button_list_view),   GTK_RELIEF_NONE);

  //--signals connection
  gtk_signal_connect (GTK_OBJECT (button_file_save), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_save_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_new), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_new_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_delete), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_delete_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_prev), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_prev_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_next), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_next_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (button_list_view), "clicked",
                      GTK_SIGNAL_FUNC (on_button_list_view_clicked), NULL);

  //--packing
  hbox_filetools = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_save,   FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_new,    FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_delete, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_prev,   FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_next,   FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_list_view,   FALSE, FALSE, 0);

  return hbox_filetools;
}//sketchpad_build_files_toolbar()
