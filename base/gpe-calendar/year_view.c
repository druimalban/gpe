/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#include <langinfo.h>

#include <gtk/gtk.h>

#include "globals.h"
#include "year_view.h"
#include "event-db.h"
#include "gtkdatesel.h"

static GtkWidget *g_table;
static GtkWidget *cal[12];

static const nl_item months[] = { MON_1, MON_2, MON_3, MON_4,
				  MON_5, MON_6, MON_7, MON_8,
				  MON_9, MON_10, MON_11, MON_12 };

static unsigned int day_event_bits[(366 / sizeof(unsigned int)) + 1];

static guint
day_of_week(guint year, guint month, guint day)
{
  guint result;

  if (month < 3) 
    {
      month += 12;
      --year;
    }

  result = day + (13 * month - 27)/5 + year + year/4
    - year/100 + year/400;
  return ((result + 6) % 7);
}


static void
year_view_update (void)
{
  guint i;
  time_t basetime;
  struct tm *tm = localtime (&viewtime);
  tm->tm_mday = 1;
  tm->tm_mon = 0;
  tm->tm_yday = 0;
  tm->tm_hour = 0;
  tm->tm_min = 0;
  tm->tm_sec = 0;
  basetime = mktime (tm);

  memset (day_event_bits, 0, sizeof (day_event_bits));

  for (i = 0; i < 367; i++)
    {
      GSList *l = event_db_list_for_period (basetime + (i * SECONDS_IN_DAY),
					    basetime + ((i + 1) * SECONDS_IN_DAY) - 1);
      if (l)
	{
	  event_db_list_destroy (l);
	  day_event_bits[i / sizeof(unsigned int)] |= 1 << (i % sizeof (unsigned int));
	}
    }

  for (i = 0; i < 12; i++)
    {
      gtk_calendar_freeze (GTK_CALENDAR (cal[i]));
      gtk_calendar_select_month (GTK_CALENDAR (cal[i]),
				 i, tm->tm_year + 1900);
      gtk_calendar_clear_marks (GTK_CALENDAR (cal[i]));
      gtk_calendar_thaw (GTK_CALENDAR (cal[i]));
    }

  gtk_widget_draw (g_table, NULL);
}

GtkWidget *
year_view(void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new (GTKDATESEL_YEAR);
  GtkWidget *table = gtk_table_new (12, 2, FALSE);
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *label;
  guint i;
  
  for (i = 0; i < 12; i++)
    {
      guint x = i % 2, y = (i / 2) * 2;
      cal[i] = gtk_calendar_new ();
      gtk_calendar_display_options (GTK_CALENDAR (cal[i]), week_starts_monday ? GTK_CALENDAR_WEEK_START_MONDAY : 0);
      gtk_table_attach_defaults (GTK_TABLE (table),
				 cal[i],
				 x, x+1,
				 y+1, y+2);
      label = gtk_label_new (nl_langinfo(months[i]));
      gtk_table_attach_defaults (GTK_TABLE (table),
				 label,
				 x, x+1,
				 y, y+1);      
    }

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled), 
					 table);

  gtk_box_pack_start (GTK_BOX (hbox), datesel, TRUE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) year_view_update);

  g_table = table;

  return vbox;
}
