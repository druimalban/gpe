/*
 * Copyright (C) 2001 Philip Blundell <philb@gnu.org>
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

#include "gtkdatesel.h"
#include "globals.h"
#include "future_view.h"

extern void selection_made( GtkWidget      *clist,
                     gint            row,
                     gint            column,
                     GdkEventButton *event,
                     GtkWidget      *widget);
		     
static gint
future_view_update ()
{
  
  gint row=0;
  time_t start=time(NULL);
  time_t end;
  event_t ev;
  event_details_t evd;
  struct tm tm;
  char buf[256];
  gchar *line_info[2];
  GSList *future_events[1];
  GSList *iter;
     
  gtk_clist_freeze(GTK_CLIST(future_list));
  gtk_clist_clear(GTK_CLIST(future_list));

  gtk_clist_set_column_width (GTK_CLIST(future_list), 0, 100);
  gtk_clist_set_column_width (GTK_CLIST(future_list), 1, 55);
  gtk_clist_set_sort_column (GTK_CLIST(future_list), 0);
  
  localtime_r (&start, &tm);
  tm.tm_year++;
  end=mktime (&tm);
      
  future_events[0] = event_db_list_for_period (start, end);
      
  for (iter = future_events[0]; iter; iter = iter->next)
  {
	  ev = (event_t) iter->data;
	  evd = event_db_get_details (ev);

          line_info[1]=evd->description;
          localtime_r (&(ev->start), &tm);
  	  strftime (buf, sizeof (buf), "%F %R", &tm);
          line_info[0]=buf;
      
          gtk_clist_append(GTK_CLIST(future_list), line_info);
	  gtk_clist_set_row_data(GTK_CLIST(future_list), row, ev);
	
	  row++;
	  
  } 
       
  gtk_clist_sort (GTK_CLIST(future_list));
  gtk_clist_thaw(GTK_CLIST(future_list));
  
  return TRUE;
}

GtkWidget *
future_view(void)
{
  time_t t=time(NULL);
  struct tm tm;
  char buf[64];
  
  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *label;
  
  localtime_r(&t, &tm);
  strftime (buf, sizeof(buf), "%A, %d %b %Y", &tm);
  
  label = gtk_label_new (buf);
  
  future_list = gtk_clist_new(2);
  
  gtk_signal_connect(GTK_OBJECT(future_list), "select_row",
                       GTK_SIGNAL_FUNC(selection_made),
                       NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  future_view_update();
  
  gtk_container_add(GTK_CONTAINER(scrolled_window), future_list);
  
  gtk_box_pack_start (GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
  
  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) future_view_update);
  
 return vbox;
}
