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
#include <langinfo.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "event-db.h"
#include "event-ui.h"
#include "gtkdatesel.h"
#include "globals.h"
#include "month_view.h"

static GtkWidget *datesel, *draw;
static guint xp, xs, ys;

struct render_ctl
{
  gboolean valid;
  guint nr;
  gboolean events;
};

static struct render_ctl rc[35];

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  guint width, height;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  guint i, j;
  GdkFont *font = widget->style->font;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  
  gdk_gc_set_clip_rectangle (black_gc, &event->area);
  gdk_gc_set_clip_rectangle (gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (white_gc, &event->area);
  
  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_window_clear_area (drawable, 
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);

  gdk_draw_rectangle (drawable, black_gc, 1,
		      0, ys / 2,
		      width,
		      ys / 2);

  for (i = 0; i < 7; i++)
    {
      guint x = xp + (i * xs);
      static const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, 
				      ABDAY_6, ABDAY_7, ABDAY_1 };
      gchar *s = nl_langinfo (days[i]);
      guint w = gdk_string_width (font, s);
      gdk_draw_text (drawable, font, white_gc,
		     x + (xs - w) / 2, ys - font->descent,
		     s, strlen (s));
 
      for (j = 0; j < 5; j++)
	{
	  guint d = i + (7 * j);
	  struct render_ctl *c = &rc[d];
	  guint y = (j + 1) * ys;

	  if (c->valid)
	    {
	      char buf[10];
	      guint w;
	      
	      gdk_draw_rectangle (drawable, white_gc, TRUE,
				  x, y, xs, ys);
	      
	      gdk_draw_rectangle (drawable, black_gc, FALSE,
				  x, y, xs, ys);
	      
	      snprintf (buf, sizeof (buf), "%d", c->nr);
	      w = gdk_string_width (font, buf);
	      
	      gdk_draw_text (drawable, font, black_gc, 
			     x + (xs - w) / 2, y + (ys / 2) + font->ascent,
			     buf, strlen (buf));
	    }
	}
    }

  gdk_gc_set_clip_rectangle (black_gc, NULL);
  gdk_gc_set_clip_rectangle (gray_gc, NULL);
  gdk_gc_set_clip_rectangle (white_gc, NULL);

  return TRUE;
}

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

static gint
month_view_update ()
{
  guint day;
  time_t start, end;
  struct tm tm_start, tm_end;
  GSList *day_events[32];
  GSList *iter;
  guint days;
  guint year, month;
  guint wday;

  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
  gtk_widget_draw (datesel, NULL);
      
  localtime_r (&viewtime, &tm_start);
  year = tm_start.tm_year + 1900;
  month = tm_start.tm_mon;

  days = days_in_month (year, month);

  for (day = 1; day <= days; day++)
    {
      localtime_r (&viewtime, &tm_start);

      tm_start.tm_mday = day;
      tm_start.tm_hour = 0;
      tm_start.tm_min = 0;
      tm_start.tm_sec = 0;
      start = mktime (&tm_start);
      localtime_r (&viewtime, &tm_end);
      tm_end.tm_mday = day;
      tm_end.tm_hour = 23;
      tm_end.tm_min = 59;
      tm_end.tm_sec = 59;
      end = mktime (&tm_end);

      day_events[day] = event_db_list_for_period (start, end);
      
      for (iter = day_events[day]; iter; iter = iter->next)
	((event_t)iter->data)->mark = FALSE;
    }

  wday = 1 - day_of_week(year, month + 1, 1);
  for (day = 0; day < 35; day++)
    {
      gint rday = day + wday;
      struct render_ctl *c = &rc[day];
      c->nr = rday;
      if (rday <= 0 || rday > days)
	{
	  c->valid = FALSE;
	}
      else
	{
	  c->valid = TRUE;
	  c->events = day_events[rday] ? TRUE : FALSE;
	}
    }

  gtk_widget_draw (draw, NULL);
  
  return TRUE;
}

static void
changed_callback(GtkWidget *widget,
		 gpointer d)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  month_view_update ();
}

static void
resize_table(GtkWidget *widget,
	     gpointer d)
{
  static guint old_width, old_height;
  guint width = widget->allocation.width,
    height = widget->allocation.height;

  if (width != old_width || height != old_height)
    {
      old_width = width;
      old_height = height;

      xs = width / 7;
      ys = height / 6;

      if (ys > xs) 
	ys = xs;

      gtk_widget_draw (draw, NULL);
    }
}

GtkWidget *
month_view(void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);

  draw = gtk_drawing_area_new ();
  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);

  datesel = gtk_date_sel_new (GTKDATESEL_MONTH);
  
  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), draw, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT (draw), "size-allocate",
		     GTK_SIGNAL_FUNC (resize_table), NULL);

  gtk_signal_connect(GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), NULL);
  
  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) month_view_update);

  return vbox;
}
