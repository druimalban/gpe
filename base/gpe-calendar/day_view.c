/*
 * Copyright (C) 2001, 2002, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
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
#include <locale.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/pixmaps.h>
#include <gpe/event-db.h>
#include <gpe/question.h>
#include <gpe/schedule.h>
#include <gpe/stylus.h>
#include <gpe/spacing.h>
#include "event-ui.h"
#include "globals.h"
#include "day_view.h"
#include "day_render.h"
#include "gtkdatesel.h"
#include "export-vcal.h"

#define _(x) gettext(x)
#define NUM_HOURS 24

static GSList *day_events[NUM_HOURS], *untimed_events;

static GtkWidget *rem_area;
static GtkWidget *datesel, *calendar, *scrolled_window;

/* Day renderer */
static event_t sel_event;
static day_page_t page_app, page_rem;
/* Day events are visualised as rectangles */

static GtkWidget *draw, *popup, *event_infos_popup, *send_bt_button, *send_ir_button;

static struct day_render *dr, *rem_render;

static void day_page_calc_time_width (day_page_t page);

/* Day page constructor */
static day_page_t
day_page_new (GtkWidget * widget)
{
  gint width, height, tmp;
  day_page_t this;

  gtk_widget_get_size_request (widget, &width, &height);
  tmp = height / NUM_HOURS;
  height = tmp * NUM_HOURS;

  this = (struct day_page *) g_malloc (sizeof (struct day_page));
  this->width = width;
  this->height = height;
  this->height_min = height;
  this->widget = widget;
  
  day_page_calc_time_width (this);

  return this;
}

static void
day_page_draw_empty (const day_page_t page)
{
  GdkGC *white_gc, *black_gc;

  white_gc = page->widget->style->white_gc;
  black_gc = page->widget->style->black_gc;

  gdk_draw_rectangle (page->widget->window, white_gc,
		      TRUE, 0, 0, page->width, page->height);
  gdk_draw_rectangle (page->widget->window, black_gc,
		      FALSE, 0, 0, page->width - 1, page->height - 1);

}

static void
day_page_calc_time_width (day_page_t page)
{
  PangoLayout *pl;
  PangoRectangle pr;
  guint width = 0;
  int i;

  pl = gtk_widget_create_pango_layout (page->widget, NULL);

  for (i = 0; i < NUM_HOURS; i++)
    {
      char buf[40], *buffer;

      snprintf (buf, sizeof (buf), "<span font_desc='normal'>%.2d:00</span>", i);
      buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
      pango_layout_set_markup (pl, buffer, strlen (buffer));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      width = MAX (width, pr.width);

      g_free (buffer);
    }

#ifdef IS_HILDON
  width = width * 1.6;
#endif
    
  page->time_width = width + 3;
    
  g_object_unref (pl);
}

/* Day page background */
static void
day_page_draw_background (const day_page_t page)
{
  GdkGC *gray_gc, *white_gc, *black_gc;
  GtkWidget *widget;
  guint i;
  PangoLayout *pl;
  GdkRectangle gr;
  PangoRectangle pr;
  widget = page->widget;
  pl = gtk_widget_create_pango_layout (widget, NULL);
  white_gc = widget->style->white_gc;
  gray_gc = pen_new (widget, 58905, 58905, 56610);
  black_gc = widget->style->black_gc;

  /* Row height */
  /* Draw the hour column */
  gdk_draw_rectangle (widget->window, white_gc, TRUE, 0, 0, page->width,
		      page->height);
  gdk_draw_rectangle (widget->window, gray_gc, TRUE, 0, 0,
		      page->time_width, page->height);

  for (i = 0; i < NUM_HOURS; i++)
    {
      char buf[40], *buffer;
      gdk_draw_line (widget->window, gray_gc, 0, page->height / NUM_HOURS * i,
		     page->width, page->height / NUM_HOURS * i);
      gdk_draw_line (widget->window, white_gc, 0,
		     page->height / NUM_HOURS * i,
		     page->time_width,
		     page->height / NUM_HOURS * i);

      snprintf (buf, sizeof (buf), "<span font_desc='normal'>%.2d:00</span>", i);
      buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
      pango_layout_set_markup (pl, buffer, strlen (buffer));
      pango_layout_get_pixel_extents (pl, &pr, NULL);
      gr.width = pr.width * 2;
      gr.height = pr.height * 2;
      gr.x = 1;
      gr.y = page->height / NUM_HOURS * i;

      gtk_paint_layout (widget->style,
			widget->window,
			GTK_WIDGET_STATE (widget),
			FALSE, &gr, widget, "label", gr.x, gr.y, pl);

      g_free (buffer);
    }

  /* Draws the hours rectangle */
  gdk_draw_line (widget->window, black_gc, page->time_width, 0,
		 page->time_width, page->height - 1);

  g_object_unref (pl);
  g_object_unref (gray_gc);
}

gboolean
day_view_button_press (GtkWidget * widget, GdkEventButton * event, gpointer d)
{
  GSList *iter;

  guint x, y;
  GtkWidget *w;
  gboolean found = FALSE;
  struct day_render *dr_ = (struct day_render *) d;

  x = event->x;
  y = event->y;
  gtk_widget_hide (popup);

  /* search for an event rectangle */
  if (dr_->event_rectangles != NULL)
    {
      for (iter = dr_->event_rectangles; iter; iter = iter->next)
	{
	  ev_rec_t e_r;
	  e_r = iter->data;
	  if ((x >= e_r->x && x <= e_r->x + e_r->width)
	      && (y >= e_r->y && y <= e_r->y + e_r->height))
	    {
	      /* found... so update and show popup */
	      gint width, height;
	      found = TRUE;
	      sel_event = e_r->event;
	      gchar *buffer = NULL;
	      gchar *tbuffer = NULL;
	      event_details_t evd;
	      struct tm start_tm, end_tm;
	      time_t end;

	      end = (time_t) (e_r->event->duration + e_r->event->start);

	      localtime_r (&(e_r->event->start), &start_tm);
	      localtime_r (&end, &end_tm);
	      evd = event_db_get_details (e_r->event);
	      buffer = (char *) g_malloc (sizeof (char) * 256);

	      snprintf (buffer, 256, "%s %.2d:%.2d-%.2d:%.2d %s\n\n%s",
			evd->summary, start_tm.tm_hour, start_tm.tm_min,
			end_tm.tm_hour, end_tm.tm_min,
			(e_r->event->flags & FLAG_ALARM ? "(A)" : ""),
			evd->description ? evd->description : "");
	      tbuffer = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
	      gtk_window_set_position (GTK_WINDOW (popup), GTK_WIN_POS_MOUSE);
	      gtk_window_set_default_size (GTK_WINDOW (popup),
					   dr_->page->width / 2,
					   dr_->page->height * .8);
	      gtk_window_get_size (GTK_WINDOW (popup), &width, &height);
	      if (buffer)
		gtk_label_set_text (GTK_LABEL (event_infos_popup),
				    tbuffer ? tbuffer : buffer);
	      else
		gtk_label_set_text (GTK_LABEL (event_infos_popup), "");

	      /* check for available export methods */
	      if (export_bluetooth_available ())
		gtk_widget_show (send_bt_button);
	      else
		gtk_widget_hide (send_bt_button);

	      if (export_irda_available ())
		gtk_widget_show (send_ir_button);
	      else
		gtk_widget_hide (send_ir_button);

	      gtk_widget_show (popup);

	      if (tbuffer)
		g_free (tbuffer);
	      if (buffer)
		g_free (buffer);
	      break;
	    }
	}
    }

  if (!found)
    {
      struct tm tm;
      int hour;
      localtime_r (&viewtime, &tm);
      hour = (int) (((float) y / (float) dr_->page->height) * NUM_HOURS);
      tm.tm_hour = hour;
      tm.tm_min = 0;

      w = new_event (mktime (&tm), 1);
      gtk_widget_show (w);
    }
  return TRUE;
}

static void
delete_event (event_t ev, GtkWidget * d)
{
  event_t ev_real;
  event_details_t ev_d;
  recur_t r;

  ev_real = get_cloned_ev (ev);
  ev_d = event_db_get_details (ev_real);

  if (ev_real->recur)
    {
      if (gpe_question_ask
	  (_
	   ("Delete all recurring entries?\n(If no, delete this instance only)"),
	   _("Question"), "question", "!gtk-no", NULL, "!gtk-yes", NULL,
	   NULL))
	{
	  if (ev_real->flags & FLAG_ALARM)
	    {
	      unschedule_alarm (ev_real, gtk_widget_get_toplevel (d));
	      schedule_next (0, 0, d);
	    }
	  event_db_remove (ev_real);
	}
      else
	{
	  r = event_db_get_recurrence (ev_real);
	  r->exceptions = g_slist_append (r->exceptions, (void *) ev->start);
	  event_db_flush (ev_real);
	}
    }
  else
    {
      if (ev_real->flags & FLAG_ALARM)
	{
	  unschedule_alarm (ev_real, gtk_widget_get_toplevel (d));
	  schedule_next (0, 0, d);
	}
      event_db_remove (ev_real);
    }

  update_all_views ();
  event_db_forget_details (ev_real);
}

static void
day_view_expose (void)
{
  GdkGC *white_gc, *gray_gc, *black_gc;
  gint width = 0, height = 0;
  white_gc = draw->style->white_gc;
  gray_gc = pen_new (draw, 58905, 58905, 56610);
  black_gc = draw->style->black_gc;

  gdk_window_get_size (draw->window, &width, &height);

  if (width != page_app->width || height != page_app->height)
    {
      if (dr)
       {
         day_render_update_offset (dr);
         day_render_resize (dr, width, height);
       }
    }
  day_page_draw_background (page_app);

  if (dr == NULL)
    day_view_init ();

  day_render_set_event_rectangles (dr);
  draw_appointments (dr);

  g_object_unref (gray_gc);
}

static gboolean
day_view_expose_cb (GtkWidget * widget,
		    GdkEventExpose * event, gpointer user_data)
{
  day_view_expose ();
  return TRUE;
}

static int
reminder_view_init ()
{
  GdkGC *white_gc, *black_gc, *cream_gc;

  time_t end, start;
  struct tm tm_start, tm_end;
  GSList *events, *iter, *rem_list = NULL;


  localtime_r (&viewtime, &tm_start);

  tm_start.tm_hour = 0;
  tm_start.tm_min = 0;
  tm_start.tm_sec = 0;
  start = mktime (&tm_start);
  localtime_r (&viewtime, &tm_end);
  tm_end.tm_hour = 23;
  tm_end.tm_min = 59;
  tm_end.tm_sec = 0;
  end = mktime (&tm_end);

  events = event_db_list_for_period (start, end);

  for (iter = events; iter; iter = iter->next)
    {
      event_t ev = iter->data;
      if (ev->duration == 0)
        {
          /* Reminder */
          rem_list = g_slist_append (rem_list, ev);
        }
    }

  white_gc = rem_area->style->white_gc;
  black_gc = rem_area->style->black_gc;

  if (rem_render)
    day_render_delete (rem_render);

  cream_gc = pen_new (rem_area, 65535, 64005, 51100);
  rem_render = day_render_new (rem_area, page_rem, cream_gc, black_gc,
			       viewtime, 5, 0, 1, rem_list);

  return TRUE;
}

static void
reminder_view_expose ()
{
  gint len;
  gint width = 0, height = 0;

  if (rem_render == NULL)
    reminder_view_init ();
  
  gdk_window_get_size (rem_area->window, &width, &height);
    
  if (width != page_rem->width || height != page_rem->height)
    day_render_resize (rem_render, width, height);

  day_page_draw_empty (page_rem);
  rem_render->offset.x = 0;
  day_render_set_event_rectangles (rem_render);

  len = g_slist_length (rem_render->events);
  if (len)
    {
      gtk_widget_set_size_request (rem_area, -1, dr->page->height / 20);
      draw_appointments (rem_render);
    }
  else
    {
      gtk_widget_set_size_request (rem_area, -1, 0);
    }
}

static gboolean
reminder_view_expose_cb (GtkWidget * widget,
			 GdkEventExpose * ev, gpointer user_data)
{
  reminder_view_expose ();
  return TRUE;
}

int
day_view_init (void)
{
  GdkGC *white_gc, *post_him_yellow, *black_gc;

  time_t end, start;
  struct tm tm_start, tm_end;
  GSList *events, *iter, *ev_list = NULL;


  localtime_r (&viewtime, &tm_start);

  tm_start.tm_hour = 0;
  tm_start.tm_min = 0;
  tm_start.tm_sec = 0;
  start = mktime (&tm_start);
  localtime_r (&viewtime, &tm_end);
  tm_end.tm_hour = 23;
  tm_end.tm_min = 59;
  tm_end.tm_sec = 0;
  end = mktime (&tm_end);

  events = event_db_list_for_period (start, end);

  for (iter = events; iter; iter = iter->next)
    {
      event_t ev = iter->data;
      if (ev->duration != 0)
        ev_list = g_slist_append (ev_list, ev);
    }

  white_gc = draw->style->white_gc;
  black_gc = draw->style->black_gc;

  if (dr)
    {
      g_slist_free (dr->events);
      dr->events = ev_list;
      dr->date = start;
    }
  else
    {
      /* Don't want to infinge some patents here */
      post_him_yellow = pen_new (draw, 63222, 59110, 33667);
      dr = day_render_new (draw, page_app, post_him_yellow, black_gc, viewtime,
                           5, 4, NUM_HOURS, ev_list);
    }
  return TRUE;
}

void
day_view_update (void)
{
  day_view_init ();
  day_view_expose ();
}

void
reminder_view_update ()
{
  reminder_view_init ();
  reminder_view_expose ();
}

static void
day_changed_calendar (GtkWidget * widget)
{
  guint day, month, year;
  struct tm tm;

  localtime_r (&viewtime, &tm);

  gtk_calendar_get_date (GTK_CALENDAR (widget), &year, &month, &day);


  if (tm.tm_year != (year - 1900) || tm.tm_mon != month || tm.tm_mday != day)
    {
      tm.tm_year = year - 1900;
      tm.tm_mon = month;
      tm.tm_mday = day;

      viewtime = mktime (&tm);

      set_time_all_views ();
    }
}

/* 
 * Go to hour h
 * If h is negative it goes to the current hour
 */
static void
scroll_to (GtkWidget * scrolled, gint hour)
{
  GtkAdjustment *adj;
  gint h = 0;
  gdouble upper, lower, value;

  /* Scroll to the current hour */
  if (hour < 0)
    {
      time_t now;
      struct tm now_tm;
      time (&now);
      localtime_r (&now, &now_tm);
      hour = MAX (now_tm.tm_hour - 1, 0);
    }

  gtk_widget_get_size_request (draw, NULL, &h);
  value = (gdouble) hour / NUM_HOURS * h;
  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled));

  lower = 0.0;
  upper = h;

  if (value > (adj->upper - adj->page_size))
    value = adj->upper - adj->page_size;

  gtk_adjustment_clamp_page (adj, lower, upper);
  gtk_adjustment_set_value (adj, value);
}

static gboolean
update_hook_callback ()
{
  struct tm tm;
  localtime_r (&viewtime, &tm);
  
  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
  gtk_calendar_select_month (GTK_CALENDAR (calendar), tm.tm_mon,
			     tm.tm_year + 1900);
  gtk_calendar_select_day (GTK_CALENDAR (calendar), tm.tm_mday);
  
  return TRUE;
}

void
day_free_lists ()
{
  int hour;
  if (untimed_events)
    event_db_list_destroy (untimed_events);
  untimed_events = NULL;
  for (hour = 0; hour <= 23; hour++)
    if (day_events[hour])
      {
	event_db_list_destroy (day_events[hour]);
	day_events[hour] = NULL;
      }
}

static gboolean
destroy_popup (GtkWidget * widget, GdkEventButton * event, gpointer d)
{
  gtk_widget_hide (popup);
  sel_event = NULL;
  return TRUE;
}

static void
delete_event_cb (GtkWidget * widget, GtkWidget * a)
{
  gtk_widget_hide (popup);
  delete_event (sel_event, widget);
}


static void
edit_event_cb (GtkWidget * widget, GtkWidget * a)
{
  GtkWidget *w;
  w = edit_event (sel_event);
  gtk_widget_show (w);
  gtk_widget_hide (popup);
}

static void
save_cb (GtkWidget * widget, GtkWidget * a)
{
  gtk_widget_hide (popup);
  vcal_do_save (sel_event);
}

static void
send_ir_cb (GtkWidget * widget, GtkWidget * a)
{
  gtk_widget_hide (popup);
  vcal_do_send_irda (sel_event);
}

static void
send_bt_cb (GtkWidget * widget, GtkWidget * a)
{
  gtk_widget_hide (popup);
  vcal_do_send_bluetooth (sel_event);
}

gboolean
changed_callback (GtkWidget * widget, GtkWidget * some)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));

  scroll_to (scrolled_window, -1);
  day_view_update ();
  reminder_view_update ();
  return TRUE;
}

GtkWidget *
day_view (void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *edit_event_button, *delete_event_button, *save_button;
  GtkWidget *vbox_popup = gtk_vbox_new (FALSE, 0);
  gboolean landscape;
  gint win_width, win_height;
  GtkWidget *frame;

  /* Popup needed to show infos about an appointment */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  popup = gtk_window_new (GTK_WINDOW_POPUP);
  frame = gtk_frame_new (NULL);

  event_infos_popup = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (event_infos_popup), 0, 0);
  gtk_misc_set_padding (GTK_MISC (event_infos_popup), gpe_get_border (),
			gpe_get_border ());

#ifdef IS_HILDON
  edit_event_button = gtk_button_new_with_label (_("Edit"));
  delete_event_button = gtk_button_new_with_label (_("Delete"));
  save_button = gtk_button_new_with_label (_("Save"));
#else
  edit_event_button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
  delete_event_button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  save_button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
#endif
  send_ir_button = gtk_button_new_with_label (_("Send via infared"));
  send_bt_button = gtk_button_new_with_label (_("Send via bluetooth"));

  gtk_button_set_relief (GTK_BUTTON (edit_event_button), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (delete_event_button), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (send_ir_button), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (send_bt_button), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (save_button), GTK_RELIEF_NONE);
  gtk_button_set_alignment (GTK_BUTTON (edit_event_button), 0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (delete_event_button), 0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (send_ir_button), 0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (send_bt_button), 0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (save_button), 0, 0.5);

  gtk_container_add (GTK_CONTAINER (popup), frame);
  gtk_container_add (GTK_CONTAINER (frame), vbox_popup);
  gtk_box_pack_start (GTK_BOX (vbox_popup), event_infos_popup, FALSE, TRUE,
		      0);

  gtk_box_pack_start (GTK_BOX (vbox_popup), edit_event_button, FALSE, TRUE,
		      0);
  gtk_box_pack_start (GTK_BOX (vbox_popup), delete_event_button, FALSE, TRUE,
		      0);
  gtk_box_pack_start (GTK_BOX (vbox_popup), save_button, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_popup), send_ir_button, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_popup), send_bt_button, FALSE, TRUE, 0);

  gtk_widget_show_all (frame);

  gtk_widget_add_events (GTK_WIDGET (popup),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (popup), "button-press-event",
		    G_CALLBACK (destroy_popup), NULL);

  g_signal_connect (G_OBJECT (edit_event_button), "clicked",
		    G_CALLBACK (edit_event_cb), NULL);

  g_signal_connect (G_OBJECT (delete_event_button), "clicked",
		    G_CALLBACK (delete_event_cb), NULL);

  g_signal_connect (G_OBJECT (save_button), "clicked",
		    G_CALLBACK (save_cb), NULL);

  g_signal_connect (G_OBJECT (send_ir_button), "clicked",
		    G_CALLBACK (send_ir_cb), NULL);

  g_signal_connect (G_OBJECT (send_ir_button), "clicked",
		    G_CALLBACK (send_bt_cb), NULL);

  gtk_window_set_decorated (GTK_WINDOW (popup), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);
  gtk_window_set_modal (GTK_WINDOW (popup), TRUE);
  /* end popup */

  landscape = gdk_screen_width () > gdk_screen_height ()? TRUE : FALSE;

  calendar = gtk_calendar_new ();
  GTK_WIDGET_UNSET_FLAGS (calendar, GTK_CAN_FOCUS);

  draw = gtk_drawing_area_new ();
  if (!GTK_WIDGET_REALIZED (gtk_widget_get_toplevel (main_window)))
    gtk_widget_realize (gtk_widget_get_toplevel (main_window));
  gdk_drawable_get_size (GTK_WIDGET (gtk_widget_get_toplevel (main_window))->
			 window, &win_width, &win_height);

  gtk_widget_set_size_request (draw, -1, win_height * 2);

  rem_area = gtk_drawing_area_new ();

  gtk_widget_set_size_request (rem_area, -1, win_height / 20);

  gtk_widget_show (draw);
  gtk_widget_show (rem_area);
  gtk_widget_show (scrolled_window);
  gtk_widget_show (hbox);
  page_app = day_page_new (draw);
  page_rem = day_page_new (rem_area);

  gtk_calendar_set_display_options (GTK_CALENDAR (calendar),
				    GTK_CALENDAR_SHOW_DAY_NAMES
				    | (week_starts_monday ?
				       GTK_CALENDAR_WEEK_START_MONDAY : 0));

  datesel = gtk_date_sel_new (GTKDATESEL_FULL);

  gtk_widget_show (datesel);

  g_signal_connect (G_OBJECT (calendar),
		    "day-selected", G_CALLBACK (day_changed_calendar), NULL);


  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW
					 (scrolled_window), draw);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (draw->parent), GTK_SHADOW_NONE);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), rem_area, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, TRUE, TRUE, 0);
#ifndef IS_HILDON
  if (landscape)
    {
      GtkWidget *sep;
      sep = gtk_vseparator_new ();

      gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), calendar, FALSE, FALSE, 0);

      gtk_widget_show (sep);
      gtk_widget_show (calendar);
    }
#endif

  g_object_set_data (G_OBJECT (vbox), "update_hook",
		     (gpointer) update_hook_callback);

  g_signal_connect (G_OBJECT (draw), "expose_event",
		    G_CALLBACK (day_view_expose_cb), NULL);

  g_signal_connect (G_OBJECT (rem_area), "expose_event",
		    G_CALLBACK (reminder_view_expose_cb), NULL);

  g_signal_connect (G_OBJECT (datesel), "changed",
		    G_CALLBACK (changed_callback), NULL);
    
  gtk_widget_add_events (GTK_WIDGET (datesel),
			 GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  g_object_set_data (G_OBJECT (main_window), "datesel-day", datesel);

  return vbox;
}
