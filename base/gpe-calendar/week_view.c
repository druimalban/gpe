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

#include "gtkdatesel.h"
#include "globals.h"
#include "event-db.h"
#include "week_view.h"

static GtkWidget *week_view_draw;
static struct tm today;
static guint min_cell_height = 38;

struct week_day
{
  guint y;
  gchar *string;
  GSList *events;
  gboolean is_today;
} week_days[7];

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  GdkGC *red_gc;
  guint max_width;
  guint max_height;
  guint day;
  GdkColor red;
  GdkColormap *colormap;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  red_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (red_gc, widget->style->black_gc);
  colormap = gdk_window_get_colormap (widget->window);
  red.red = 0xffff;
  red.green = 0;
  red.blue = 0;
  gdk_colormap_alloc_color (colormap, &red, FALSE, TRUE);
  gdk_gc_set_foreground (red_gc, &red);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  gdk_gc_set_clip_rectangle (black_gc, &event->area);
  gdk_gc_set_clip_rectangle (red_gc, &event->area);
  gdk_gc_set_clip_rectangle (gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (white_gc, &event->area);

  for (day = 0; day < 7; day++)
    {
      guint x, y = week_days[day].y;
      GSList *iter;

      if (day)
	gdk_draw_line (drawable, black_gc, 0, y, max_width, y);

      gdk_draw_rectangle (drawable, white_gc, 
			  TRUE, 0, y + 1, max_width, max_height);

      x = max_width - gdk_string_width (datefont, week_days[day].string) - 8;
      gdk_draw_text (drawable, datefont,
		     week_days[day].is_today ? red_gc : black_gc,
		    x, y + datefont->ascent + 1, week_days[day].string, 
		     strlen (week_days[day].string));

      y += datefont->ascent + datefont->descent;

      if (week_days[day].events)
	{
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
	      GdkFont *font = widget->style->font;
	      char buf[256];
	      struct tm tm;
	      char *p = buf;
	      event_t ev = iter->data;
	      event_details_t evd = event_db_get_details (ev);
	      size_t s = sizeof (buf), l;
	      localtime_r (&ev->start, &tm);
	      l = strftime (p, s, TIMEFMT, &tm);
	      s -= l;
	      p += l;
	      snprintf (p, s - 1, " %s", evd->summary);
	      p[s - 1] = 0;
	      
	      gdk_draw_text (drawable, font, black_gc,
			     4, y + font->ascent,
			     buf, strlen (buf));
	      y += font->ascent + font->descent;
	    }
	}
    }

  gdk_gc_unref (red_gc);

  return TRUE;
}

static void
week_view_update (void)
{
  guint day;
  time_t t = time (NULL);
  struct tm tm;
  guint y = 0;

  localtime_r (&t, &today);

  t = viewtime;
  localtime_r (&t, &tm);
#ifdef WEEK_STARTS_ON_MONDAY
  tm.tm_wday = (tm.tm_wday + 6) % 7;
#endif
  t -= SECONDS_IN_DAY * tm.tm_wday;

  for (day = 0; day < 7; day++)
    {
      char buf[32];
      GSList *iter;      
      guint height = datefont->ascent + datefont->descent;

      week_days[day].events = event_db_list_for_period (t, t + SECONDS_IN_DAY - 1);
      week_days[day].y = y;

      localtime_r (&t, &tm);
      week_days[day].is_today = (tm.tm_mday == today.tm_mday 
		       && tm.tm_mon == today.tm_mon 
		       && tm.tm_year == today.tm_year);
      strftime(buf, sizeof (buf), "%a %d %B", &tm);
      if (week_days[day].string)
	g_free (week_days[day].string);
      week_days[day].string = g_strdup (buf);

      t += SECONDS_IN_DAY;

      if (week_days[day].events)
	{
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    height += week_view_draw->style->font->ascent + 
	      week_view_draw->style->font->descent;
	}

      if (height < min_cell_height)
	height = min_cell_height;

      y += height;
    }

  gtk_widget_set_usize (week_view_draw, -1, y);

  gtk_widget_draw (week_view_draw, NULL);
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *entry)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));

  week_view_update ();
}

GtkWidget *
week_view(void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *draw = gtk_drawing_area_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new (GTKDATESEL_WEEK);
  GtkWidget *scroller = gtk_scrolled_window_new (NULL, NULL);

  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      datesel);
  
  gtk_box_pack_start (GTK_BOX (hbox), datesel, TRUE, FALSE, 0);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroller), draw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);

  gtk_signal_connect(GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), NULL);

  week_view_draw = draw;

  week_view_update ();

  return vbox;
}
