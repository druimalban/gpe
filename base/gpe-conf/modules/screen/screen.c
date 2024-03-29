/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>, 
 *   Moray Allan <moray@sermisy.org>
 *               2003, 2004, 2007  Florian Boor <florian.boor@kernelconcepts.de>
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
#include <libintl.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/spacing.h>

#include "brightness.h"
#ifndef DISABLE_XRANDR
#include "rotation.h"
#endif
#include "callbacks.h"
#include "xset.h"

#include <math.h>

#define _(x) gettext(x)

int initialising = 1;
char *RotationLabels[4];
static int rotation_available;

static struct
{
	int brightness;
	int orientation;
	int screensaver;
}initval;

tself self;

gchar* change_screen_saver_label (GtkScale *scale, gdouble sec);

GtkWidget *screen_Build_Objects()
{
  GtkWidget *menu = NULL;
  GtkWidget *glade_menuitem = NULL;
  guint i;
  int ss_sec;
  gchar *tstr;
  guint gpe_boxspacing = gpe_get_boxspacing ();
  guint gpe_border     = gpe_get_border ();

  GtkAttachOptions table_attach_left_col_x;
  GtkAttachOptions table_attach_left_col_y;
  GtkAttachOptions table_attach_right_col_x;
  GtkAttachOptions table_attach_right_col_y;
  GtkJustification table_justify_left_col;
  //GtkJustification table_justify_right_col;
  
  GtkObject* adjLight;

  if (gdk_screen_height() > gdk_screen_width()) /* portrait or landscape display */
    {
      RotationLabels[0] = _("Portrait");
      RotationLabels[1] = _("Landscape (left)");
      RotationLabels[3] = _("Landscape (right)");
    }
  else
    {
      RotationLabels[0] = _("Landscape");
      RotationLabels[1] = _("Portrait (left)");
      RotationLabels[3] = _("Portrait (right)");
    }
	
  RotationLabels[2] = _("Inverted");
  
  table_attach_left_col_x = GTK_FILL; 
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;
  
  table_justify_left_col = GTK_JUSTIFY_LEFT;
  //table_justify_right_col = GTK_JUSTIFY_RIGHT;

  backlight_init();


  ss_sec = xset_get_ss_sec();
  initval.screensaver = ss_sec;
#ifndef DISABLE_XRANDR  
  rotation_available = check_init_rotation();
#endif  
  /* ======================================================================== */
  /* draw the GUI */

  self.table = gtk_table_new(2,6,FALSE);
  gtk_widget_set_name (self.table, "table");
  gtk_container_set_border_width (GTK_CONTAINER (self.table), gpe_border);

  self.lightl = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(self.lightl),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Light"));
  gtk_label_set_markup (GTK_LABEL (self.lightl), tstr);
  g_free(tstr);
  
  self.brightnessl = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(self.brightnessl),0.0,0.5);
  tstr = g_strdup_printf ("%s", _("Brightness"));
  gtk_label_set_markup (GTK_LABEL (self.brightnessl), tstr);
  g_free(tstr);
  
  adjLight = gtk_adjustment_new ( (gfloat) backlight_get_brightness () / 2.55, 0, 100, 0, 0, 0);
  self.brightness = gtk_hscale_new(GTK_ADJUSTMENT (adjLight));
  gtk_scale_set_value_pos (GTK_SCALE (self.brightness), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (self.brightness), 0);
  
#ifndef DISABLE_XRANDR
  self.displayl = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(self.displayl),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Display"));
  gtk_label_set_markup (GTK_LABEL (self.displayl), tstr);
  g_free(tstr);
  
  self.rotationl = gtk_label_new(_("Rotation"));
  gtk_misc_set_alignment(GTK_MISC(self.rotationl),0.0,0.5);
  
  self.rotation = gtk_option_menu_new ();
  gtk_widget_set_sensitive(self.rotation,rotation_available);

  menu =  gtk_menu_new ();

  for(i=0;i<4;i++)
    {
      glade_menuitem = gtk_menu_item_new_with_label (RotationLabels[i]);
      gtk_widget_show (glade_menuitem);
      gtk_menu_append (GTK_MENU (menu), glade_menuitem);
    }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (self.rotation),menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU (self.rotation),get_rotation());
#endif	
 
  self.touchscreen = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(self.touchscreen),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Touchscreen"));
  gtk_label_set_markup (GTK_LABEL (self.touchscreen), tstr);
  g_free(tstr);

  self.calibratel = gtk_label_new(_("Calibrate"));
  gtk_misc_set_alignment(GTK_MISC(self.calibratel),0.0,0.5);
  self.calibrate = gtk_button_new_with_label(_("Start"));
	
  gtk_table_attach (GTK_TABLE (self.table), self.lightl, 0, 1, 0, 1,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, gpe_boxspacing);
	
  gtk_table_attach (GTK_TABLE (self.table), self.brightnessl, 0, 1, 1, 2,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, gpe_boxspacing);
  gtk_misc_set_padding (GTK_MISC (self.brightnessl),
			gpe_boxspacing, gpe_boxspacing);
	
  gtk_table_attach (GTK_TABLE (self.table), self.brightness, 1, 2, 1, 2,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);
/*										
  gtk_table_attach (GTK_TABLE (self.table), self.screensaverl, 0, 1, 3, 4,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, gpe_boxspacing);
  gtk_label_set_justify (GTK_LABEL (self.screensaverl), table_justify_left_col);

  gtk_table_attach (GTK_TABLE (self.table), self.screensaverl3, 0, 1, 4, 5,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);
  gtk_misc_set_padding (GTK_MISC (self.screensaverl3),
			gpe_boxspacing, gpe_boxspacing);

  hbox = gtk_hbox_new(FALSE,gpe_boxspacing);
  gtk_box_pack_start(GTK_BOX(hbox),self.screensaverbt1 ,FALSE,TRUE,0);
  gtk_box_pack_start_defaults(GTK_BOX(hbox),self.screensaverbt2);
  gtk_table_attach (GTK_TABLE (self.table), hbox, 1, 2, 4, 5,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);

  gtk_table_attach (GTK_TABLE (self.table), self.screensaverl2, 0, 1, 5, 6,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);
  gtk_misc_set_padding (GTK_MISC (self.screensaverl2),
			gpe_boxspacing, gpe_boxspacing);
								
  gtk_table_attach (GTK_TABLE (self.table), self.screensaver, 1, 2, 5, 6,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);
*/
#ifndef DISABLE_XRANDR
  gtk_table_attach (GTK_TABLE (self.table), self.displayl, 0, 1, 6, 7,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, gpe_boxspacing);
					
  gtk_table_attach (GTK_TABLE (self.table), self.rotationl, 0, 1, 7, 8,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, gpe_boxspacing);
  gtk_label_set_justify (GTK_LABEL (self.rotationl), table_justify_left_col);
 
  gtk_table_attach (GTK_TABLE (self.table), self.rotation, 1, 2, 7, 8,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);
#endif
  gtk_table_attach (GTK_TABLE (self.table), self.touchscreen, 0, 2, 8, 9,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, gpe_boxspacing);
  gtk_label_set_justify (GTK_LABEL (self.touchscreen), table_justify_left_col);
  gtk_table_attach (GTK_TABLE (self.table), self.calibratel, 0, 1, 9, 10,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);

  gtk_table_attach (GTK_TABLE (self.table), self.calibrate, 1, 2, 9, 10,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, gpe_boxspacing);


  g_signal_connect (G_OBJECT (adjLight), "value_changed",
                    G_CALLBACK (on_brightness_hscale_draw), NULL);
		  
#ifndef DISABLE_XRANDR
  g_signal_connect (G_OBJECT (gtk_option_menu_get_menu(GTK_OPTION_MENU (self.rotation))), "selection-done",
                    G_CALLBACK (on_rotation_entry_changed),
                    NULL);
#endif
  g_signal_connect (G_OBJECT (self.calibrate), "clicked",
                    G_CALLBACK (on_calibrate_button_clicked),
                    NULL);
  
  initval.brightness = backlight_get_brightness();
#ifndef DISABLE_XRANDR
  initval.orientation = get_rotation();
#endif    

  g_timeout_add(2000, (GSourceFunc)on_light_check, (gpointer)adjLight);
    
  initialising = 0;

  gtk_widget_show_all(self.table);

  return self.table;
}

gchar*
change_screen_saver_label (GtkScale *scale, gdouble val)
{
  int min; 
  int sec;
  gchar* buf;

  if(val > 0.1)
    sec=1+(int)exp(val/2.8208);// an exponentiel range from 0 to 20 min
  else
    sec = 0;
  if(sec>60)
    sec = sec - sec % 60;
	
  min = sec / 60;
  
  if(min > 0)
    {
      sec = min * 60;
      buf = g_strdup_printf("%d %s",min,_("min"));
    }
  else 
    buf = g_strdup_printf("%d %s",sec,_("sec"));
  
  return buf;
}

void screen_Free_Objects()
{
}

void screen_Save()
{
/*
	int sec;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.screensaverbt1)))
  {
    if(GTK_ADJUSTMENT(self.adjSaver)->value>0.1)
      sec=1+(int)exp(GTK_ADJUSTMENT(self.adjSaver)->value/2.8208);// an exponentiel range from 0 to 20 min
    else
      sec = 0;
    if(sec>60)
      sec = sec - sec%60;
	
	  xset_set_ss_sec(sec); 
  }
  else
  {
	  xset_set_ss_sec(0);
  }
*/	
}

void screen_Restore()
{
  backlight_set_brightness(initval.brightness);
#ifndef DISABLE_XRANDR	
  set_rotation(initval.orientation);
#endif	
}
