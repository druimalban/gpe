/*
 * gpe-conf
 *
 * Copyright (C) 2002   Moray Allan <moray@sermisy.org>,Pierre TARDY <tardyp@free.fr>
 *               2003,2004   Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include <math.h>
#include "../applets.h"
#include "callbacks.h"
#include "xset.h"

#include "brightness.h"
#include "calibrate.h"
#include "rotation.h"

extern tself self;


gint 
on_light_check(gpointer adj)
{
	gtk_adjustment_set_value(GTK_RANGE(self.brightness)->adjustment,
		(gfloat) get_brightness() / 2.55);
 	return TRUE;	
}


void
on_brightness_hscale_draw              (GtkObject       *adjustment,
                                        gpointer         user_data)
{
	char val[10];
	snprintf(val,9,"%d",(int) (GTK_ADJUSTMENT(adjustment)->value * 2.55));
	suid_exec("SCRB",val);
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
#ifndef DISABLE_XRANDR
  char val[10];
  // it seems that it is the only way to have the nb of the selected item..

  GtkWidget *active=gtk_menu_get_active(GTK_MENU(menu));
  GList *children = gtk_container_children(GTK_CONTAINER(menu));
  int i = 0;
  while(children != NULL && active!=GTK_WIDGET(children->data))
    {
      children = children->next;
      i++;
    }
  snprintf(val,9,"%d",i);
  suid_exec("SCRR",val);
#endif	
}


void
on_calibrate_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  calibrate();
}
