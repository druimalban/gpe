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

#include <gtk/gtk.h>

#include <gpe/gtkdatesel.h>
#include "globals.h"
#include "event-db.h"
#include "week_view.h"
#include "event-ui.h"

#define _(x) gettext(x)

static struct tm today;
static GtkWidget *datesel;

static GtkWidget *list[7];
static GtkWidget *week_view_vbox;

static GtkStyle *dark_style, *red_style;
static GdkColor dark_color, red_color;

struct week_day
{
  struct tm tm;
  guint y;
  gchar *string;
  GSList *events;
  gboolean is_today;
} week_days[7];

static void 
selection_made(GtkWidget *w, 
	       gint            row,
	       gint            column,
	       GdkEventButton *event,
	       GtkWidget      *widget)
{
  if (event->type == GDK_2BUTTON_PRESS)
    {
      if (row==0) {
	struct week_day *new_day = g_malloc (sizeof (struct week_day));
	
	new_day = gtk_clist_get_row_data (GTK_CLIST (w), row);
      	viewtime = mktime (&(new_day->tm));
        set_day_view ();      
      }
      
      else {
	event_t ev = gtk_clist_get_row_data (GTK_CLIST (w), row);
      	if (ev)
	{
	  GtkWidget *appt = edit_event (ev);
	  gtk_widget_show (appt);
	}
      }
    }
}

static void
week_view_update (void)
{
  guint day, j;
  time_t t = time (NULL);
  struct tm tm;
  gchar *line_info[2];
	      
  localtime_r (&t, &today);

  t = viewtime;
  localtime_r (&t, &tm);
  if (week_starts_monday)
    tm.tm_wday = (tm.tm_wday + 6) % 7;
  t -= SECONDS_IN_DAY * tm.tm_wday;
  t -= tm.tm_sec + (60 * tm.tm_min) + (60 * 60 * tm.tm_hour);

  if (! dark_style)\
  {
    dark_color.red = 52000;
    dark_color.green = 52000;
    dark_color.blue = 52000;
    red_color.red = 52000;
    red_color.green = 40000;
    red_color.blue = 40000;

    red_style = gtk_style_copy (gtk_widget_get_style (list[0]));
    dark_style = gtk_style_copy (gtk_widget_get_style (list[0]));
    
    for (j = 0; j < 5; j++) {
	    dark_style->base[j] = dark_color;
	    red_style->base[j] = red_color;
    }
  }	
	  
  for (day = 0; day < 7; day++)
    {
      char buf[32];
      GSList *iter;      

      if (week_days[day].events)
	event_db_list_destroy (week_days[day].events);

      week_days[day].events = event_db_list_for_period (t, t + SECONDS_IN_DAY - 1);

      localtime_r (&t, &tm);
      week_days[day].tm=tm;
      week_days[day].is_today = (tm.tm_mday == today.tm_mday 
		       && tm.tm_mon == today.tm_mon 
		       && tm.tm_year == today.tm_year);
      strftime(buf, sizeof (buf), "%a %d %B", &tm);
      if (week_days[day].string)
	g_free (week_days[day].string);
      week_days[day].string = g_strdup (buf);

      if (week_days[day].events)
	{
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
	      event_t ev = iter->data;
	      ev->mark = FALSE;
	    }
	}

      t += SECONDS_IN_DAY;
    }

  for (day = 0; day < 7; day++)
    {
      guint row = 0;

      gtk_clist_clear (GTK_CLIST (list[day]));

      line_info[0] = NULL;
      line_info[1] = week_days[day].string;
      gtk_clist_append (GTK_CLIST (list[day]), line_info);    
      gtk_clist_set_row_data (GTK_CLIST (list[day]), row, &(week_days[day]));
      if (!week_days[day].is_today) gtk_clist_set_row_style (GTK_CLIST (list[day]), row, dark_style); 
      else gtk_clist_set_row_style (GTK_CLIST (list[day]), row, red_style); 
      row++;
       
      if (week_days[day].events)
	{
	  GSList *iter;
	  
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
	      event_t ev = iter->data;
	      event_details_t evd = event_db_get_details (ev);
	      
	      line_info[0] = evd->summary;
	      line_info[1] = NULL;
	      gtk_clist_append (GTK_CLIST (list[day]), line_info);
	      gtk_clist_set_row_data (GTK_CLIST (list[day]), row, ev);
	      row++;
	    }
	}
    }

  gtk_widget_draw (week_view_vbox, NULL);
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *entry)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));

  week_view_update ();
}

static void
update_hook_callback()
{
  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
  gtk_widget_draw (datesel, NULL);

  week_view_update ();
}

GtkWidget *
week_view(void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *scroller = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);
  guint i;

  gtk_widget_show (scroller);
  gtk_widget_show (vbox2);

  datesel = gtk_date_sel_new (GTKDATESEL_WEEK);
  gtk_widget_show (datesel);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroller), 
					 vbox2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  for (i = 0; i < 7; i++)
    {
      
      list[i] = gtk_clist_new (2);
      gtk_widget_show (list[i]);
      
      gtk_clist_set_column_width (GTK_CLIST (list[i]), 0, 120);
      gtk_clist_set_column_width (GTK_CLIST (list[i]), 1, 70);
      gtk_clist_set_column_justification (GTK_CLIST (list[i]), 0,
       			GTK_JUSTIFY_LEFT);
      gtk_clist_set_column_justification (GTK_CLIST (list[i]), 1,
       			GTK_JUSTIFY_RIGHT);
      gtk_clist_set_shadow_type (GTK_CLIST (list[i]), GTK_SHADOW_NONE);
      gtk_widget_show (list[i]);

      gtk_signal_connect (GTK_OBJECT (list[i]), "select_row",
			 GTK_SIGNAL_FUNC (selection_made),
			 NULL);

      gtk_box_pack_start (GTK_BOX (vbox2), list[i], TRUE, TRUE, 0);

    }

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), NULL);

  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook",
                       (gpointer) update_hook_callback);

  week_view_vbox = vbox;

  return vbox;
}
