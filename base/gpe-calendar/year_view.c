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

static GtkWidget *g_draw;

static const nl_item months[] = { ABMON_1, ABMON_2, ABMON_3, ABMON_4,
				  ABMON_5, ABMON_6, ABMON_7, ABMON_8,
				  ABMON_9, ABMON_10, ABMON_11, ABMON_12 };

static unsigned int day_event_bits[(366 / sizeof(unsigned int)) + 1];

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   user_data)
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
  guint xi, yi;
  gint dbias;
  guint year;
  time_t t;
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

  gdk_draw_rectangle (drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  for (yi = 0; yi < 4; yi++)
    {
      for (xi = 0; xi < 3; xi++)
	{
	  guint xd, yd;
	  guint m = yi * 3 + xi;
	  const char *s = nl_langinfo(months[m]);
	  guint x = (xi * 78);
	  guint y = (yi * 72) + yearfont->ascent + 2;

	  /* Draw the month name at a 90-degree angle */
	  {
	    GdkColor black, white;
	    GdkPixmap *pixmap;
	    GdkGC *rotgc;
	    GdkImage *image;
	    
	    int lbearing, rbearing, width, ascent, descent, height;
	    int i, j;
	    
	    gdk_text_extents (timefont, s, strlen (s),
			      &lbearing, &rbearing,
			      &width, &ascent, &descent);
	    
	    height = ascent + descent;
	    
	    /* draw text into pixmap */
	    pixmap = gdk_pixmap_new (drawable, width, height, 1);
	    rotgc = gdk_gc_new (pixmap);
	    gdk_gc_set_font (rotgc, timefont);
	    
	    white.pixel = gdk_rgb_xpixel_from_rgb (0xffffffff);
	    black.pixel = gdk_rgb_xpixel_from_rgb (0);

	    gdk_gc_set_foreground (rotgc, &white);
	    gdk_draw_rectangle (pixmap, rotgc, 1, 0, 0, width, height);
	    
	    gdk_gc_set_foreground (rotgc, &black);
	    gdk_draw_text (pixmap, timefont, rotgc, 0, ascent, s, 
			   strlen (s));
	    image = gdk_image_get (pixmap, 0, 0, width, height);

	    for (i = 0; i < width; i++)
	      {
		for (j = 0; j < height; j++)
		  {
		    if (gdk_image_get_pixel (image, width - i - 1, j) 
			== black.pixel)
		      gdk_draw_point (drawable, black_gc, 
				      8 - (height / 2 ) + x + j, 
				      28 - (width / 2 ) + y + i);
		  }
	      }
	    
	    gdk_pixmap_unref (pixmap);
	    gdk_gc_unref (rotgc);
	  }

	  /* Draw the weekend highlights */
	  gdk_draw_rectangle (drawable, grey_gc, 1,
			      x + 16, y + (4 * 9),
			      (12 * 5), (2 * 9));

	  /* Draw the days */
	  for (xd = 0; xd < 5; xd++)
	    {
	      guint xp = x + 16 + (xd * 12);
			     
	      for (yd = 0; yd < 7; yd++)
		{
		  char buf[3];
		  guint w;
		  guint yp = y + (yd * 9);
		  guint d = 1 + (xd * 7) + yd - dbias;
		  GdkGC *fg_gc = black_gc;

		  if (d <= 0 || d > days[m])
		    continue;

		  if (day_event_bits[dn / sizeof(unsigned int)] & (1 << (dn % sizeof(unsigned int))))
		    {
		      gdk_draw_rectangle (drawable, black_gc, 1, xp, yp - 9,
					  12, 9);
		      fg_gc = white_gc;
		    }

		  snprintf (buf, sizeof (buf), "%d", d);
		  w = gdk_string_width (yearfont, buf);
		  gdk_draw_text (drawable, yearfont, 
				 (d == today.tm_mday 
				  && m == today.tm_mon 
				  && tm.tm_year == today.tm_year) ? red_gc : fg_gc,
				 xp + 6 - (w / 2), yp, buf, strlen (buf));

		  dn++;
		}
	    }

	  dbias = (dbias + days[m]) % 7;
	}
    }

  gdk_gc_unref (grey_gc);

  return TRUE;
}

static void
year_view_update (void)
{
  guint i;
  time_t basetime;
  struct tm *tm = localtime (&viewtime);
  tm->tm_mday = 0;
  tm->tm_mon = 0;
  tm->tm_yday = 0;
  tm->tm_hour = 0;
  tm->tm_min = 0;
  tm->tm_sec = 0;
  basetime = mktime (tm);

  for (i = 0; i < 367; i++)
    {
      GSList *l = event_db_list_for_period (basetime + (i * 24 * 60 * 60),
					    basetime + 
					    ((i + 1) * 24 * 60 * 60) - 1);
      if (l)
	{
	  g_slist_free (l);
	  day_event_bits[(i-1) / sizeof(unsigned int)] |= 1 << ((i-1) % sizeof (unsigned int));
	}
      else
	day_event_bits[(i-1) / sizeof(unsigned int)] &= ~(1 << ((i-1) % sizeof (unsigned int)));
    }

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
  GtkWidget *draw = gtk_drawing_area_new ();
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *datesel = gtk_date_sel_new (GTKDATESEL_YEAR);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_drawing_area_size (GTK_DRAWING_AREA (draw), 240, 14 * 20);
  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), draw);

  gtk_box_pack_start (GTK_BOX (hbox), datesel, TRUE, FALSE, 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_window), draw);
  
  gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) year_view_update);

  g_draw = draw;

  return vbox;
}
