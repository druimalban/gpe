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

#include <gpe/gtkdatesel.h>

#include "globals.h"
#include "event-db.h"
#include "week_view.h"

#define _(x) gettext(x)

static GtkWidget *week_view_draw;
static struct tm today;
static guint min_cell_height = 38;
static GtkWidget *datesel;

static guint time_width, available_width;

struct week_day
{
  guint y0, y1;
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
#if GTK_MAJOR_VERSION >= 2
  PangoLayout *pl = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
  PangoLayout *pl_evt = gtk_widget_create_pango_layout (GTK_WIDGET (widget), NULL);
#endif
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
#if GTK_MAJOR_VERSION >= 2
      PangoRectangle pr;
#endif

      if (draw_sep)
	gdk_draw_line (drawable, black_gc, 0, y, max_width, y);

      gdk_draw_rectangle (drawable, white_gc, 
			  TRUE, time_width, y + 1, max_width - time_width, max_height);

      gdk_draw_rectangle (drawable, gray_gc, 
			  TRUE, 0, y + 1, time_width, max_height);

#if GTK_MAJOR_VERSION < 2
      x = max_width - gdk_string_width (datefont, week_days[day].string) - 8;
      gdk_draw_text (drawable, datefont,
		     week_days[day].is_today ? red_gc : black_gc,
		     x, y + datefont->ascent + 1, week_days[day].string, 
		     strlen (week_days[day].string));

      y += datefont->ascent + datefont->descent;
#else
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
#endif

      if (week_days[day].events)
	{
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
#if GTK_MAJOR_VERSION < 2
	      GdkFont *font = widget->style->font;
#endif
	      guint height = 0;
	      char buf[256];
	      struct tm tm;
	      event_t ev = iter->data;
	      event_details_t evd = event_db_get_details (ev);

#if GTK_MAJOR_VERSION < 2
	      gdk_draw_text (drawable, font, black_gc,
			     4, y + font->ascent,
			     buf, strlen (buf));
	      y += font->ascent + font->descent;
#else
	      if ((ev->flags & FLAG_UNTIMED) == 0)
		{
		  if (ev->mark == FALSE)
		    {
		      localtime_r (&ev->start, &tm);
		      strftime (buf, sizeof (buf), "%H:%M", &tm);
		  
		      pango_layout_set_text (pl_evt, buf, strlen (buf));
		      gtk_paint_layout (widget->style,
					widget->window,
					GTK_WIDGET_STATE (widget),
					FALSE,
					&event->area,
					widget,
					"label",
					0, y,
					pl_evt);

		      pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
		      if (height < pr.height)
			height = pr.height;
		      ev->mark = TRUE;
		    }
		}

	      pango_layout_set_width (pl_evt, available_width * PANGO_SCALE);
	      pango_layout_set_text (pl_evt, evd->summary, strlen (evd->summary));
	      pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
	      gtk_paint_layout (widget->style,
				widget->window,
				GTK_WIDGET_STATE (widget),
				FALSE,
				&event->area,
				widget,
				"label",
				2 + time_width, y,
				pl_evt);

	      if (height < pr.height)
		height = pr.height;

	      y += height + 2;
#endif
	    }
	}

      if (week_days[day].is_today)
	{
	  gdk_draw_rectangle (drawable, red_gc, FALSE, 0, week_days[day].y0, max_width, week_days[day].y1 - week_days[day].y0);
	  gdk_draw_rectangle (drawable, red_gc, FALSE, 1, week_days[day].y0 + 1, max_width - 2, week_days[day].y1 - week_days[day].y0 - 2);
	  draw_sep = FALSE;
	}
      else
	draw_sep = TRUE;
    }

  gdk_gc_unref (red_gc);

#if GTK_MAJOR_VERSION >= 2
  g_object_unref (pl);
  g_object_unref (pl_evt);
#endif

  return TRUE;
}

static void
week_view_update (void)
{
  guint day;
  time_t t = time (NULL);
  struct tm tm;
  guint y = 0;
#if GTK_MAJOR_VERSION >= 2
  PangoLayout *pl = gtk_widget_create_pango_layout (GTK_WIDGET (week_view_draw), NULL);
  PangoLayout *pl_evt = gtk_widget_create_pango_layout (GTK_WIDGET (week_view_draw), NULL);
#endif
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
      strftime (buf, sizeof (buf), "<b>%a %d %B</b>", &tm);
      if (d->string)
	g_free (d->string);
      d->string = g_strdup (buf);

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
	      pango_layout_set_text (pl_evt, buf, strlen (buf));
	      pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
	      if (time_width < pr.width)
		time_width = pr.width;
	    }
	}
    }

  time_width += 2;

  available_width = week_view_draw->allocation.width - time_width - 2;

  for (day = 0; day < 7; day++)
    {
      guint height;

#if GTK_MAJOR_VERSION < 2
      height = datefont->ascent + datefont->descent;
#else
      PangoRectangle pr;
      pango_layout_set_width (pl, week_view_draw->allocation.width * PANGO_SCALE);
      pango_layout_set_markup (pl, week_days[day].string, strlen (week_days[day].string));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      height = pr.height + 2;
#endif

      if (week_days[day].events)
	{
	  event_t ev;
	  event_details_t evd;
  
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
#if GTK_MAJOR_VERSION < 2
	      height += week_view_draw->style->font->ascent + 
		week_view_draw->style->font->descent;
#else
	      ev = iter->data;
	      evd = event_db_get_details (ev);
	      pango_layout_set_width (pl_evt, available_width * PANGO_SCALE);
	      pango_layout_set_text (pl_evt, evd->summary, strlen (evd->summary));
	      pango_layout_get_pixel_extents (pl_evt, &pr, NULL);
	      height += pr.height + 2;
#endif
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

#if GTK_MAJOR_VERSION >= 2
  g_object_unref (pl);
  g_object_unref (pl_evt);
#endif
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *entry)
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

GtkWidget *
week_view (void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *draw = gtk_drawing_area_new ();
  GtkWidget *scroller = gtk_scrolled_window_new (NULL, NULL);

  datesel = gtk_date_sel_new (GTKDATESEL_WEEK);

  gtk_widget_show (draw);
  gtk_widget_show (scroller);
  gtk_widget_show (datesel);

  gtk_signal_connect (GTK_OBJECT (draw),
		      "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      datesel);
  
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroller), draw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), NULL);

  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook",
                       (gpointer) update_hook_callback);

  week_view_draw = draw;

  return vbox;
}
