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
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/event-db.h>

#include "globals.h"
#include "week_view.h"
#include "day_popup.h"

#include "gtkdatesel.h"

#define _(x) gettext(x)

static GtkWidget *week_view_draw;
static struct tm today;
static guint min_cell_height = 38;
static GtkWidget *datesel;

static guint time_width, available_width;

static struct render_ctl *c_old;

struct render_ctl
{
  struct day_popup popup;
  gboolean valid;
  gboolean today;
};

struct week_day
{
  guint y0, y1;
  gchar *string;
  GSList *events;
  gboolean is_today;
  gboolean is_active;
  gboolean initialized;
  struct render_ctl rc;
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
  PangoLayout *pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
  PangoLayout *pl_evt = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
  gboolean draw_sep = FALSE;

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
      GSList *iter;
      for (iter = week_days[day].events; iter; iter = iter->next)
        ((event_t)iter->data)->mark = FALSE;
    }

  for (day = 0; day < 7; day++)
    {
      guint x, y = week_days[day].y0;
      GSList *iter;
      PangoRectangle pr;

      if (draw_sep)
        gdk_draw_line (drawable, black_gc, 0, y, max_width, y);

      gdk_draw_rectangle (drawable, white_gc, 
			  TRUE, time_width, y + 1, max_width - time_width, max_height);

      gdk_draw_rectangle (drawable, gray_gc, 
			  TRUE, 0, y + 1, time_width, max_height);

      pango_layout_set_width (pl, max_width * PANGO_SCALE);
      pango_layout_set_markup (pl, week_days[day].string, strlen (week_days[day].string));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      x = max_width - pr.width - 8;
      gtk_paint_layout (widget->style,
			widget->window,
			GTK_WIDGET_STATE (widget),
			FALSE,
			&event->area,
			widget,
			"label",
			x, y,
			pl);
      y += pr.height + 2;

      if (week_days[day].events)
      {
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
	      guint height = 0;
	      char buf[256];
	      struct tm tm;
	      event_t ev = iter->data;
	      event_details_t evd = event_db_get_details (ev);
	      
	      if ((ev->flags & FLAG_UNTIMED) == 0)
            {
              if (ev->mark == FALSE)
                {
                  gchar *buffer;
                  localtime_r (&ev->start, &tm);
                  strftime (buf, sizeof (buf), "%H:%M", &tm);
                  buffer = g_locale_to_utf8 (buf, -1, NULL,
                             NULL, NULL);
                  pango_layout_set_text (pl_evt, buffer, strlen (buffer));
                  gtk_paint_layout (widget->style,
                        widget->window,
                        GTK_WIDGET_STATE (widget),
                        FALSE,
                        &event->area,
                        widget,
                        "label",
                        2, y - 2,
                        pl_evt);
    
                  pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
                  if (height < pr.height)
                    height = pr.height;
                  ev->mark = TRUE;
                  g_free (buffer);
                }
              }

	      pango_layout_set_width (pl_evt, available_width * PANGO_SCALE);
	      pango_layout_set_text (pl_evt, evd->summary, -1);
	      pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
	      gtk_paint_layout (widget->style,
				widget->window,
				GTK_WIDGET_STATE (widget),
				FALSE,
				&event->area,
				widget,
				"label",
				2 + time_width, y - 2,
				pl_evt);

          if (height < pr.height)
            height = pr.height;

	      y += height + 2;
	    }
	}

      if (week_days[day].is_active)
        {
	      gdk_draw_rectangle (drawable, red_gc, FALSE, 0, week_days[day].y0, max_width, week_days[day].y1 - week_days[day].y0);
          gdk_draw_rectangle (drawable, red_gc, FALSE, 1, week_days[day].y0 + 1, max_width - 2, week_days[day].y1 - week_days[day].y0 - 2);
          draw_sep = FALSE;
        }
      else
       draw_sep = TRUE;
  }

  gdk_gc_unref (red_gc);

  g_object_unref (pl);
  g_object_unref (pl_evt);

  return TRUE;
}


void
week_free_lists()
{
  int day;
  for (day = 0; day < 7; day++)
    {
      struct week_day *d = &week_days[day];
      if (d->events)
        {
          event_db_list_destroy (d->events);
          d->events = NULL;
        }
    }
}
  
static void
week_view_update (void)
{
  guint day;
  time_t t = time (NULL);
  struct tm tm;
  guint y = 0;
  PangoLayout *pl = gtk_widget_create_pango_layout (GTK_WIDGET (week_view_draw), NULL);
  PangoLayout *pl_evt = gtk_widget_create_pango_layout (GTK_WIDGET (week_view_draw), NULL);
  GSList *iter;

  localtime_r (&t, &today);

  t = viewtime;
  localtime_r (&t, &tm);
  if (week_starts_monday)
    tm.tm_wday = (tm.tm_wday + 6) % 7;
  t -= SECONDS_IN_DAY * tm.tm_wday;
  t -= tm.tm_sec + (60 * tm.tm_min) + (60 * 60 * tm.tm_hour);

  time_width = 0;

  for (day = 0; day < 7; day++)
    {
      char buf[32];
      struct week_day *d = &week_days[day];
      struct render_ctl *c = &d->rc;

      if (d->events)
        event_db_list_destroy (d->events);

      d->events = event_db_untimed_list_for_period (t, t + SECONDS_IN_DAY - 1, TRUE);
      d->events = g_slist_concat (d->events, 
				  event_db_untimed_list_for_period (t, t + SECONDS_IN_DAY - 1, 
								    FALSE));

      localtime_r (&t, &tm);
      week_days[day].is_today = (tm.tm_mday == today.tm_mday 
		       && tm.tm_mon == today.tm_mon 
		       && tm.tm_year == today.tm_year);
      if (!week_days[day].initialized)
        {
          week_days[day].is_active = week_days[day].is_today;
          week_days[day].initialized = TRUE;
        }
      strftime (buf, sizeof (buf), "<b>%a %d %B</b>", &tm);
      if (d->string)
	     g_free (d->string);
      d->string = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);

      c->popup.day = tm.tm_mday;
      c->popup.year = tm.tm_year;
      c->popup.month = tm.tm_mon;
 	  c->valid = TRUE;
	  c->popup.events = week_days[day].events;
      c->today = ((tm.tm_year == today.tm_year + 1900
		   && tm.tm_mon == today.tm_mon
		   && tm.tm_mday == today.tm_mday)) ? TRUE : FALSE;
      
      t += SECONDS_IN_DAY;

      for (iter = week_days[day].events; iter; iter = iter->next)
        ((event_t)iter->data)->mark = FALSE;
      
    }

  for (day = 0; day < 7; day++)
    {
      struct week_day *d = &week_days[day];
      for (iter = d->events; iter; iter = iter->next)
        {
          /* calculate width required to display times */
          PangoRectangle pr;
          event_t ev = (event_t)iter->data;
          char buf[32];
          char *p = buf;
          size_t l = sizeof (buf) - 1;
          gchar *s;
          if ((ev->flags & FLAG_UNTIMED) == 0)
            {
              if (ev->mark == FALSE)
                {
                  size_t n;
                  localtime_r (&ev->start, &tm);
                  n = strftime (p, l, "%H:%M", &tm);
                  p += n;
                  l -= n;
                }
    
              ev->mark = TRUE;
              s = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
              pango_layout_set_text (pl_evt, s, strlen (s));
              pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
              if (time_width < pr.width)
                time_width = pr.width;
              g_free (s);
            }
        }
    }

  time_width += 2;

  available_width = week_view_draw->allocation.width - time_width - 2;

  for (day = 0; day < 7; day++)
    {
      guint height;
      PangoRectangle pr;

      pango_layout_set_width (pl, week_view_draw->allocation.width * PANGO_SCALE);
      pango_layout_set_markup (pl, week_days[day].string, strlen (week_days[day].string));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      height = pr.height + 2;

      if (week_days[day].events)
	{

	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
	      event_t ev = iter->data;
	      event_details_t evd = event_db_get_details (ev);
	      pango_layout_set_width (pl_evt, available_width * PANGO_SCALE);
	      pango_layout_set_text (pl_evt, evd->summary, -1);
	      pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
	      height += pr.height + 2;
	    }
	}

    if (height < min_cell_height)
      height = min_cell_height;

      week_days[day].y0 = y;
      y += height;
      week_days[day].y1 = y;
    }    
    
  gtk_widget_set_usize (week_view_draw, -1, y);

  gtk_widget_draw (week_view_draw, NULL);

  g_object_unref (pl);
  g_object_unref (pl_evt);
}

static void
changed_callback (GtkWidget *widget, GtkWidget *entry)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));

  week_view_update ();
}

static void
update_hook_callback (void)
{
  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
  gtk_widget_draw (datesel, NULL);

  week_view_update ();
}

static gboolean
week_view_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *data)
{
  int i;
  struct render_ctl *c;
  
  if (k->keyval == GDK_Escape)
    {
      for (i=0;i<7;i++)
      if (week_days[i].is_active)
        {
          c = &week_days[i].rc;
          if (c->valid)
            {
              if (pop_window) 
                gtk_widget_destroy (pop_window);
              if (c != c_old) 
                {
                  pop_window = day_popup (main_window, &c->popup, FALSE);
                  c_old = c;
                }
              else 
                {
                  pop_window = NULL;
                  c_old = NULL;
                }
            }    
        }
    }
    
  if (k->keyval == GDK_Down) 
  {
    for (i=0;i<7;i++)
      if (week_days[i].is_active)
        {
          if (i < 6) 
            {
              week_days[i].is_active = FALSE;
              week_days[i+1].is_active = TRUE;
            }
          else
            gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(widget)),
		                         GTK_DIR_DOWN);  
          week_view_update ();
          break;
        }
    return TRUE;
  }
  if (k->keyval == GDK_Up) 
  {
    for (i=0;i<7;i++)
      if (week_days[i].is_active)
        {
          if (i > 0) 
            {
              week_days[i].is_active = FALSE;
              week_days[i-1].is_active = TRUE;
            }
          else
            gtk_widget_child_focus(gtk_widget_get_toplevel(GTK_WIDGET(widget)),
		                         GTK_DIR_UP);  
          week_view_update ();
          break;
        }
    return TRUE;
  }

  if (k->keyval == GDK_space)
    {
      for (i=0;i<7;i++)
          if (week_days[i].is_active) break;
            
      c = &week_days[i].rc;
      if (c->valid)
        {
          if (pop_window) 
            gtk_widget_destroy (pop_window);
              if (c != c_old) 
            {
              pop_window = day_popup (main_window, &c->popup, FALSE);
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
      for (i=0;i<7;i++)
          if (week_days[i].is_active) break;
            
      c = &week_days[i].rc;
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


static int
get_day_from_y(int yc)
{
  int i;
  for (i=0;i<7;i++)
    {
      if ((week_days[i].y0 <= yc) && (week_days[i].y1 >= yc))
        return i;
    }
  return 0;
}


static gboolean
week_view_button_press (GtkWidget *widget,
	                    GdkEventButton *event,
	                    gpointer d)
{
  guint y = event->y;
  struct render_ctl *c;

  if (event->button != 1)
    return FALSE;

  c = &week_days[get_day_from_y(y)].rc;
  if (c->valid)
    {
      if (event->type == GDK_BUTTON_PRESS)
        {
          int i;
          for (i=0;i<7;i++)
             week_days[i].is_active = FALSE;
          week_days[get_day_from_y(y)].is_active = TRUE;
          week_view_update();
          
          if (pop_window) 
            gtk_widget_destroy (pop_window);
    
          if (c != c_old) 
            {
              pop_window = day_popup (main_window, &c->popup, FALSE);
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
          tm.tm_year = c->popup.year;// - 1900;
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


GtkWidget *
week_view (void)
{
  int i;
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *draw = gtk_drawing_area_new ();
  GtkWidget *scroller = gtk_scrolled_window_new (NULL, NULL);

  datesel = gtk_date_sel_new (GTKDATESEL_WEEK);

  gtk_widget_show (draw);
  gtk_widget_show (scroller);
  gtk_widget_show (datesel);

  g_signal_connect (G_OBJECT (draw), "expose_event",
                    G_CALLBACK (draw_expose_event), datesel);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroller), draw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (datesel), "changed",
                    G_CALLBACK (changed_callback), NULL);

  g_object_set_data (G_OBJECT (vbox), "update_hook",
                     (gpointer) update_hook_callback);

  week_view_draw = draw;
  
  for (i=0;i<7;i++)
    week_days[i].initialized = FALSE;
  gtk_widget_grab_focus(datesel);
  g_signal_connect (G_OBJECT (datesel), "key_press_event", 
		    G_CALLBACK (week_view_key_press_event), NULL);
  g_signal_connect(G_OBJECT (draw), "button-press-event",
                   G_CALLBACK (week_view_button_press), NULL);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_add_events (GTK_WIDGET (datesel), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  
  return vbox;
}
