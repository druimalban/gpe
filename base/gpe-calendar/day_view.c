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

#include "gtkdatesel.h"
#include "cotidie.h"

extern GtkWidget *new_event(time_t t, guint timesel);
extern GdkFont *timefont, *datefont;

guint cell_height = 22;
guint bias = 8;

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
  guint c = gdk_string_width(timefont, "88:88") + 4;

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

  for (hour = 0; hour <= 23; hour++)
    {
      char buf[32];
      g_snprintf(buf, sizeof(buf), "%02d:00", hour);
      y = (hour - bias) * cell_height;
      gdk_draw_line(drawable, black_gc, 0, y, max_width, y);
      gdk_draw_text(drawable, timefont, white_gc,
		    0, y + timefont->ascent + (cell_height - 12) / 2, buf, strlen(buf));
      if (hour & 1)
	gdk_draw_rectangle(drawable, gray_gc, TRUE, c + 1, y, max_width, cell_height - 1);
      else
	gdk_draw_rectangle(drawable, white_gc, TRUE, c + 1, y, max_width, cell_height - 1);
    }

  gdk_draw_line(drawable, black_gc, 0, max_height - 1, max_width, max_height - 1);

  return TRUE;
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *entry)
{
  gtk_widget_draw(entry, NULL);
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
  adj->page_size = allocation->height / cell_height;
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
  GtkWidget *draw = gtk_drawing_area_new();
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
  GtkWidget *hbox2 = gtk_hbox_new(FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new(0);
  GtkObject *adj = gtk_adjustment_new (bias, 0.0, 24.0, 1.0, 1.0, 1.0);
  GtkWidget *scroll = gtk_vscrollbar_new(GTK_ADJUSTMENT (adj));

  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);
  
  gtk_box_pack_start (GTK_BOX (hbox), datesel, TRUE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox2), draw, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), scroll, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX(vbox), hbox2, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

  gtk_signal_connect(GTK_OBJECT(datesel), "changed",
		     GTK_SIGNAL_FUNC(changed_callback), draw);

  gtk_signal_connect (GTK_OBJECT (draw), "button_press_event",
		      GTK_SIGNAL_FUNC (click_callback), datesel);

  gtk_signal_connect (GTK_OBJECT (draw), "size-allocate",
		      GTK_SIGNAL_FUNC (size_allocate_callback), adj);

  gtk_signal_connect (GTK_OBJECT (adj), "value-changed",
		      GTK_SIGNAL_FUNC (scroll_dragged), draw);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK);

  return vbox;
}

