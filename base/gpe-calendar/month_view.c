/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
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
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#include <gpe/event-db.h>
#include "event-ui.h"
#include "globals.h"
#include "month_view.h"
#include "day_popup.h"

#include "gtkdatesel.h"

static GSList *day_events[32];
static GtkWidget *datesel, *draw;
static guint xp, xs, ys;
static struct render_ctl *c_old;
static guint active_day = 0;

struct render_ctl
{
  struct day_popup popup;
  gboolean valid;
  gboolean today;
  gboolean active;
  gboolean initialized;
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

  if (event->button != 1)
    return FALSE;

  if (x < xp || y < ys)
    return FALSE;

  x -= xp;
  x /= xs;

  y /= ys;
  y -= 1;

  c = &rc[x + (y * 7) - week_offset];
  if (c->valid)
    {
      if (event->type == GDK_BUTTON_PRESS)
	{
	  if (pop_window) 
	    gtk_widget_destroy (pop_window);

	  if (c != c_old) 
	    {
	      pop_window = day_popup (main_window, &c->popup, TRUE);
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
          time_t selected_time;
          localtime_r (&viewtime, &tm);
          tm.tm_year = c->popup.year;
          tm.tm_mon = c->popup.month;
          tm.tm_mday = c->popup.day;
          selected_time = mktime (&tm);
          if (pop_window) 
            gtk_widget_destroy (pop_window);
          set_time_and_day_view (selected_time);    
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
  PangoLayout *pl;
  PangoRectangle pr;
  static nl_item days[7];
  
  if (week_starts_monday) 
  {
  	  days[0] = ABDAY_2;
  	  days[1] = ABDAY_3;
  	  days[2] = ABDAY_4;
  	  days[3] = ABDAY_5;
  	  days[4] = ABDAY_6;
  	  days[5] = ABDAY_7;
  	  days[6] = ABDAY_1;
  }
  else {
  	  days[0] = ABDAY_1;
  	  days[1] = ABDAY_2;
  	  days[2] = ABDAY_3;
  	  days[3] = ABDAY_4;
  	  days[4] = ABDAY_5;
  	  days[5] = ABDAY_6;
  	  days[6] = ABDAY_7;
  }
  
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

  pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);

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
      guint w;
      guint x = xp + (i * xs);
      gchar *s = g_locale_to_utf8 (nl_langinfo (days[i]), -1,
                                   NULL, NULL, NULL);
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

      for (j = 0; j < (TOTAL_DAYS / 7); j++)
	{
	  gchar *buffer;
	  guint d = i + (7 * j) - week_offset;
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
	      buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	      pango_layout_set_text (pl, buffer, strlen (buffer));
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
	      g_free (buffer);
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
      g_free (s);
    }

  for (i = 0; i < 7; i++)
    {
      guint x = xp + (i * xs);
      for (j = 0; j < (TOTAL_DAYS / 7); j++)
	{
	  guint d = i + (7 * j) - week_offset;
	  struct render_ctl *c = &rc[d];
	  guint y = (j + 1) * ys;

	  if ((!force_today && c->active) || (force_today && c->today))
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

  g_object_unref (pl);

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
  GSList *iter;
  guint days;
  guint year, month;
  guint wday;
  struct tm today;
  time_t t;

  t = viewtime;
  localtime_r (&t, &today);

  localtime_r (&viewtime, &tm_start);
  year = tm_start.tm_year + 1900;
  month = tm_start.tm_mon;

  days = days_in_month (year, month);
  wday = 1 - day_of_week(year, month + 1, 1);

  for (day = 0; day < TOTAL_DAYS; day++)
    {
      gint rday = day + wday;
      struct render_ctl *c = &rc[day];
      if (c->popup.events)
        c->popup.events = NULL;
      if (rday == tm_start.tm_mday) active_day = day;
      c->active = (day == active_day) ? TRUE : FALSE;
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

      if (day_events[day]) event_db_list_destroy (day_events[day]);
      day_events[day] = event_db_list_for_period (start, end);

      for (iter = day_events[day]; iter; iter = iter->next)
        ((event_t)iter->data)->mark = FALSE;
    }

  for (day = 0; day < TOTAL_DAYS; day++)
    {
      gint rday = day + wday;
      struct render_ctl *c = &rc[day];
      c->popup.day = rday;
      c->popup.year = year - 1900;
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
         if (!c->initialized)
            {
              c->active = c->today;
              c->initialized = TRUE;
            } 
       if (c->active)
         active_day = day;            
    }

  gtk_widget_draw (draw, NULL);

  return TRUE;
}

static void
changed_callback(GtkWidget *widget, gpointer d)
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

static gboolean
month_view_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *user_data)
{
  struct render_ctl *c;
 
  if (k->keyval == GDK_Escape)
    {
      c = &rc[active_day];
      if (c->valid)
        {
          if (pop_window) 
            gtk_widget_destroy (pop_window);
            pop_window = NULL;
            c_old = NULL;
        }    
    }
    
  if (k->keyval == GDK_Right) 
    {
      if ((active_day < TOTAL_DAYS) && (rc[active_day+1].valid))
        {
          active_day++;
          viewtime = time_from_day(rc[active_day].popup.year,
                                   rc[active_day].popup.month,
                                   rc[active_day].popup.day);
          month_view_update();
        }
      return TRUE;
    }
    
  if (k->keyval == GDK_Left) 
    {
      if ((active_day) && (rc[active_day-1].valid))
        {
          active_day--;
          viewtime = time_from_day(rc[active_day].popup.year,
                                   rc[active_day].popup.month,
                                   rc[active_day].popup.day);
          month_view_update();
        }
      return TRUE;
    }

  if (k->keyval == GDK_Down) 
    {
      if ((active_day < TOTAL_DAYS-7) && (rc[active_day+7].valid))
        {
          active_day+=7;
          viewtime = time_from_day(rc[active_day].popup.year,
                                   rc[active_day].popup.month,
                                   rc[active_day].popup.day);
          month_view_update();
        }
      return TRUE;
    }
    
  if (k->keyval == GDK_Up) 
    {
      if ((active_day >= 7) && (rc[active_day-7].valid))
        {
          active_day -= 7;
          viewtime = time_from_day(rc[active_day].popup.year,
                                   rc[active_day].popup.month,
                                   rc[active_day].popup.day);
          month_view_update();
        }
      return TRUE;
    }
    
  if (k->keyval == GDK_space)
    {
      c = &rc[active_day];
      if (c->valid)
        {
          if (pop_window) 
            gtk_widget_destroy (pop_window);
              if (c != c_old) 
            {
              pop_window = day_popup (main_window, &c->popup, TRUE);
              c_old = c;
            }
           else 
            {
              pop_window = NULL;
              c_old = NULL;
            }
         }
         return TRUE;
     }  
     
  if (k->keyval == GDK_Return)
    {
      c = &rc[active_day];
      if (c->valid)
        {
          struct tm tm;
          time_t selected_time;
          localtime_r (&viewtime, &tm);
          tm.tm_year = c->popup.year;//- 1900;
          tm.tm_mon = c->popup.month;
          tm.tm_mday = c->popup.day;
          tm.tm_hour = 0;
          tm.tm_min = 0;
          tm.tm_sec = 0;
          selected_time = mktime (&tm);
          if (pop_window) 
              gtk_widget_destroy (pop_window);
          set_time_and_day_view (selected_time);    
        }
      return TRUE; 
    }
  
  return FALSE;
}

void
month_free_lists()
{
  guint day;
  struct tm tm_start;
  guint days;
  guint year, month;
  struct tm today;
  time_t t;

  time (&t);
  localtime_r (&t, &today);

  localtime_r (&viewtime, &tm_start);
  year = tm_start.tm_year + 1900;
  month = tm_start.tm_mon;

  days = days_in_month (year, month);

  for (day = 1; day <= days; day++)
    if (day_events[day]) 
      {
        event_db_list_destroy (day_events[day]);
        day_events[day] = NULL;
      }
}

GtkWidget *
month_view(void)
{
  int day;
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);

  draw = gtk_drawing_area_new ();
  gtk_widget_show (draw);
  g_signal_connect (G_OBJECT (draw), "expose_event",
                    G_CALLBACK (draw_expose_event), NULL);

  datesel = gtk_date_sel_new (GTKDATESEL_MONTH);
  gtk_widget_show (datesel);
  GTK_WIDGET_SET_FLAGS(datesel, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(datesel);

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), draw, TRUE, TRUE, 0);

  g_signal_connect(G_OBJECT (draw), "size-allocate",
                   G_CALLBACK (resize_table), NULL);

  g_signal_connect(G_OBJECT (draw), "button-press-event",
                   G_CALLBACK (button_press), NULL);

  gtk_widget_add_events (GTK_WIDGET (draw), 
            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (datesel), "key_press_event", 
		    G_CALLBACK (month_view_key_press_event), NULL);
            
  gtk_widget_add_events (GTK_WIDGET (datesel), 
            GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
            
  g_signal_connect(G_OBJECT (datesel), "changed",
                     G_CALLBACK (changed_callback), NULL);

  g_object_set_data (G_OBJECT (vbox), "update_hook",
                     (gpointer) update_hook_callback);
                     
  for (day = 0; day < TOTAL_DAYS; day++)
      rc[day].initialized = FALSE;
  
  g_object_set_data(G_OBJECT(main_window),"datesel-month",datesel);

  return vbox;
}
