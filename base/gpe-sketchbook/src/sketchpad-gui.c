#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "sketchpad-gui.h"
#include "sketchpad-cb.h"

#include "_support.h"

GtkWidget*
create_window_sketchpad (void)
{
  GtkWidget *window_sketchpad;
  GtkWidget *vbox;
  GtkWidget *hbox_sketchpad_toolbar;
  GtkWidget *hbox_filetools;
  GtkWidget *button_file_save;
  GtkWidget *pixmap10;
  GtkWidget *button_file_new;
  GtkWidget *pixmap12;
  GtkWidget *button_file_delete;
  GtkWidget *pixmap24;
  GtkWidget *button_file_prev;
  GtkWidget *pixmap23;
  GtkWidget *button_file_next;
  GtkWidget *pixmap22;
  GtkWidget *button_list_view;
  GtkWidget *pixmap15;
  GtkWidget *vseparator4;
  GtkWidget *hbox_drawtools;
  GSList *tool_group = NULL;
  GtkWidget *radiobutton_tools_eraser;
  GtkWidget *pixmap7;
  GtkWidget *radiobutton_tools_pen;
  GtkWidget *pixmap5;
  GtkWidget *vseparator3;
  GtkWidget *table2;
  GSList *brush_group = NULL;
  GtkWidget *radiobutton_brush_medium;
  GtkWidget *pixmap8;
  GtkWidget *radiobutton_brush_large;
  GtkWidget *pixmap25;
  GtkWidget *radiobutton_brush_xlarge;
  GtkWidget *pixmap26;
  GtkWidget *radiobutton_brush_small;
  GtkWidget *pixmap6;
  GtkWidget *vseparator2;
  GtkWidget *table1;
  GSList *color_group = NULL;
  GtkWidget *radiobutton_color_blue;
  GtkWidget *pixmap2;
  GtkWidget *radiobutton_color_green;
  GtkWidget *pixmap4;
  GtkWidget *radiobutton_color_red;
  GtkWidget *pixmap14;
  GtkWidget *radiobutton_color_black;
  GtkWidget *pixmap1;
  GtkWidget *scrolledwindow_drawing_area;
  GtkWidget *viewport_drawing_area;
  GtkWidget *drawing_area;

  window_sketchpad = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window_sketchpad), "window_sketchpad", window_sketchpad);
  gtk_window_set_default_size (GTK_WINDOW (window_sketchpad), 240, 280);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "vbox", vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window_sketchpad), vbox);

  hbox_sketchpad_toolbar = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_sketchpad_toolbar);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "hbox_sketchpad_toolbar", hbox_sketchpad_toolbar,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_sketchpad_toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), hbox_sketchpad_toolbar, FALSE, FALSE, 0);

  hbox_filetools = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_filetools);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "hbox_filetools", hbox_filetools,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_filetools);
  gtk_box_pack_start (GTK_BOX (hbox_sketchpad_toolbar), hbox_filetools, FALSE, FALSE, 0);

  button_file_save = gtk_button_new ();
  gtk_widget_ref (button_file_save);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "button_file_save", button_file_save,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_file_save);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_save, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_file_save), GTK_RELIEF_NONE);

  pixmap10 = create_pixmap (window_sketchpad, "gsave.xpm");
  gtk_widget_ref (pixmap10);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap10", pixmap10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap10);
  gtk_container_add (GTK_CONTAINER (button_file_save), pixmap10);

  button_file_new = gtk_button_new ();
  gtk_widget_ref (button_file_new);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "button_file_new", button_file_new,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_file_new);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_new, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_file_new), GTK_RELIEF_NONE);

  pixmap12 = create_pixmap (window_sketchpad, "gnew.xpm");
  gtk_widget_ref (pixmap12);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap12", pixmap12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap12);
  gtk_container_add (GTK_CONTAINER (button_file_new), pixmap12);

  button_file_delete = gtk_button_new ();
  gtk_widget_ref (button_file_delete);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "button_file_delete", button_file_delete,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_file_delete);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_delete, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_file_delete), GTK_RELIEF_NONE);

  pixmap24 = create_pixmap (window_sketchpad, "gdelete.xpm");
  gtk_widget_ref (pixmap24);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap24", pixmap24,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap24);
  gtk_container_add (GTK_CONTAINER (button_file_delete), pixmap24);

  button_file_prev = gtk_button_new ();
  gtk_widget_ref (button_file_prev);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "button_file_prev", button_file_prev,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_file_prev);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_prev, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_file_prev), GTK_RELIEF_NONE);

  pixmap23 = create_pixmap (window_sketchpad, "gprev.xpm");
  gtk_widget_ref (pixmap23);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap23", pixmap23,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap23);
  gtk_container_add (GTK_CONTAINER (button_file_prev), pixmap23);

  button_file_next = gtk_button_new ();
  gtk_widget_ref (button_file_next);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "button_file_next", button_file_next,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_file_next);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_file_next, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_file_next), GTK_RELIEF_NONE);

  pixmap22 = create_pixmap (window_sketchpad, "gnext.xpm");
  gtk_widget_ref (pixmap22);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap22", pixmap22,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap22);
  gtk_container_add (GTK_CONTAINER (button_file_next), pixmap22);

  button_list_view = gtk_button_new ();
  gtk_widget_ref (button_list_view);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "button_list_view", button_list_view,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_list_view);
  gtk_box_pack_start (GTK_BOX (hbox_filetools), button_list_view, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button_list_view), GTK_RELIEF_NONE);

  pixmap15 = create_pixmap (window_sketchpad, "glist.xpm");
  gtk_widget_ref (pixmap15);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap15", pixmap15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap15);
  gtk_container_add (GTK_CONTAINER (button_list_view), pixmap15);

  vseparator4 = gtk_vseparator_new ();
  gtk_widget_ref (vseparator4);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "vseparator4", vseparator4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vseparator4);
  gtk_box_pack_start (GTK_BOX (hbox_sketchpad_toolbar), vseparator4, FALSE, FALSE, 2);

  hbox_drawtools = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_drawtools);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "hbox_drawtools", hbox_drawtools,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_drawtools);
  gtk_box_pack_end (GTK_BOX (hbox_sketchpad_toolbar), hbox_drawtools, FALSE, FALSE, 0);

  radiobutton_tools_eraser = gtk_radio_button_new (tool_group);
  tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_tools_eraser));
  gtk_widget_ref (radiobutton_tools_eraser);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_tools_eraser", radiobutton_tools_eraser,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_tools_eraser);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), radiobutton_tools_eraser, FALSE, FALSE, 0);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_tools_eraser), FALSE);

  pixmap7 = create_pixmap (window_sketchpad, "geraser.xpm");
  gtk_widget_ref (pixmap7);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap7", pixmap7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap7);
  gtk_container_add (GTK_CONTAINER (radiobutton_tools_eraser), pixmap7);

  radiobutton_tools_pen = gtk_radio_button_new (tool_group);
  tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_tools_pen));
  gtk_widget_ref (radiobutton_tools_pen);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_tools_pen", radiobutton_tools_pen,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_tools_pen);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), radiobutton_tools_pen, FALSE, FALSE, 0);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_tools_pen), FALSE);

  pixmap5 = create_pixmap (window_sketchpad, "gpencil.xpm");
  gtk_widget_ref (pixmap5);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap5", pixmap5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap5);
  gtk_container_add (GTK_CONTAINER (radiobutton_tools_pen), pixmap5);

  vseparator3 = gtk_vseparator_new ();
  gtk_widget_ref (vseparator3);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "vseparator3", vseparator3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vseparator3);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), vseparator3, FALSE, FALSE, 2);

  table2 = gtk_table_new (2, 2, FALSE);
  gtk_widget_ref (table2);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "table2", table2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table2);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), table2, FALSE, FALSE, 0);

  radiobutton_brush_medium = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_medium));
  gtk_widget_ref (radiobutton_brush_medium);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_brush_medium", radiobutton_brush_medium,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_brush_medium);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_medium, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_widget_set_usize (radiobutton_brush_medium, 10, 10);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_medium), FALSE);

  pixmap8 = create_pixmap (window_sketchpad, "brush_medium.xpm");
  gtk_widget_ref (pixmap8);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap8", pixmap8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap8);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_medium), pixmap8);

  radiobutton_brush_large = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_large));
  gtk_widget_ref (radiobutton_brush_large);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_brush_large", radiobutton_brush_large,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_brush_large);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_large, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_widget_set_usize (radiobutton_brush_large, 10, 10);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_large), FALSE);

  pixmap25 = create_pixmap (window_sketchpad, "brush_large.xpm");
  gtk_widget_ref (pixmap25);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap25", pixmap25,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap25);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_large), pixmap25);

  radiobutton_brush_xlarge = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_xlarge));
  gtk_widget_ref (radiobutton_brush_xlarge);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_brush_xlarge", radiobutton_brush_xlarge,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_brush_xlarge);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_xlarge, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_widget_set_usize (radiobutton_brush_xlarge, 10, 10);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_xlarge), FALSE);

  pixmap26 = create_pixmap (window_sketchpad, "brush_xlarge.xpm");
  gtk_widget_ref (pixmap26);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap26", pixmap26,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap26);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_xlarge), pixmap26);

  radiobutton_brush_small = gtk_radio_button_new (brush_group);
  brush_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_brush_small));
  gtk_widget_ref (radiobutton_brush_small);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_brush_small", radiobutton_brush_small,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_brush_small);
  gtk_table_attach (GTK_TABLE (table2), radiobutton_brush_small, 0, 1, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (radiobutton_brush_small, 10, 10);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_brush_small), FALSE);

  pixmap6 = create_pixmap (window_sketchpad, "brush_small.xpm");
  gtk_widget_ref (pixmap6);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap6", pixmap6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap6);
  gtk_container_add (GTK_CONTAINER (radiobutton_brush_small), pixmap6);

  vseparator2 = gtk_vseparator_new ();
  gtk_widget_ref (vseparator2);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "vseparator2", vseparator2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vseparator2);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), vseparator2, FALSE, FALSE, 2);

  table1 = gtk_table_new (2, 2, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (hbox_drawtools), table1, FALSE, FALSE, 0);

  radiobutton_color_blue = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_blue));
  gtk_widget_ref (radiobutton_color_blue);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_color_blue", radiobutton_color_blue,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_color_blue);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_blue, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_blue), FALSE);

  pixmap2 = create_pixmap (window_sketchpad, "color_blue.xpm");
  gtk_widget_ref (pixmap2);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap2", pixmap2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap2);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_blue), pixmap2);

  radiobutton_color_green = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_green));
  gtk_widget_ref (radiobutton_color_green);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_color_green", radiobutton_color_green,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_color_green);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_green, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_green), FALSE);

  pixmap4 = create_pixmap (window_sketchpad, "color_green.xpm");
  gtk_widget_ref (pixmap4);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap4", pixmap4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap4);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_green), pixmap4);

  radiobutton_color_red = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_red));
  gtk_widget_ref (radiobutton_color_red);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_color_red", radiobutton_color_red,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_color_red);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_red, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_red), FALSE);

  pixmap14 = create_pixmap (window_sketchpad, "color_red.xpm");
  gtk_widget_ref (pixmap14);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap14", pixmap14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap14);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_red), pixmap14);

  radiobutton_color_black = gtk_radio_button_new (color_group);
  color_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_color_black));
  gtk_widget_ref (radiobutton_color_black);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "radiobutton_color_black", radiobutton_color_black,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_color_black);
  gtk_table_attach (GTK_TABLE (table1), radiobutton_color_black, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (radiobutton_color_black, 10, 10);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radiobutton_color_black), FALSE);

  pixmap1 = create_pixmap (window_sketchpad, "color_black.xpm");
  gtk_widget_ref (pixmap1);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "pixmap1", pixmap1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pixmap1);
  gtk_container_add (GTK_CONTAINER (radiobutton_color_black), pixmap1);

  scrolledwindow_drawing_area = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_drawing_area);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "scrolledwindow_drawing_area", scrolledwindow_drawing_area,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow_drawing_area);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow_drawing_area, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_drawing_area), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport_drawing_area = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport_drawing_area);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "viewport_drawing_area", viewport_drawing_area,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport_drawing_area);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_drawing_area), viewport_drawing_area);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_ref (drawing_area);
  gtk_object_set_data_full (GTK_OBJECT (window_sketchpad), "drawing_area", drawing_area,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (drawing_area);
  gtk_container_add (GTK_CONTAINER (viewport_drawing_area), drawing_area);
  gtk_widget_set_usize (drawing_area, 200, 200);
  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK);

  gtk_signal_connect (GTK_OBJECT (window_sketchpad), "destroy",
                      GTK_SIGNAL_FUNC (on_window_sketchpad_destroy),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_save), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_save_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_new), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_new_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_delete), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_delete_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_prev), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_prev_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_file_next), "clicked",
                      GTK_SIGNAL_FUNC (on_button_file_next_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_list_view), "clicked",
                      GTK_SIGNAL_FUNC (on_button_list_view_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (radiobutton_tools_eraser), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_tool_clicked),
                      "eraser");
  gtk_signal_connect (GTK_OBJECT (radiobutton_tools_pen), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_tool_clicked),
                      "pen");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_medium), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked),
                      "medium");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_large), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked),
                      "large");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_xlarge), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked),
                      "xlarge");
  gtk_signal_connect (GTK_OBJECT (radiobutton_brush_small), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_brush_clicked),
                      "small");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_blue), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked),
                      "blue");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_green), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked),
                      "green");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_red), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked),
                      "red");
  gtk_signal_connect (GTK_OBJECT (radiobutton_color_black), "clicked",
                      GTK_SIGNAL_FUNC (on_radiobutton_color_clicked),
                      "black");
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

  return window_sketchpad;
}

