/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "event-db.h"
#include "gtkdatesel.h"
#include "globals.h"
#include "day_view.h"

extern GdkFont *timefont, *datefont;
  
gint bias = 8;

#define SECONDS_IN_DAY (24*60*60)

void selection_made( GtkWidget      *clist,
                     gint            row,
                     gint            column,
                     GdkEventButton *event,
                     GtkWidget      *widget)
{
    event_t ev;
    
    if (event->type == GDK_2BUTTON_PRESS)
    {
      guint hour = row;
      GtkWidget *appt;
      struct tm tm;
      
      ev=gtk_clist_get_row_data(GTK_CLIST(clist), row);
      if (ev) localtime_r (&(ev->start), &tm);
      else {
	      localtime_r (&viewtime, &tm);
              tm.tm_hour = hour+1;
      	      tm.tm_min = 0;
	      tm.tm_sec = 0;
      }
      appt = new_event (mktime (&tm), 1, ev);
      gtk_widget_show (appt);
    }
    return;
}

static gint
day_view_update ()
{
  
  GtkStyle *light_style, *dark_style, *time_style;
  GdkColor light_color, dark_color, time_color;
  guint hour, j, row=0;
  time_t start, end;
  event_t ev;
  event_details_t evd;
  struct tm tm_start, tm_end;
  char buf[10];
  gchar *line_info[2];
  GSList *day_events[24];
      
  light_color.red = 60000;
  light_color.green = 60000;
  light_color.blue = 60000;
  
  time_color.red = 20000;
  time_color.green = 20000;
  time_color.blue = 20000;
  
  dark_color.red = 45000;
  dark_color.green = 45000;
  dark_color.blue = 45000;
     
  light_style = gtk_style_copy(gtk_widget_get_style(day_list));
  dark_style = gtk_style_copy(gtk_widget_get_style(day_list));
  time_style = gtk_style_copy(gtk_widget_get_style(day_list));
  
  for(j = 0; j < 5; j++) {
      light_style->base[j] = light_color;
      dark_style->base[j] = dark_color;
      time_style->base[j] = time_color;
      time_style->fg[j] = light_color;
  }

  gtk_clist_freeze(GTK_CLIST(day_list));
  gtk_clist_clear(GTK_CLIST(day_list));

  gtk_clist_set_column_width (GTK_CLIST(day_list), 0, 50);
  gtk_clist_set_column_width (GTK_CLIST(day_list), 1, 55);
  
  for (hour = 0; hour <= 23; hour++)
    {
      GSList *iter;
      int found=0;
      
      localtime_r (&viewtime, &tm_start);
      tm_start.tm_hour = hour;
      tm_start.tm_min = 0;
      tm_start.tm_sec = 0;
      start=mktime (&tm_start);
      localtime_r (&viewtime, &tm_end);
      tm_end.tm_hour = hour+1;
      tm_end.tm_min = 0;
      tm_end.tm_sec = 0;
      end=mktime (&tm_end);
      
      day_events[hour] = event_db_list_for_period (start, end-1);
      
      for (iter = day_events[hour]; iter; iter = iter->next)
	{
	  found=1;
	  ev = (event_t) iter->data;
	  evd = event_db_get_details (ev);

          line_info[1]=evd->description;
          strftime (buf, sizeof (buf), "%l:%M%p", &tm_start);
          line_info[0]=buf;
      
          gtk_clist_append(GTK_CLIST(day_list), line_info);
	  gtk_clist_set_row_data(GTK_CLIST(day_list), row, ev);
	
	  gtk_clist_set_cell_style(GTK_CLIST(day_list), row, 1, time_style);
	  row++;
	  
       } 
       
       if (!found) {
       
	  line_info[1]=NULL;
          strftime (buf, sizeof (buf), "%l:%M%p", &tm_start);
          line_info[0]=buf;
      
          gtk_clist_append(GTK_CLIST(day_list), line_info);
		
	  if (hour & 1)
             gtk_clist_set_cell_style(GTK_CLIST(day_list), row, 1, light_style); 
          else 
	     gtk_clist_set_cell_style(GTK_CLIST(day_list), row, 1, dark_style);
          row++;
	  
       }
       
       gtk_clist_set_cell_style(GTK_CLIST(day_list), hour, 0, time_style);

    }
  
  gtk_clist_moveto(GTK_CLIST(day_list), bias, 0, 0.0, 0.0);

  gtk_clist_thaw(GTK_CLIST(day_list));
  
  return TRUE;
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *clist)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  day_view_update();
}

GtkWidget *
day_view(void)
{
  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new (GTKDATESEL_FULL);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  
  day_list = gtk_clist_new(2);
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (datesel));
  
  gtk_signal_connect(GTK_OBJECT(day_list), "select_row",
                       GTK_SIGNAL_FUNC(selection_made),
                       NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  day_view_update();
  gtk_container_add(GTK_CONTAINER(scrolled_window), day_list);
  
  gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  
  gtk_signal_connect(GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), day_list);
  
  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) day_view_update);

  return vbox;
}
