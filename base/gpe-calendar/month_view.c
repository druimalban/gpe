
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
#include <gpe/gtkdatesel.h>
#include "globals.h"
#include "month_view.h"
#include "day_popup.h"

static GtkWidget *datesel, *draw;
static guint xp, xs, ys;
static struct render_ctl *c_old;

struct render_ctl
{
  struct day_popup popup;
  gboolean valid;
  gboolean today;
};

#define TOTAL_DAYS (6 * 7)

static struct render_ctl rc[TOTAL_DAYS];

static gboolean
button_press (GtkWidget *widget,
	      GdkEventButton *event,
	      gpointer d)
{
  guint x = event->x;
  guint y = event->y;
  struct render_ctl *c;

  if (x < xp || y < ys)
    return FALSE;

  x -= xp;
  x /= xs;

  y /= ys;
  y -= 1;

  c = &rc[x + (y * 7)];
  if (c->valid) 
    {
      if (event->type == GDK_BUTTON_PRESS)
	{
	  if (pop_window) 
	    gtk_widget_destroy (pop_window);

	  if (c != c_old) 
	    {
	      pop_window = day_popup (main_window, &c->popup);
	      c_old = c;
	    }
	  else 
	    {
	      pop_window = NULL;
	      c_old = NULL;
	    }
	}
      
      else if (event->type == GDK_2BUTTON_PRESS)
	{
	  struct tm tm;
	  localtime_r (&viewtime, &tm);
	  tm.tm_year = c->popup.year - 1900;
	  tm.tm_mon = c->popup.month;
	  tm.tm_mday = c->popup.day;
	  viewtime = mktime (&tm);
	  if (pop_window) 
	    gtk_widget_destroy (pop_window);
	  set_day_view ();    
	}
    }
  
  return TRUE;
}

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
  GdkGC *cream_gc;
  GdkGC *light_gray_gc;
  GdkGC *dark_gray_gc;
  GdkGC *red_gc;
  GdkColor cream;
  GdkColor light_gray;
  GdkColor dark_gray;
  GdkColor red;
  GdkColormap *colormap;
  guint i, j;
#if GTK_MAJOR_VERSION < 2
  GdkFont *font = widget->style->font;
#else
  PangoContext *pc;
  PangoLayout *pl;
  PangoRectangle pr;
#endif

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  colormap = gdk_window_get_colormap (widget->window);
  cream_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (cream_gc, widget->style->black_gc);
  cream.red = 65535;
  cream.green = 64005;
  cream.blue = 61200;
  gdk_colormap_alloc_color (colormap, &cream, FALSE, TRUE);
  gdk_gc_set_foreground (cream_gc, &cream);

  light_gray_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (light_gray_gc, widget->style->black_gc);
  light_gray.red = 53040;
  light_gray.green = 53040;
  light_gray.blue = 53040;
  gdk_colormap_alloc_color (colormap, &light_gray, FALSE, TRUE);
  gdk_gc_set_foreground (light_gray_gc, &light_gray);

  red_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (red_gc, widget->style->black_gc);
  red.red = 65535;
  red.green = 0;
  red.blue = 0;
  gdk_colormap_alloc_color (colormap, &red, FALSE, TRUE);
  gdk_gc_set_foreground (red_gc, &red);

  dark_gray_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (dark_gray_gc, widget->style->black_gc);
  gdk_color_parse ("gray40", &dark_gray);
  gdk_colormap_alloc_color (colormap, &dark_gray, FALSE, TRUE);
  gdk_gc_set_foreground (dark_gray_gc, &dark_gray);

  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  
  gdk_gc_set_clip_rectangle (black_gc, &event->area);
  gdk_gc_set_clip_rectangle (gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (white_gc, &event->area);
  gdk_gc_set_clip_rectangle (cream_gc, &event->area);
  gdk_gc_set_clip_rectangle (light_gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (dark_gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (red_gc, &event->area);

#if GTK_MAJOR_VERSION >= 2
  pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
#endif
  
  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_window_clear_area (drawable, 
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);

  gdk_draw_rectangle (drawable, light_gray_gc, 1,
		      0, ys / 2,
		      width,
		      ys / 2);

  for (i = 0; i < 7; i++)
    {
      guint x = xp + (i * xs);
      static const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, 
				      ABDAY_6, ABDAY_7, ABDAY_1 };
      gchar *s = nl_langinfo (days[i]);
      guint w;
#if GTK_MAJOR_VERSION < 2
      w = gdk_string_width (font, s);
      gdk_draw_text (drawable, font, black_gc,
		     x + (xs - w) / 2, ys - font->descent,
		     s, strlen (s));
#else
      pango_layout_set_text (pl, s, strlen (s));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      w = pr.width;
      
      gtk_paint_layout (widget->style,
			widget->window,
			GTK_WIDGET_STATE (widget),
			FALSE,
			&event->area,
			widget,
			"label",
			x + (xs - w) / 2, ys / 2,
			pl);
#endif
 
      for (j = 0; j < (TOTAL_DAYS / 7); j++)
	{
	  guint d = i + (7 * j);
	  struct render_ctl *c = &rc[d];
	  guint y = (j + 1) * ys;

	  if (c->valid)
	    {
	      char buf[10];
	      guint w;
	      
	      gdk_draw_rectangle (drawable, c->popup.events ? dark_gray_gc : cream_gc, 
				  TRUE,
				  x, y, xs, ys);
	      
	      gdk_draw_rectangle (drawable, light_gray_gc, FALSE,
				  x, y, xs, ys);
	      
	      snprintf (buf, sizeof (buf), "%d", c->popup.day);
#if GTK_MAJOR_VERSION < 2
	      w = gdk_string_width (font, buf);
	      
	      gdk_draw_text (drawable, font, c->popup.events ? white_gc : black_gc, 
			     x + (xs - w) / 2, y + (ys / 2) + font->ascent,
			     buf, strlen (buf));
#else
	      pango_layout_set_text (pl, buf, strlen (buf));
	      pango_layout_get_pixel_extents (pl, &pr, NULL);
	      w = pr.width;
	      gtk_paint_layout (widget->style,
				widget->window,
				GTK_WIDGET_STATE (widget),
				FALSE,
				&event->area,
				widget,
				"label",
				x + (xs - w) / 2, y + (ys / 2)/* + font->ascent*/,
				pl);
#endif
	    } 
	  else 
	    {	      
	      gdk_draw_rectangle (drawable, gray_gc, 
				  TRUE,
				  x, y, xs, ys);
	      
	      gdk_draw_rectangle (drawable, light_gray_gc, FALSE,
				  x, y, xs, ys);
	    }

	}
    }

  for (i = 0; i < 7; i++)
    {
      guint x = xp + (i * xs);
      for (j = 0; j < (TOTAL_DAYS / 7); j++)
	{
	  guint d = i + (7 * j);
	  struct render_ctl *c = &rc[d];
	  guint y = (j + 1) * ys;

	  if (c->today)
	    {
	      gdk_draw_rectangle (drawable, red_gc, FALSE,
				  x, y, xs, ys);
	      gdk_draw_rectangle (drawable, red_gc, FALSE,
				  x + 1, y + 1, xs - 2, ys - 2);
	    }
	}
    }

  gdk_gc_set_clip_rectangle (black_gc, NULL);
  gdk_gc_set_clip_rectangle (gray_gc, NULL);
  gdk_gc_set_clip_rectangle (white_gc, NULL);

  gdk_gc_unref (red_gc);
  gdk_gc_unref (cream_gc);
  gdk_gc_unref (light_gray_gc);
  gdk_gc_unref (dark_gray_gc);

#if GTK_MAJOR_VERSION >= 2
  g_object_unref (pl);
#endif

  return TRUE;
}

guint
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
  struct tm today;
  time_t t;

  time (&t);
  localtime_r (&t, &today);

  localtime_r (&viewtime, &tm_start);
  year = tm_start.tm_year + 1900;
  month = tm_start.tm_mon;

  days = days_in_month (year, month);

  for (day = 0; day < TOTAL_DAYS; day++)
    {
      struct render_ctl *c = &rc[day];
      if (c->popup.events)
	{
	  event_db_list_destroy (c->popup.events);
	  c->popup.events = NULL;
	}
    }

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

      if (!tm_start.tm_isdst) {
	      start+=60*60;
	      end+=60*60;
      }
      
      day_events[day] = event_db_list_for_period (start, end);
      
      for (iter = day_events[day]; iter; iter = iter->next)
	((event_t)iter->data)->mark = FALSE;
    }

  wday = 1 - day_of_week(year, month + 1, 1);
  for (day = 0; day < TOTAL_DAYS; day++)
    {
      gint rday = day + wday;
      struct render_ctl *c = &rc[day];
      c->popup.day = rday;
      c->popup.year = year;
      c->popup.month = month;
      if (rday <= 0 || rday > days)
	{
	  c->valid = FALSE;
	}
      else
	{
	  c->valid = TRUE;
	  c->popup.events = day_events[rday];
	}
      
      c->today = ((year == today.tm_year + 1900
		   && month == today.tm_mon
		   && rday == today.tm_mday)) ? TRUE : FALSE;
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
update_hook_callback()
{
  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
  gtk_widget_draw (datesel, NULL);
      
  month_view_update ();
}

static void
resize_table (GtkWidget *widget,
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
      ys = height / 7;

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
  gtk_widget_show (draw);
  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);

  datesel = gtk_date_sel_new (GTKDATESEL_MONTH);
  gtk_widget_show (datesel);
  
  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), draw, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT (draw), "size-allocate",
		     GTK_SIGNAL_FUNC (resize_table), NULL);

  gtk_signal_connect(GTK_OBJECT (draw), "button-press-event",
		     GTK_SIGNAL_FUNC (button_press), NULL);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK);

  gtk_signal_connect(GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), NULL);
  
  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) update_hook_callback);

  return vbox;
}
