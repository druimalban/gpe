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
#include <glib.h>

#include "event-db.h"
#include "gtkdatesel.h"
#include "globals.h"
#include "day_view.h"

extern GtkWidget *new_event(time_t t, guint timesel);
extern GdkFont *timefont, *datefont;

static GtkWidget *g_draw;

static GSList *day_events[24];

guint bias = 8;

#define SECONDS_IN_DAY (24*60*60)

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  guint max_width;
  guint max_height;
  guint hour, y;
  guint c = gdk_string_width(timefont, "88am") + 4;
  struct tm tm;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  gdk_draw_rectangle(drawable, black_gc, TRUE, 0, 0, c, max_height);

  y = 0;

  for (hour = bias; hour <= 23; hour++)
    {
      GSList *iter;
      char buf[128];
      guint cell_height = 22, yoff = 0;

      if (hour & 1)
	gdk_draw_rectangle (drawable, gray_gc, TRUE, c + 1, y, max_width, max_height);
      else
	gdk_draw_rectangle (drawable, white_gc, TRUE, c + 1, y, max_width, max_height);

      for (iter = day_events[hour]; iter; iter = iter->next)
	{
	  event_t ev = (event_t) iter->data;
	  event_details_t ev_d = event_db_get_details (ev);
	  guint xoff = c + 1;

	  localtime_r (&ev->start, &tm);
	  strftime (buf, sizeof(buf), "%R", &tm);
	  gdk_draw_text (drawable, datefont, black_gc, xoff,
			 y + yoff + datefont->ascent, buf,
			 strlen (buf));
	  xoff += gdk_string_width (datefont, buf);
	  if (ev->duration)
	    {
	      time_t et = ev->start + ev->duration;
	      GdkPoint points[3];
	      struct tm *tm = localtime (&et);
	      strftime (buf, sizeof(buf), "%R", tm);

	      points[0].x = xoff + 6;
	      points[1].x = xoff + 6;
	      points[2].x = xoff + 11;
	      points[0].y = y + yoff;
	      points[1].y = y + yoff + datefont->ascent;
	      points[2].y = y + yoff + datefont->ascent / 2;

	      gdk_draw_line (drawable, black_gc, xoff + 2, 
			     y + yoff + datefont->ascent / 2,
			     xoff + 6,
			     y + yoff + (datefont->ascent / 2));
	      gdk_draw_polygon (drawable, black_gc, TRUE,
				points, 3);

	      gdk_draw_text (drawable, datefont, black_gc, xoff + 13,
			     y + yoff + datefont->ascent, buf,
			     strlen (buf));
	      xoff += 8 + gdk_string_width (datefont, buf);
	    }

	  yoff += datefont->ascent + datefont->descent;

	  gdk_draw_text (drawable, datefont, black_gc, c + 1,
			 y + yoff + datefont->ascent, ev_d->description,
			 strlen (ev_d->description));

	  yoff += datefont->ascent + datefont->descent;

	  if (cell_height < yoff)
	    cell_height = yoff;
	}

      tm.tm_hour = hour;
      strftime (buf, sizeof (buf), "%l%p", &tm);
      gdk_draw_text (drawable, timefont, white_gc,
		     0, y + timefont->ascent + 6, 
		     buf, strlen (buf));

      y += cell_height;
    }

  gdk_draw_line (drawable, black_gc, 0, max_height - 1, max_width, 
		 max_height - 1);

  return TRUE;
}

static void
day_view_update(void)
{
  unsigned int i; 
  time_t basetime;
  struct tm *tm = localtime (&viewtime);
  tm->tm_hour = 0;
  tm->tm_min = 0;
  tm->tm_sec = 0;
  basetime = mktime (tm);

  for (i = 0; i < 24; i++)
    {
      if (day_events[i])
	g_slist_free (day_events[i]);

      day_events[i] = event_db_list_for_period (basetime + (i * 60 * 60),
						basetime + 
						((i + 1) * 60 * 60) - 1);
    }

  /* flush out duplicates */
  for (i = 0; i < 23; i++)
    {
      GSList *iter;
      
      for (iter = day_events[i]; iter; iter = g_slist_next (iter))
	{
	  guint j;
	  for (j = i + 1; j < 24; j++)
	    day_events[j] = g_slist_remove (day_events[j], iter->data);
	}
    }

  gtk_widget_draw (g_draw, NULL);
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *draw)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  day_view_update ();
}

static void
click_callback (GtkWidget *widget,
		GdkEventButton *event,
		GtkWidget *datesel)
{
  if (event->type == GDK_2BUTTON_PRESS)
    {
      guint hour = 8 + (event->y / 20);
      time_t t = gtk_date_sel_get_time (GTK_DATE_SEL (datesel));
      GtkWidget *appt;
      struct tm tm;
      localtime_r (&t, &tm);
      tm.tm_hour = hour;
      tm.tm_min = 0;
      appt = new_event (mktime (&tm), 1);
      gtk_widget_show (appt);
    }
}

static void
size_allocate_callback(GtkWidget *widget,
		       GtkAllocation *allocation,
		       GtkAdjustment *adj)
{
  adj->page_size = allocation->height / 22;
  gtk_adjustment_changed (adj);
}

static void
scroll_dragged(GtkAdjustment *adj,
	       GtkWidget *draw)
{
  bias = adj->value;
  gtk_widget_draw (draw, NULL);
}

GtkWidget *
day_view(void)
{
  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  GtkWidget *draw = gtk_drawing_area_new ();
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
  GtkWidget *hbox2 = gtk_hbox_new(FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new (GTKDATESEL_FULL);
  GtkObject *adj = gtk_adjustment_new (bias, 0.0, 24.0, 1.0, 1.0, 1.0);
  GtkWidget *scroll = gtk_vscrollbar_new(GTK_ADJUSTMENT (adj));

  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);
  
  gtk_box_pack_start (GTK_BOX (hbox), datesel, TRUE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox2), draw, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), scroll, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);

  gtk_signal_connect(GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), draw);

  gtk_signal_connect (GTK_OBJECT (draw), "button_press_event",
		      GTK_SIGNAL_FUNC (click_callback), datesel);

  gtk_signal_connect (GTK_OBJECT (draw), "size-allocate",
		      GTK_SIGNAL_FUNC (size_allocate_callback), adj);

  gtk_signal_connect (GTK_OBJECT (adj), "value-changed",
		      GTK_SIGNAL_FUNC (scroll_dragged), draw);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK);

  g_draw = draw;

  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) day_view_update);

  return vbox;
}
