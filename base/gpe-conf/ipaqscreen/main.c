/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>, Moray Allan <moray@sermisy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <gtk/gtk.h>

#include "gpe/init.h"

#include "brightness.h"
#include "rotation.h"
#include "callbacks.h"
#include "xset.h"

int initialising = 1;
char *RotationLabels[4]=
  {
    "Portrait",
    "Landscape (left)",
    "Inverted",
    "Landscape (right)"
  };
static struct
{
  GtkWidget *table;
  GtkWidget *hbox2;
  GtkWidget *brightnessl;
  GtkWidget *brightness;
  GtkWidget *screensaverl;
  GtkWidget *screensaverl2;
  GtkWidget *screensaver;
  GtkWidget *screensaverbt;
  GtkWidget *rotation;
  GtkWidget *rotationl;
  GtkWidget *touchscreen;
  GtkWidget *calibrate;
}self;

GtkWidget *ipaqscreen_Build_Objects()
{
  GtkWidget *menu = NULL;
  GtkWidget *glade_menuitem = NULL;
  guint i;
  int ss_sec;

  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  GtkJustification table_justify_right_col;
  guint widget_padding_x;
  guint widget_padding_y_even;
  guint widget_padding_y_odd;

  /* 
   * GTK_EXPAND  the widget should expand to take up any extra space
                 in its container that has been allocated.
   * GTK_SHRINK  the widget should shrink as and when possible.
   * GTK_FILL    the widget should fill the space allocated to it.
   */
  
  /*
   * GTK_SHRINK to make it as small as possible, but use GTK_FILL to
   * let it fill on the left side (so that the right alignment
   * works:
   */ 
  table_attach_left_col_x = GTK_FILL; 
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;
  
  /*
   * GTK_JUSTIFY_LEFT
   * GTK_JUSTIFY_RIGHT
   * GTK_JUSTIFY_CENTER (the default)
   * GTK_JUSTIFY_FILL
   */
  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  widget_padding_x = 5;
  widget_padding_y_even = 5; /* padding in y direction for widgets in an even row */
  widget_padding_y_odd  = 0; /* padding in y direction for widgets in an odd row  */

  ss_sec = xset_get_ss_sec();
  /* ======================================================================== */
  /* draw the GUI */

  self.table = gtk_table_new(2,6,FALSE);
  gtk_widget_set_name (self.table, "table");
  gtk_container_set_border_width (GTK_CONTAINER (self.table), widget_padding_x);

  self.brightnessl = gtk_label_new("Brightness:");
  self.brightness = gtk_hscale_new(GTK_ADJUSTMENT (gtk_adjustment_new ( (gfloat) get_brightness () / 2.55, 0, 100, 0, 0, 0)));
  gtk_scale_set_value_pos (GTK_SCALE (self.brightness), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (self.brightness), 0);

  self.screensaverl = gtk_label_new("Screensaver:");
  self.screensaverl2 = gtk_label_new("Screensaver:");

  self.screensaverbt = gtk_check_button_new_with_label ("On");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self.screensaverbt), TRUE);
  //  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (self.screensaverbt), FALSE);

  self.screensaver = gtk_hscale_new(GTK_ADJUSTMENT (gtk_adjustment_new ( ss_sec ? log((float)ss_sec)*2.8208 : 0, 0, 20, 0, 0, 0)));
  gtk_scale_set_digits (GTK_SCALE (self.screensaver), 2);
  gtk_scale_set_draw_value (GTK_SCALE (self.screensaver), FALSE);

  self.rotationl = gtk_label_new("Rotation:");
  self.rotation = gtk_option_menu_new ();

  menu =  gtk_menu_new ();

  for(i=0;i<4;i++)
    {
      glade_menuitem = gtk_menu_item_new_with_label (RotationLabels[i]);
      gtk_widget_show (glade_menuitem);
      gtk_menu_append (GTK_MENU (menu), glade_menuitem);
    }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (self.rotation),menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU (self.rotation),get_rotation());
 
  self.touchscreen = gtk_label_new("Touchscreen:");
  self.calibrate = gtk_button_new_with_label("Calibrate");

  gtk_table_attach (GTK_TABLE (self.table), self.brightnessl, 0, 1, 0, 1,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 4);
  gtk_label_set_justify (GTK_LABEL (self.brightnessl), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (self.brightnessl), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (self.brightnessl),
			widget_padding_x, widget_padding_y_even);
  /* make the label grey: */
  gtk_rc_parse_string ("widget '*self.brightnessl' style 'gpe_labels'");
  gtk_widget_set_name (self.brightnessl, "self.brightnessl");

  gtk_table_attach (GTK_TABLE (self.table), self.brightness, 1, 2, 0, 1,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 4);

  gtk_table_attach (GTK_TABLE (self.table), self.screensaverl, 0, 1, 1, 3,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 4);
  gtk_label_set_justify (GTK_LABEL (self.screensaverl), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (self.screensaverl), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (self.screensaverl),
			widget_padding_x, widget_padding_y_even);
  /* make the label grey: */
  gtk_rc_parse_string ("widget '*self.screensaverl' style 'gpe_labels'");
  gtk_widget_set_name (self.screensaverl, "self.screensaverl");

  gtk_table_attach (GTK_TABLE (self.table), self.screensaverl2, 1, 2, 1, 2,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 4);

/*  gtk_table_attach (GTK_TABLE (self.table), self.screensaverbt, 2, 3, 1, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);
*/
  gtk_table_attach (GTK_TABLE (self.table), self.screensaver, 1, 2, 2, 3,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 4);

  gtk_table_attach (GTK_TABLE (self.table), self.rotationl, 0, 1, 3, 4,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 4);
  gtk_label_set_justify (GTK_LABEL (self.rotationl), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (self.rotationl), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (self.rotationl),
			widget_padding_x, widget_padding_y_even);
  /* make the label grey: */
  gtk_rc_parse_string ("widget '*self.rotationl' style 'gpe_labels'");
  gtk_widget_set_name (self.rotationl, "self.rotationl");

  gtk_table_attach (GTK_TABLE (self.table), self.rotation, 1, 2, 3, 4,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 4);

  gtk_table_attach (GTK_TABLE (self.table), self.touchscreen, 0, 1, 4, 5,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 4);
  gtk_label_set_justify (GTK_LABEL (self.touchscreen), table_justify_left_col);
  gtk_misc_set_alignment (GTK_MISC (self.touchscreen), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (self.touchscreen),
			widget_padding_x, widget_padding_y_even);
  /* make the label grey: */
  gtk_rc_parse_string ("widget '*self.touchscreen' style 'gpe_labels'");
  gtk_widget_set_name (self.touchscreen, "self.touchscreen");

  gtk_table_attach (GTK_TABLE (self.table), self.calibrate, 1, 2, 4, 5,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 4);


  gtk_signal_connect (GTK_OBJECT (self.brightness), "draw",
                      GTK_SIGNAL_FUNC (on_brightness_hscale_draw),
                      NULL);

  gtk_signal_connect (GTK_OBJECT (self.screensaver), "draw",
                      GTK_SIGNAL_FUNC (on_screensaver_hscale_draw),
                      NULL);

  gtk_signal_connect (GTK_OBJECT (gtk_option_menu_get_menu(GTK_OPTION_MENU (self.rotation))), "selection-done",
                      GTK_SIGNAL_FUNC (on_rotation_entry_changed),
                      NULL);

  gtk_signal_connect (GTK_OBJECT (self.calibrate), "clicked",
                      GTK_SIGNAL_FUNC (on_calibrate_button_clicked),
                      NULL);

  gtk_signal_connect (GTK_OBJECT (self.screensaverbt), "clicked",
                      GTK_SIGNAL_FUNC (on_calibrate_button_clicked),
                      NULL);
  initialising = 0;

  return self.table;
}
void change_screen_saver_label(int sec)
{
  int min = sec /60;
  char buf[30];
  if(min>0)
    {
      sec = min * 60;
      sprintf(buf,"%d minutes",min);
    }
  else if(sec)
    sprintf(buf,"%d seconds",sec);
  else
      sprintf(buf,"Off");

  gtk_label_set_text(GTK_LABEL(self.screensaverl2),buf);
}
void ipaqscreen_Free_Objects()
{
}
void ipaqscreen_Save()
{
}
void ipaqscreen_Restore()
{
}

