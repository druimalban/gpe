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
#include <libintl.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/pixmaps.h>
#include <gpe/event-db.h>
#include <gpe/question.h>
#include <gpe/schedule.h>

#include "event-ui.h"
#include "globals.h"
#include "day_view.h"

#include "gtkdatesel.h"

#define _(x) gettext(x)

static GSList *strings;
static GSList *day_events[24], *untimed_events;
static GtkWidget *day_list;
static GtkWidget *datesel;
static GtkAdjustment *scroll_adjustment;

static GtkStyle *light_style, *dark_style, *time_style;
static GdkColor light_color, dark_color, time_color;


static void
delete_event (event_t ev)
{
  event_t ev_real;
  event_details_t ev_d;
  recur_t r;
  
  ev_real = (event_t)ev->cloned_ev;
  ev_d = event_db_get_details (ev_real);
     
  if (ev_real->recur)
    {
      if (gpe_question_ask(
        _("Delete all recurring entries?\n(If no, delete this instance only)"), 
        _("Question"), "question", "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
        {			  
          if (ev_real->flags & FLAG_ALARM)
            {
              schedule_cancel_alarm (ev_real->uid, ev->start);
              schedule_next (0,0);
            }
          event_db_remove (ev_real);
        }
      else 
        {
          r = event_db_get_recurrence (ev_real);
          r->exceptions = g_slist_append (r->exceptions, (void *)ev->start);
          event_db_flush (ev_real);
        }
    }
  else
    {			  
      if (ev_real->flags & FLAG_ALARM)
        {
          schedule_cancel_alarm (ev_real->uid, ev->start);
          schedule_next (0,0);
        }
      event_db_remove (ev_real);
  }
  
  update_all_views ();
  event_db_forget_details (ev_real);
}


static gboolean
window_key_press_event (GtkWidget *clist, GdkEventKey *k, gpointer user_data)
{
  int row;
  event_t ev;
  GtkWidget *w;
  
  row = (int)g_object_get_data(G_OBJECT(clist),"selected-row");
  
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

static char *
format_event (event_t ev)
{
  char buf[256];
  char *p = buf;
  size_t l = sizeof (buf);
  size_t r;
  struct tm tm;
  time_t t;
  event_details_t evd;

  if ((ev->flags & FLAG_UNTIMED) == 0 && day_view_combined_times == FALSE)
    {
      localtime_r (&ev->start, &tm);
      r = strftime (p, l, "%R", &tm);
      if (r == 0) return NULL;
      p += r;
      l -= r;
      
      if (ev->duration)
	{
	  t = ev->start + ev->duration;
	  localtime_r (&t, &tm);
	  r = strftime (p, l, "-%R", &tm);
	  if (r == 0) return NULL;
	  p += r;
	  l -= r;
	}
      if (l)
	{
	  *p++ = ' ';
	  l--;
	}
    }
      
  evd = event_db_get_details (ev);
  if (evd == NULL) return NULL;
  strncpy (p, evd->summary, l - 1);
  p[l - 1] = 0;
  
  return g_strdup (buf);
}

static void 
selection_made (GtkWidget      *clist,
		gint            row,
		gint            column,
		GdkEventButton *event,
		GtkWidget      *widget)
{
  event_t ev;
  GtkWidget *w;
  
  g_object_set_data(G_OBJECT(clist),"selected-row",(void *)row);

  if (!event) return;
  
  if ((event->type == GDK_2BUTTON_PRESS) 
     || ((event->type == GDK_BUTTON_RELEASE) && (column == 1)))
    {
      struct tm tm;
      
      ev = gtk_clist_get_row_data (GTK_CLIST (clist), row);
      
      if (ev) 
	{
	  w = edit_event (ev);
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
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      // set the default event time to match the selected row
      struct tm tm;
      char *t;
      if (gtk_clist_get_text (GTK_CLIST (clist), row, 0, &t))
	{
	  localtime_r (&viewtime, &tm);
	  strptime (t, TIMEFMT, &tm);
	  viewtime = mktime (&tm);
	}
    }
}

static gint
day_view_update ()
{
  guint hour, row = 0;
  time_t start, end;
  struct tm tm_start, tm_end;
  char buf[10];
  gchar *line_info[2];
  GSList *iter;
  guint i, width = 0, widget_width;
  PangoLayout *pl = gtk_widget_create_pango_layout (GTK_WIDGET (day_list), NULL);
  PangoRectangle pr;
  double pos;
  gchar *last_text = NULL;
  
  widget_width = day_list->allocation.width;

  if (! light_style)
    {
      guint j;
      light_color.red = 60000;
      light_color.green = 60000;
      light_color.blue = 60000;
  
      time_color.red = 20000;
      time_color.green = 20000;
      time_color.blue = 20000;
  
      dark_color.red = 45000;
      dark_color.green = 45000;
      dark_color.blue = 45000;

      light_style = gtk_style_copy (gtk_widget_get_style (day_list));
      dark_style = gtk_style_copy (gtk_widget_get_style (day_list));
      time_style = gtk_style_copy (gtk_widget_get_style (day_list));
      
      for (j = 0; j < 5; j++) 
        {
          light_style->base[j] = light_color;
          dark_style->base[j] = dark_color;
          time_style->base[j] = time_color;
          time_style->fg[j] = light_color;
        }
    }
  
  gtk_clist_freeze (GTK_CLIST (day_list));
  gtk_clist_clear (GTK_CLIST (day_list));

  if (strings)
    {
      for (iter = strings; iter; iter = iter->next)
        g_free (iter->data);
      g_slist_free (strings);
      strings = NULL;
    }

  localtime_r (&viewtime, &tm_start);
  pos = 24 / (tm_start.tm_hour + 1);
  gtk_adjustment_set_value (scroll_adjustment, pos);
  tm_start.tm_hour = 0;
  tm_start.tm_min = 0;
  tm_start.tm_sec = 0;
  start = mktime (&tm_start);
  localtime_r (&viewtime, &tm_end);
  tm_end.tm_hour = 23;
  tm_end.tm_min = 59;
  tm_end.tm_sec = 0;
  end = mktime (&tm_end);
  
  if (untimed_events) event_db_list_destroy (untimed_events);
  untimed_events = event_db_untimed_list_for_period (start, end, TRUE);

  for (iter = untimed_events; iter; iter = iter->next)
    {
      event_t ev = (event_t)iter->data;
      gchar *text = format_event (ev);
      g_slist_append (strings, text);

      line_info[0] = NULL;
      line_info[1] = text;

      gtk_clist_append (GTK_CLIST (day_list), line_info);
      gtk_clist_set_row_data (GTK_CLIST (day_list), row, ev);
      row++;
    }
  
  for (hour = 0; hour <= 23; hour++)
    {
      localtime_r (&viewtime, &tm_start);

      tm_start.tm_hour = hour;
      tm_start.tm_min = 0;
      tm_start.tm_sec = 0;
      start = mktime (&tm_start);
      localtime_r (&viewtime, &tm_end);
      tm_end.tm_hour = hour+1;
      tm_end.tm_min = 0;
      tm_end.tm_sec = 0;
      end = mktime (&tm_end);

      if (day_events[hour]) 
        event_db_list_destroy (day_events[hour]);
      day_events[hour] = event_db_untimed_list_for_period (start, end-1, FALSE);
      
      for (iter = day_events[hour]; iter; iter = iter->next)
	    ((event_t)iter->data)->mark = FALSE;
    }

  for (hour = 0; hour <= 23; hour++)
    {
      guint w;
      gchar *text = NULL;
      event_t ev = NULL;
 
      tm_start.tm_hour = hour;
      tm_start.tm_min = 0;
      tm_start.tm_sec = 0;

      strftime (buf, sizeof (buf), "%R", &tm_start);

      for (iter = day_events[hour]; iter; iter = iter->next)
        {
          ev = (event_t) iter->data;
    
          if (ev->mark == FALSE)
            {
              struct tm tm;
              ev->mark = TRUE;
              text = format_event (ev);
              g_slist_append (strings, text);
    
              if (day_view_combined_times)
                {
                  localtime_r (&ev->start, &tm);
                  strftime (buf, sizeof (buf), "%R", &tm);
                }
              break;
            }
        } 

      if (!last_text || !text || (strcmp(last_text,text)))
        line_info[1] = text;
      else 
        line_info[1] = NULL;
      line_info[0] = buf;
      if (last_text) 
        g_free(last_text);
      last_text = g_strdup(text);

      pango_layout_set_text (pl, buf, strlen (buf));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      w = pr.width;

      if (w > width) width = w;
	
      /* don't display very early/late slots if they are not used */
      if (ev || ((hour > 5) && (hour < 23)))
      {
        gtk_clist_append (GTK_CLIST (day_list), line_info);
        gtk_clist_set_row_data (GTK_CLIST (day_list), row, ev);
      }
      
      if (ev)
        {
          GdkPixmap *pmap;
          GdkBitmap *bmap;
          
          if ((ev->flags & FLAG_ALARM) && ev->recur)
            {
              if (gpe_find_icon_pixmap ("bell_recur", &pmap, &bmap))
                gtk_clist_set_pixtext (GTK_CLIST (day_list), row, 1, text, 5,
                           pmap, bmap);
            }
          else if (ev->flags & FLAG_ALARM)
            { 
              if (gpe_find_icon_pixmap ("bell", &pmap, &bmap))
                gtk_clist_set_pixtext (GTK_CLIST (day_list), row, 1, text, 5,
                           pmap, bmap);
            }
          else if (ev->recur)
            {
              if (gpe_find_icon_pixmap ("recur", &pmap, &bmap))
                gtk_clist_set_pixtext (GTK_CLIST (day_list), row, 1, text, 5,
                           pmap, bmap);
            }
        }
      
      /* don't display very early/late slots if they are not used */
      if (ev || ((hour > 5) && (hour < 23)))
        {
          gtk_clist_set_cell_style (GTK_CLIST (day_list), row, 1, 
                  day_events[hour] ? dark_style : light_style); 
          row++;
        }
              
  for (; iter; iter = iter->next)
	{
	  ev = (event_t) iter->data;
      
	  if (ev->mark)
	    continue;

	  ev->mark = TRUE;

	  if (day_view_combined_times)
	    {
	      struct tm tm;
	      localtime_r (&ev->start, &tm);
	      strftime (buf, sizeof (buf), "%R", &tm);
	      line_info[0] = buf;
	    }
	  else
	    {
	      line_info[0] = NULL;
	    }

        line_info[1] = format_event (ev);
        g_slist_append (strings, line_info[1]);
  
        gtk_clist_append (GTK_CLIST (day_list), line_info);
        gtk_clist_set_row_data (GTK_CLIST (day_list), row, ev);
        gtk_clist_set_cell_style (GTK_CLIST (day_list), row, 1, dark_style);
        row++;
      } 
      
  }

  for (i = 0; i < row; i++)
    gtk_clist_set_cell_style (GTK_CLIST (day_list), i, 0, time_style);

  gtk_clist_set_column_width (GTK_CLIST (day_list), 0, width + 4);
  gtk_clist_set_column_width (GTK_CLIST (day_list), 1, widget_width - 20 - (width + 4));

  gtk_clist_thaw (GTK_CLIST (day_list));

  g_object_unref (pl);
  
  return TRUE;
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *clist)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  day_view_update ();
}

static void
update_hook_callback()
{
  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
  gtk_widget_draw (datesel, NULL);
      
  day_view_update ();
}

GtkWidget *
day_view(void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);

  gtk_widget_show (scrolled_window);

  datesel = gtk_date_sel_new (GTKDATESEL_FULL);
  gtk_widget_show (datesel);
  
  day_list = gtk_clist_new (2);
  gtk_widget_show (day_list);
 
  g_signal_connect (G_OBJECT (day_list), "select_row",
                    G_CALLBACK (selection_made), NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  gtk_container_add (GTK_CONTAINER (scrolled_window), day_list);

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (datesel), "changed",
                    G_CALLBACK (changed_callback), day_list);

  g_object_set_data (G_OBJECT (vbox), "update_hook",
		     (gpointer) update_hook_callback);

  scroll_adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window));

  g_signal_connect (G_OBJECT (day_list), "key_press_event", 
		    G_CALLBACK (window_key_press_event), NULL);
  g_object_set_data(G_OBJECT(day_list),"selected-row",(void *)-1);
            
  return vbox;
}
