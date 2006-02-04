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
#include <gpe/event-db.h>

#include "globals.h"
#include "year_view.h"

#include "gtkdatesel.h"

static GtkWidget *g_draw;

static const nl_item months[] = { MON_1, MON_2, MON_3, MON_4,
				  MON_5, MON_6, MON_7, MON_8,
				  MON_9, MON_10, MON_11, MON_12 };

static unsigned int day_event_bits[(366 / sizeof(unsigned int)) + 1];

static guint day_pitch, month_width, yoff;
static guint months_across, month_height, space;
static guint day_vpitch;
static guint old_x;

static void
calc_geometry (GtkWidget *widget)
{
  guint rows;
  guint x = widget->allocation.width;
  guint y;

  if (x == old_x)
    return;

  old_x = x;

  months_across = x / month_width;

  if (months_across == 0)
    months_across = 1;

  space = (x - (months_across * month_width)) / (months_across + 1);
  rows = (12 + (months_across - 1)) / months_across;
  y = rows * month_height + yoff + 4;

  if (widget->allocation.height != y)
    gtk_widget_set_usize (widget, -1, y);
}

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *grey_gc;
  GdkGC *white_gc;
  GdkGC *red_gc;
  GdkColor grey_col, red_col;
  guint max_width;
  guint max_height;
  struct tm tm, today;
  gint dbias;
  guint year;
  time_t t;
  guint m;
  guint dn = 0;

  static guint days[] = { 31, 28, 31, 30, 31, 30, 
			  31, 31, 30, 31, 30, 31 };

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  time (&t);
  localtime_r (&t, &today);

  localtime_r (&viewtime, &tm);
  dbias = tm.tm_wday - (tm.tm_yday % 7) - 1;
  if (dbias < 0) dbias += 7;

  year = tm.tm_year + 1900;
  if ((year % 4) == 0
      && ((year % 100) != 0
	  || (year % 400) == 0))
    days[1] = 29;
  else
    days[1] = 28;

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  grey_gc = gdk_gc_new (drawable);
  grey_col.pixel = gdk_rgb_xpixel_from_rgb(0xffe0e0e0);
  gdk_gc_set_foreground(grey_gc, &grey_col);

  red_gc = gdk_gc_new (drawable);
  red_col.pixel = gdk_rgb_xpixel_from_rgb(0xffff0000);
  gdk_gc_set_foreground(red_gc, &red_col);

  gdk_gc_set_clip_rectangle (black_gc, &event->area);
  gdk_gc_set_clip_rectangle (red_gc, &event->area);
  gdk_gc_set_clip_rectangle (grey_gc, &event->area);
  gdk_gc_set_clip_rectangle (white_gc, &event->area);

  gdk_draw_rectangle (drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  for (m = 0; m < 12; m++)
    {
      const char *s = nl_langinfo(months[m]);
      guint i = m % months_across, j = m / months_across;
      guint x = space + (i * (space + month_width));
      guint y = 4 + (j * month_height);
      guint w = gdk_string_width (datefont, s);
      guint d, dy;
      guint y1 = y + yoff;

      /* Draw the month label */
      gdk_draw_string (drawable, datefont, black_gc, 
		       x + (month_width / 2) - (w / 2), y + datefont->ascent, s);

      /* Draw the weekend highlight */
      gdk_draw_rectangle (drawable, grey_gc, 1,
			  x + (5 * day_pitch), y1, 2 * day_pitch, 5 * day_vpitch);

      /* Draw the days */
      dy = day_of_week (year, m + 1, 1);
      for (d = 1; d <= days[m]; d++)
	{
	  GdkGC *fg_gc = black_gc;
	  char buf[32];
	  guint x1, w;
	  snprintf (buf, sizeof (buf), "%d", d);
	  w = gdk_string_width (yearfont, buf);
	  x1 = x + (dy * day_pitch);

	  if (day_event_bits[dn / sizeof(unsigned int)] & (1 << (dn % sizeof(unsigned int))))
	    {
	      gdk_draw_rectangle (drawable, black_gc, 1, x1, y1,
				  day_pitch, day_vpitch);
	      fg_gc = white_gc;
	    }

	  if (d == today.tm_mday && m == today.tm_mon  && tm.tm_year == today.tm_year)
	    fg_gc = red_gc;

	  gdk_draw_string (drawable, yearfont, fg_gc, 
			 x1 + ((day_pitch - w) / 2), y1 + yearfont->ascent, 
			 buf);
	  if (++dy == 7)
	    {
	      dy = 0;
	      y1 += day_vpitch;
	    }
	  
	  dn++;
	}
    }

  gdk_gc_unref (grey_gc);
  gdk_gc_unref (red_gc);

  return TRUE;
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
      struct tm *newtm;
      time_t newbasetime;
      GSList *l;
      
      newbasetime=basetime + (i * SECONDS_IN_DAY);
      newtm = localtime (&newbasetime);
      
      if (!newtm->tm_isdst) newbasetime+=60*60;
            
      l = event_db_list_for_period (newbasetime, newbasetime + SECONDS_IN_DAY - 1);
      
      if (l)
	{
	  event_db_list_destroy (l);
	  day_event_bits[i / sizeof(unsigned int)] |= 1 << (i % sizeof (unsigned int));
	}
    }

  calc_geometry (g_draw);
  gtk_widget_draw (g_draw, NULL);
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *draw)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  year_view_update ();
}

GtkWidget *
year_view(void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new (GTKDATESEL_YEAR);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  char buf[32];
  guint d, maxw = 0;

  g_draw = gtk_drawing_area_new ();

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  g_signal_connect (G_OBJECT (g_draw), "expose_event",
                    G_CALLBACK (draw_expose_event), NULL);
  g_signal_connect (G_OBJECT (g_draw), "configure_event",
                    G_CALLBACK (calc_geometry), NULL);
  g_signal_connect (G_OBJECT (datesel), "changed",
                    G_CALLBACK (changed_callback), g_draw);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
                                         g_draw);

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT (vbox), "update_hook",
                     (gpointer) year_view_update);

  for (d = 10; d < 32; d++)
    {
      guint w;
      snprintf (buf, sizeof (buf), "%d", d);
      w = gdk_string_width (yearfont, buf);
      if (w > maxw)
        maxw = w;
    }

  day_pitch = maxw + 4;
  day_vpitch = yearfont->ascent + yearfont->descent;
  yoff = datefont->ascent + datefont->descent + 1;
  month_width = (day_pitch * 7) + 4;
  month_height = (day_vpitch * 5) + 4 + yoff;
  calc_geometry (g_draw);

  gtk_widget_show (g_draw);
  gtk_widget_show (datesel);
  gtk_widget_show (scrolled_window);

  return vbox;
}
