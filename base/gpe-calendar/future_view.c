/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
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

#include "globals.h"
#include "future_view.h"
#include "event-ui.h"

GSList *events;
GtkWidget *future_list;

static void
selection_made (GtkWidget *clist, int row, gint column,
                GdkEventButton *event, GtkWidget *widget)
{
  event_t ev;

  if (event->type == GDK_2BUTTON_PRESS)
    {
      ev = gtk_clist_get_row_data (GTK_CLIST (clist), row);
      if (ev)
        gtk_widget_show (edit_event (ev));
    }
}

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
  GSList *iter;
  guint widget_width;
     
  widget_width=future_list->allocation.width;
		
  gtk_clist_freeze (GTK_CLIST (future_list));
  gtk_clist_clear (GTK_CLIST (future_list));

  gtk_clist_set_sort_column (GTK_CLIST (future_list), 0);
  
  localtime_r (&start, &tm);
  tm.tm_year++;
  end = mktime (&tm);
   
  if (events) event_db_list_destroy (events);
     
  events = event_db_list_for_future (start, 15);
      
  for (iter = events; iter; iter = iter->next)
    {
      ev = (event_t) iter->data;
      evd = event_db_get_details (ev);
      
      line_info[1] = evd->summary;
      localtime_r (&(ev->start), &tm);
      strftime (buf, sizeof (buf), "%x " TIMEFMT, &tm);
      line_info[0] = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
      
      gtk_clist_append (GTK_CLIST (future_list), line_info);
      gtk_clist_set_row_data (GTK_CLIST (future_list), row, ev);

      g_free (line_info[0]);
      
      row++;
    } 

  gtk_clist_sort (GTK_CLIST (future_list));
  gtk_clist_thaw (GTK_CLIST (future_list));
  
  return TRUE;
}


void
future_free_lists(void)
{
  if (events) 
    event_db_list_destroy (events);
  events = NULL;
}

GtkWidget *
future_view (void)
{
  time_t t = time (NULL);
  struct tm tm;
  char buf[64];
  gchar *sbuf;

  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *label;

  gtk_widget_show (scrolled_window);

  localtime_r(&t, &tm);
  strftime (buf, sizeof (buf), "%A, %d %b %Y", &tm);

  sbuf = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  label = gtk_label_new (sbuf);
  g_free (sbuf);
  gtk_widget_show (label);

  future_list = gtk_clist_new (2);
  gtk_widget_show (future_list);

  g_signal_connect(G_OBJECT (future_list), "select_row",
                   G_CALLBACK (selection_made), NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolled_window), future_list);

  gtk_box_pack_start (GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (vbox), "update_hook",
                     (gpointer) future_view_update);

  return vbox;
}
