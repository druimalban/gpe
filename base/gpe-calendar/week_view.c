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

GdkGC *red_gc;
extern GdkFont *datefont, *timefont;

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
  guint day, y;
  guint h;
  guint cell_height = 35;
  struct tm tm, today;
  time_t t;
  /*guint year, mon, baseday;*/

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  if (red_gc == NULL)
    {
      GdkColor red;
      GdkColormap *colormap;

      red_gc = gdk_gc_new(widget->window);
      gdk_gc_copy(red_gc, widget->style->black_gc);
      colormap = gdk_window_get_colormap(widget->window);
      red.red = 0xffff;
      red.green = 0;
      red.blue = 0;
      gdk_colormap_alloc_color(colormap, &red, FALSE, TRUE);
      gdk_gc_set_foreground(red_gc, &red);
    }

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  h = 7 * cell_height;
  
  time(&t);
  localtime_r(&t, &today);

  t = gtk_date_sel_get_time (GTK_DATE_SEL (user_data));
  localtime_r(&t, &tm);
  t -= 60 * 60 * 24 * tm.tm_wday;

  for (day = 0; day < 7; day++)
    {
      guint x;
      char buf[32];
      
      localtime_r (&t, &tm);

      strftime(buf, sizeof(buf), "%a %d %B", &tm);
      y = day * cell_height;
      if (day & 1)
	gdk_draw_rectangle(drawable, gray_gc, TRUE, 0, y + 1, max_width, cell_height - 2);
      else
	gdk_draw_rectangle(drawable, white_gc, TRUE, 0, y + 1, max_width, cell_height - 2);
      x = max_width - gdk_string_width(datefont, buf) - 4;
      gdk_draw_text(drawable, datefont, (tm.tm_mday == today.tm_mday && tm.tm_mon == today.tm_mon && tm.tm_year == today.tm_year) ? red_gc : black_gc,
		    x, y + timefont->ascent + 1, buf, strlen(buf));
      gdk_draw_line(drawable, black_gc, 0, y + cell_height, max_width, y + cell_height);

      t += 60 * 60 * 24;
    }

  return TRUE;
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *entry)
{
  gtk_widget_draw(entry, NULL);
}

GtkWidget *
week_view(void)
{
  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  GtkWidget *draw = gtk_drawing_area_new();
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new(1);

  gtk_drawing_area_size(GTK_DRAWING_AREA(draw), 240, 13 * 20);
  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      datesel);
  
  gtk_box_pack_start(GTK_BOX(hbox), datesel, TRUE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), draw, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

  gtk_signal_connect(GTK_OBJECT(datesel), "changed",
		     GTK_SIGNAL_FUNC(changed_callback), draw);

  return vbox;
}
