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

#include "gtkdatesel.h"
#include "globals.h"
#include "event-db.h"
#include "week_view.h"
#include "event-ui.h"

#define _(x) gettext(x)

static struct tm today;
static GtkWidget *datesel;

static GtkWidget *list[7];
static GtkWidget *week_view_vbox;

struct week_day
{
  guint y;
  gchar *string;
  GSList *events;
  gboolean is_today;
} week_days[7];

static void 
selection_made(GtkWidget *w, 
	       gint            row,
	       gint            column,
	       GdkEventButton *event,
	       GtkWidget      *widget)
{
  if (event->type == GDK_2BUTTON_PRESS)
    {
      event_t ev = gtk_clist_get_row_data (GTK_CLIST (w), row);
      if (ev)
	{
	  GtkWidget *appt = edit_event (ev);
	  gtk_widget_show (appt);
	}
    }
}

static int
draw_header(GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  guint width, height;
  GdkGC *black_gc;
  GdkGC *white_gc;
  guint w;
  GdkFont *font = widget->style->font;
  struct week_day *wp = (struct week_day *)user_data;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  white_gc = widget->style->white_gc;
  black_gc = widget->style->black_gc;
  
  gdk_gc_set_clip_rectangle (black_gc, &event->area);
  gdk_gc_set_clip_rectangle (white_gc, &event->area);
  gdk_gc_set_clip_rectangle (widget->style->base_gc[GTK_STATE_ACTIVE],
			     &event->area);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_draw_rectangle (drawable,
		      widget->style->base_gc[GTK_STATE_ACTIVE],
		      TRUE, 0, 0, width, height);

  gdk_draw_line (drawable, black_gc,
		 0, 0, width, 0);
  w = gdk_string_width (font, wp->string);
  gdk_draw_text (drawable, font, black_gc,
		 width - w, font->ascent, wp->string, strlen (wp->string));

  gdk_gc_set_clip_rectangle (black_gc, NULL);
  gdk_gc_set_clip_rectangle (white_gc, NULL);
  gdk_gc_set_clip_rectangle (widget->style->base_gc[GTK_STATE_ACTIVE],
			     NULL);

  return TRUE;
}

static void
week_view_update (void)
{
  guint day;
  time_t t = time (NULL);
  struct tm tm;

  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
  gtk_widget_draw (datesel, NULL);

  localtime_r (&t, &today);

  t = viewtime;
  localtime_r (&t, &tm);
  if (week_starts_monday)
    tm.tm_wday = (tm.tm_wday + 6) % 7;
  t -= SECONDS_IN_DAY * tm.tm_wday;
  t -= tm.tm_sec + (60 * tm.tm_min) + (60 * 60 * tm.tm_hour);

  for (day = 0; day < 7; day++)
    {
      char buf[32];
      GSList *iter;      

      if (week_days[day].events)
	event_db_list_destroy (week_days[day].events);

      week_days[day].events = event_db_list_for_period (t, t + SECONDS_IN_DAY - 1);

      localtime_r (&t, &tm);
      week_days[day].is_today = (tm.tm_mday == today.tm_mday 
		       && tm.tm_mon == today.tm_mon 
		       && tm.tm_year == today.tm_year);
      strftime(buf, sizeof (buf), "%a %d %B", &tm);
      if (week_days[day].string)
	g_free (week_days[day].string);
      week_days[day].string = g_strdup (buf);

      if (week_days[day].events)
	{
	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
	      event_t ev = iter->data;
	      ev->mark = FALSE;
	    }
	}

      t += SECONDS_IN_DAY;
    }

  for (day = 0; day < 7; day++)
    {
      gtk_clist_clear (GTK_CLIST (list[day]));

      if (week_days[day].events)
	{
	  GSList *iter;
	  guint row = 0;

	  for (iter = week_days[day].events; iter; iter = iter->next)
	    {
	      event_t ev = iter->data;
	      event_details_t evd = event_db_get_details (ev);
	      gchar *line_info[2];
	      line_info[0] = NULL;
	      line_info[1] = evd->summary;
	      gtk_clist_append (GTK_CLIST (list[day]), line_info);
	      gtk_clist_set_row_data (GTK_CLIST (list[day]), row, ev);
	      row++;
	    }
	}
    }

  gtk_widget_draw (week_view_vbox, NULL);
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
  GtkWidget *scroller = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);
  guint i;

  datesel = gtk_date_sel_new (GTKDATESEL_WEEK);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroller), 
					 vbox2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  for (i = 0; i < 7; i++)
    {
      GtkWidget *draw = gtk_drawing_area_new ();

      gtk_widget_set_usize (draw, -1, draw->style->font->ascent 
			    + draw->style->font->descent);
      gtk_widget_show (draw);

      list[i] = gtk_clist_new (2);
      gtk_clist_set_shadow_type (GTK_CLIST (list[i]), GTK_SHADOW_NONE);
      gtk_widget_show (list[i]);

      gtk_signal_connect (GTK_OBJECT (list[i]), "select_row",
			 GTK_SIGNAL_FUNC (selection_made),
			 NULL);

      gtk_box_pack_start (GTK_BOX (vbox2), draw, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox2), list[i], TRUE, TRUE, 0);

      gtk_signal_connect (GTK_OBJECT (draw), "expose_event",
			  GTK_SIGNAL_FUNC (draw_header), &week_days[i]);
    }

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), NULL);

  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook",
                       (gpointer) week_view_update);

  week_view_vbox = vbox;

  return vbox;
}
