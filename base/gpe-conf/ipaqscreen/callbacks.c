/*
 * gpe-conf
 *
 * Copyright (C) 2002   Moray Allan <moray@sermisy.org>,Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>

#include "callbacks.h"
#include "xset.h"

#include "brightness.h"
#include "calibrate.h"
#include "rotation.h"
void
										
on_brightness_hscale_draw              (GtkWidget       *adjustment,
                                        gpointer         user_data)
{
  set_brightness ((int) (GTK_ADJUSTMENT(adjustment)->value * 2.55));
}



void change_screen_saver_label(int);

void
on_screensaver_hscale_draw              (GtkWidget       *adjustment,
                                        gpointer         user_data)
{
  int sec;

  if(GTK_ADJUSTMENT(adjustment)->value>0.1)
    sec=1+(int)exp(GTK_ADJUSTMENT(adjustment)->value/2.8208);// an exponentiel range from 0 to 20 min
  else
    sec = 0;
  if(sec>60)
    sec = sec - sec%60;


  change_screen_saver_label(sec);

  xset_set_ss_sec(sec); // round to the min.
  
}

void
on_screensaver_button_clicked           (GtkWidget       *widget,
                                        GdkRectangle    *area,
                                        gpointer         user_data)
{
  GtkAdjustment *adjustment;
  adjustment = gtk_range_get_adjustment (GTK_RANGE(widget));
}


void
on_rotation_entry_changed              (GtkWidget     *menu,
                                        gpointer         user_data)
{
  // it seems that it is the only way to have the nb of the selected item..

  GtkWidget *active=gtk_menu_get_active(GTK_MENU(menu));
  GList *children = gtk_container_children(GTK_CONTAINER(menu));
  int i = 0;
  while(children != NULL && active!=GTK_WIDGET(children->data))
    {
      children = children->next;
      i++;
    }
  set_rotation(i);

}


void
on_calibrate_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  calibrate ();
}

