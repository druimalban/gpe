/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Philip Blundell <philb@gnu.org>
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

static gint month_view_update (void);

static gint title_height;

static gboolean
button_press (GtkWidget *widget, GdkEventButton *event, gpointer d)
{
  guint x = event->x;
  guint y = event->y;
  struct render_ctl *c;

  if (x < xp || y < ys)
    return FALSE;

  y -= title_height;

  x -= xp;
  x /= xs;

  y /= ys;

  c = &rc[x + (y * 7) - week_offset];
  if (c->valid)
    {
      if (event->type == GDK_BUTTON_PRESS)
	{
	  if (event->button == 3)
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
	  else
	    {
	      viewtime = time_from_day (c->popup.year, c->popup.month, c->popup.day);
	      month_view_update ();
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

static nl_item days1[] = 
  { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, ABDAY_1 };
static nl_item days2[] = 
  { ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7 };

static void
calc_title_height (GtkWidget *widget)
{
  PangoLayout *pl;
  PangoRectangle pr;
  int i;
  int max_height = 0;

  pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);

  for (i = 0; i < 7; i++)
    {
      gchar *s = g_locale_to_utf8 (nl_langinfo (days1[i]), -1,
                                   NULL, NULL, NULL);
      pango_layout_set_text (pl, s, strlen (s));
      pango_layout_get_pixel_extents (pl, &pr, NULL);

      if (pr.height > max_height)
	max_height = pr.height;

      g_free (s);
    }

  title_height = max_height + 8;

  g_object_unref (pl);
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
  GdkGC *yellow_gc;
  GdkGC *salmon_gc;
  GdkGC *blue_gc;
  GdkColor cream;
  GdkColor light_gray;
  GdkColor yellow;
  GdkColor salmon;
  GdkColor blue;
  GdkColormap *colormap;
  guint i, j;
  PangoLayout *pl;
  PangoLayout *pl_evt;
  PangoRectangle pr;
  nl_item *days;

  days = week_starts_monday ? days1 : days2;
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

  blue_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (blue_gc, widget->style->black_gc);
  blue.red = 0;
  blue.green = 0;
  blue.blue = 0xffff;
  gdk_colormap_alloc_color (colormap, &blue, FALSE, TRUE);
  gdk_gc_set_foreground (blue_gc, &blue);

  yellow_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (yellow_gc, widget->style->black_gc);
  gdk_color_parse ("palegoldenrod", &yellow);
  gdk_colormap_alloc_color (colormap, &yellow, FALSE, TRUE);
  gdk_gc_set_foreground (yellow_gc, &yellow);

  salmon_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (salmon_gc, widget->style->black_gc);
  gdk_color_parse ("light salmon", &salmon);
  gdk_colormap_alloc_color (colormap, &salmon, FALSE, TRUE);
  gdk_gc_set_foreground (salmon_gc, &salmon);

  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;

  gdk_gc_set_clip_rectangle (black_gc, &event->area);
  gdk_gc_set_clip_rectangle (gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (white_gc, &event->area);
  gdk_gc_set_clip_rectangle (cream_gc, &event->area);
  gdk_gc_set_clip_rectangle (light_gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (yellow_gc, &event->area);
  gdk_gc_set_clip_rectangle (salmon_gc, &event->area);
  gdk_gc_set_clip_rectangle (blue_gc, &event->area);

  pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
  pl_evt = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_window_clear_area (drawable, 
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);

  gdk_draw_rectangle (drawable, light_gray_gc, 1,
		      0, 0, width, title_height);

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
			x + (xs - w) / 2, 1,
			pl);

      for (j = 0; j < (TOTAL_DAYS / 7); j++)
	{
	  guint height = 0;
	  gchar *buffer;
	  guint d = i + (7 * j) - week_offset;
	  struct render_ctl *c = &rc[d];
	  guint y = j * ys + title_height;

	  if (c->valid)
	    {
	      char buf[10];
	      guint w;

	      gdk_draw_rectangle (drawable, c->popup.events ? cream_gc :
	                         (i < 5 ? yellow_gc : salmon_gc), 
				  TRUE,
				  x, y, xs, ys);

	      gdk_draw_rectangle (drawable, black_gc, FALSE,
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
				x + 2, y,
				pl);
	      g_free (buffer);

              if (c->popup.events)
                {
                  GSList *iter;

                  for (iter = c->popup.events; iter; iter = iter->next)
                    {
	              y += pr.height + 2;

	              event_t ev = iter->data;
	              event_details_t evd = event_db_get_details (ev);

	              pango_layout_set_width (pl_evt, xs * PANGO_SCALE);
	              pango_layout_set_text (pl_evt, evd->summary, -1);
	              gtk_paint_layout (widget->style,
				        widget->window,
				        GTK_WIDGET_STATE (widget),
				        FALSE,
				        &event->area,
				        widget,
				        "label",
				        x + 2, y,
				        pl_evt);
                    }
                }
	    }
	  else
	    {
	      gdk_draw_rectangle (drawable, gray_gc,
				  TRUE,
				  x + 1, y + 1, xs - 1, ys - 1);

              if (i == 0)
                gdk_draw_line (drawable, light_gray_gc, x, y + 1, x, y + ys);

              gdk_draw_line (drawable, light_gray_gc, x + 1, y + ys, x + xs, y + ys);
              gdk_draw_line (drawable, light_gray_gc, x + xs, y + 1, x + xs, y + ys); 
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
	  guint y = j * ys + title_height;

	  if ((!force_today && c->active) || (force_today && c->today))
	    {
	      gdk_draw_rectangle (drawable, blue_gc, FALSE,
				  x, y, xs, ys);
	      gdk_draw_rectangle (drawable, blue_gc, FALSE,
				  x + 1, y + 1, xs - 2, ys - 2);
	    }
	}
    }

  gdk_gc_set_clip_rectangle (black_gc, NULL);
  gdk_gc_set_clip_rectangle (gray_gc, NULL);
  gdk_gc_set_clip_rectangle (white_gc, NULL);

  gdk_gc_unref (blue_gc);
  gdk_gc_unref (cream_gc);
  gdk_gc_unref (light_gray_gc);
  gdk_gc_unref (yellow_gc);
  gdk_gc_unref (salmon_gc);

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
month_view_update (void)
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
      if (rday == tm_start.tm_mday) 
	active_day = day;
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
      
      if (!tm_start.tm_isdst) 
	{
	  start+=60*60;
	  end+=60*60;
	}

      if (day_events[day]) 
	event_db_list_destroy (day_events[day]);
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

      c->active = (day == active_day) ? TRUE : FALSE;
    }

  gtk_widget_queue_draw (draw);

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
  int width, height;
  
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  
  draw = gtk_drawing_area_new ();
  
  gtk_widget_show (draw);
  gtk_widget_show (scrolled_window);
  
  
  g_signal_connect (G_OBJECT (draw), "expose_event",
                    G_CALLBACK (draw_expose_event), NULL);
  
   gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), draw);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		                                      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    

  
  datesel = gtk_date_sel_new (GTKDATESEL_MONTH);
  gtk_widget_show (datesel);
  GTK_WIDGET_SET_FLAGS(datesel, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(datesel);

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
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
  
  gtk_window_get_size (main_window,&width,&height);
  gtk_widget_set_size_request (draw, width*.9, height*.65);

  calc_title_height (draw);

  return vbox;
}
