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
#include <time.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#include "init.h"

#include "event-db.h"
#include "event-ui.h"
#include "globals.h"

#include "day_view.h"
#include "week_view.h"
#include "future_view.h"
#include "year_view.h"
#include "month_view.h"

extern void about (void);

GdkFont *timefont;
GdkFont *datefont;
GdkFont *yearfont;
GList *times;
time_t viewtime;

static GtkWidget *window, *day, *week, *month, *future, *year, *current_view;

guint window_x = 240, window_y = 310;

gboolean week_starts_monday = TRUE;

static guint nr_days[] = { 31, 28, 31, 30, 31, 30, 
			   31, 31, 30, 31, 30, 31 };

guint
days_in_month (guint year, guint month)
{
  if (month == 1)
    {
      return ((year % 4) == 0
	      && ((year % 100) != 0
		  || (year % 400) == 0)) ? 29 : 28;
    }

  return nr_days[month];
}

void
update_current_view (void)
{
  if (current_view)
    {
      gpointer p = gtk_object_get_data (GTK_OBJECT (current_view),
					"update_hook");
      if (p)
	{
	  void (*f)(void) = p;
	  f();
	}
    }
}

static void
new_view (GtkWidget *widget)
{
  if (current_view)
    gtk_widget_hide (current_view);

  gtk_widget_show_all (widget);
  current_view = widget;

  update_current_view ();
}

void
set_day_view(void)
{
  new_view (day);
}

static void
set_week_view(void)
{
  new_view (week);
}

static void
set_month_view(void)
{
  new_view (month);
}

static void
set_future_view(void)
{
  new_view (future);
}

static void
set_year_view(void)
{
  new_view (year);
}

static void
new_appointment(void)
{
  GtkWidget *appt = new_event (time (NULL), 0);
  gtk_widget_show (appt);
}

int
main(int argc, char *argv[])
{
  GtkWidget *toolbar;
  GdkPixmap *tmp_pix;
  GdkBitmap *tmp_mask;
  GtkWidget *new_pixmap, *today_pixmap, *future_pixmap, *day_pixmap, *week_pixmap, *month_pixmap, *year_pixmap;
  GtkWidget *vbox;
  guint hour;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);
  
  if (event_db_start () == FALSE)
    exit (1);

  for (hour = 0; hour < 24; hour++)
    {
      char buf[32];
      struct tm tm;
      time_t t=time(NULL);
      
      localtime_r (&t, &tm);
      tm.tm_hour = hour;
      tm.tm_min = 0;
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      times = g_list_append (times, g_strdup (buf));
      tm.tm_hour = hour;
      tm.tm_min = 30;
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      times = g_list_append (times, g_strdup (buf));
    }

  vbox = gtk_vbox_new (FALSE, 0);

  timefont = gdk_font_load ("-*-*-medium-r-normal--10-*-*-*-*-*-*");
  if (timefont == NULL)
    {
      printf ("Couldn't get time font\n");
      abort ();
    }
  datefont = gdk_font_load ("-*-*-medium-r-normal--8-*-*-*-c-*-*");
  if (datefont == NULL)
    {
      printf ("Couldn't get date font\n");
      abort ();
    }
  yearfont = gdk_font_load ("-*-*-medium-r-normal--8-*-*-*-*-*-*");
  if (yearfont == NULL)
    {
      printf ("Couldn't get year font\n");
      abort ();
    }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  tmp_pix = gdk_pixmap_create_from_xpm (window->window, &tmp_mask, NULL, "pixmaps/new.xpm");
  new_pixmap = gtk_pixmap_new (tmp_pix, tmp_mask);

  tmp_pix = gdk_pixmap_create_from_xpm (window->window, &tmp_mask, NULL, "pixmaps/today.xpm");
  today_pixmap = gtk_pixmap_new (tmp_pix, tmp_mask);

  tmp_pix = gdk_pixmap_create_from_xpm (window->window, &tmp_mask, NULL, "pixmaps/futureview.xpm");
  future_pixmap = gtk_pixmap_new (tmp_pix, tmp_mask);

  tmp_pix = gdk_pixmap_create_from_xpm (window->window, &tmp_mask, NULL, "pixmaps/dayview.xpm");
  day_pixmap = gtk_pixmap_new (tmp_pix, tmp_mask);

  tmp_pix = gdk_pixmap_create_from_xpm (window->window, &tmp_mask, NULL, "pixmaps/weekview.xpm");
  week_pixmap = gtk_pixmap_new (tmp_pix, tmp_mask);

  tmp_pix = gdk_pixmap_create_from_xpm (window->window, &tmp_mask, NULL, "pixmaps/monthview.xpm");
  month_pixmap = gtk_pixmap_new (tmp_pix, tmp_mask);

  tmp_pix = gdk_pixmap_create_from_xpm (window->window, &tmp_mask, NULL, "pixmaps/yearview.xpm");
  year_pixmap = gtk_pixmap_new (tmp_pix, tmp_mask);

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "New Appointment", "New Appointment", "New Appointment", GTK_WIDGET (new_pixmap), new_appointment, NULL);
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Today", "Today", "Today", GTK_WIDGET (today_pixmap), new_appointment, NULL);
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Future View", "Future View", "Future View", GTK_WIDGET (future_pixmap), set_future_view, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Day View", "Day View", "Day View", GTK_WIDGET (day_pixmap), set_day_view, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Week View", "Week View", "Week View", GTK_WIDGET (week_pixmap), set_week_view, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Month View", "Month View", "Month View", GTK_WIDGET (month_pixmap), set_month_view, NULL);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Year View", "Year View", "Year View", GTK_WIDGET (year_pixmap), set_year_view, NULL);

  time (&viewtime);
  week = week_view ();
  day = day_view ();
  month = month_view ();
  future = future_view ();
  year = year_view ();

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), day, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), week, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), month, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), future, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), year, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (toolbar);

  new_view (day);
  
  gtk_main ();

  return 0;
}
