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
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/question.h>

#include "globals.h"
#include "future_view.h"
#include "event-ui.h"

#define _(x) gettext(x)

GSList *events;
GtkWidget *future_list;
GtkWidget *time_label;

static void
selection_made (GtkWidget *clist, int row, gint column,
                GdkEventButton *event, GtkWidget *widget)
{
  event_t ev;

  g_object_set_data(G_OBJECT(clist),"last-row",
    g_object_get_data(G_OBJECT(clist),"selected-row"));
  g_object_set_data(G_OBJECT(clist),"selected-row",(void *)row);
  
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
  time_t start=viewtime;
  time_t end;
  event_t ev;
  event_details_t evd;
  struct tm tm;
  char buf[256];
  gchar *sbuf;
  gchar *line_info[2];
  GSList *iter;
  guint widget_width;
     
  widget_width=future_list->allocation.width;
		
  gtk_clist_freeze (GTK_CLIST (future_list));
  gtk_clist_clear (GTK_CLIST (future_list));

  gtk_clist_set_sort_column (GTK_CLIST (future_list), 0);
  
  localtime_r (&start, &tm);

  /* update label */
  strftime (buf, sizeof (buf), "%A, %d %b %Y", &tm);
  sbuf = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  gtk_label_set_text(GTK_LABEL(time_label),sbuf);
  g_free (sbuf);
  
  tm.tm_year++;
  end = mktime (&tm);
  
  /* update events */
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


static gboolean
list_key_press_event (GtkWidget *clist, GdkEventKey *k, gpointer user_data)
{
  int row;
  event_t ev;
  GtkWidget *w;
  int lastrow = (int)g_object_get_data(G_OBJECT(clist),"last-row");
  
  row = (int)g_object_get_data(G_OBJECT(clist),"selected-row");
  
  if (k->keyval == GDK_Up)
  {
    gtk_clist_select_row(GTK_CLIST(clist),row-1, 0);
    if (!row)
      {
        gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(clist)),
		                         GTK_DIR_UP);
        return TRUE;
      }
    else
      return FALSE;
  }
  
  if (k->keyval == GDK_Down)
  {
    gtk_clist_select_row(GTK_CLIST(clist),row + 1, 0);
    if (row == lastrow)
      {
        gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(clist)),
		                      GTK_DIR_TAB_FORWARD);
        lastrow = -1;
        return TRUE;
      }
    else
      {
        lastrow = row;
        return FALSE;
      }
  }
  
  if (k->keyval == GDK_Return)
  {
    if (row >= 0)
      {
        struct tm tm;
        ev = gtk_clist_get_row_data(GTK_CLIST (clist), row);
        if (ev)
          {
            w = edit_event(ev);
          }
        else
          {
            char *t;
            localtime_r (&viewtime, &tm);
            if (gtk_clist_get_text (GTK_CLIST (clist), row, 0, &t))
              strptime (t, TIMEFMT, &tm);
            w = new_event (mktime (&tm), 1);
          }
        gtk_widget_show (w);
      }
    return TRUE;
  }
 
  if (k->keyval == GDK_Delete)
  {
    row = (int)g_object_get_data(G_OBJECT(clist),"selected-row");
    if (row >= 0)
      {
        ev = gtk_clist_get_row_data(GTK_CLIST (clist), row);
        if ((ev) && (gpe_question_ask (_("Delete event?"), _("Question"), "question",
			   "!gtk-no", NULL, "!gtk-yes", NULL, NULL)))
          {
            delete_event(ev);
            g_object_set_data(G_OBJECT(clist),"selected-row",(void *)-1);
          }
      }
    return TRUE;
  }
  
  return FALSE;
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

  gtk_widget_show (scrolled_window);

  localtime_r(&t, &tm);
  strftime (buf, sizeof (buf), "%A, %d %b %Y", &tm);

  sbuf = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  time_label = gtk_label_new (sbuf);
  g_free (sbuf);
  gtk_widget_show (time_label);

  future_list = gtk_clist_new (2);
  gtk_widget_show (future_list);

  g_signal_connect(G_OBJECT (future_list), "select_row",
                   G_CALLBACK (selection_made), NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolled_window), future_list);

  gtk_box_pack_start (GTK_BOX(vbox), time_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (vbox), "update_hook",
                     (gpointer) future_view_update);
                     
  g_signal_connect (G_OBJECT (future_list), "key_press_event", 
		    G_CALLBACK (list_key_press_event), NULL);
  g_object_set_data(G_OBJECT(future_list),"selected-row",(void *)0);

  return vbox;
}
