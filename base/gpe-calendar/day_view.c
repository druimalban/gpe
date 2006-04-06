/*
 * Copyright (C) 2001, 2002, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
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
#include "event-cal.h"

#define _(x) gettext(x)
#define NUM_HOURS 24

struct _GtkDayView
{
  GtkVBox widget;

  /*
      Basic layout of a DayView:

       /-DayView------------------------------------\
       | /-event_box--------------\ /-calendar----\ |
       | | /-reminders----------\ | |             | |
       | | |                    | | |             | |
       | | \--------------------/ | |             | |
       | | /-appointment_window-\ | |             | |
       | | |/-appointments-----\| | |             | |
       | | ||                  || | |             | |
       | | |\------------------/| | |             | |
       | | \--------------------/ | |             | |
       | \------------------------/ \-------------/ |
       \--------------------------------------------/
   */

  GtkWidget *datesel;
  GtkWidget *calendar;

  gboolean scrolling;
  gboolean scroll_floating;
  GtkWidget *event_box;

  GtkDayRender *reminders;
  GtkWidget *appointment_window;
  GtkDayRender *appointments;

  /* Currently selected event (if any).  */
  event_t sel_event;

  /* A popup menu for events.  */
  GtkWidget *event_menu;
  /* A label describing the clicked event.  */
  GtkWidget *event_menu_info;

  time_t date;
};

typedef struct
{
  GtkVBoxClass vbox_class;
  GObjectClass parent_class;
} DayViewClass;

static void gtk_day_view_base_class_init (gpointer klass);
static void gtk_day_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_day_view_dispose (GObject *obj);
static void gtk_day_view_finalize (GObject *object);
static void gtk_day_view_show (GtkWidget *object);
static void day_view_update (GtkDayView *day_view, gboolean force);

static GtkWidgetClass *parent_class;

GType
gtk_day_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (DayViewClass),
	gtk_day_view_base_class_init,
	NULL,
	NULL,
	NULL,
	NULL,
	sizeof (struct _GtkDayView),
	0,
	gtk_day_view_init
      };

      type = g_type_register_static (gtk_vbox_get_type (),
				     "GtkDayView", &info, 0);
    }

  return type;
}

static void
gtk_day_view_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_vbox_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_day_view_finalize;
  object_class->dispose = gtk_day_view_dispose;

  widget_class = (GtkWidgetClass *) klass;
  widget_class->show = gtk_day_view_show;
}

static void
gtk_day_view_init (GTypeInstance *instance, gpointer klass)
{
  GtkDayView *day_view = GTK_DAY_VIEW (instance);

  day_view->reminders = 0;
  day_view->appointments = 0;
  day_view->sel_event = 0;
  day_view->date = (time_t) -1;
}

static void
gtk_day_view_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_day_view_finalize (GObject *object)
{
  GtkDayView *day_view;

  g_return_if_fail (object);
  g_return_if_fail (GTK_IS_DAY_VIEW (object));

  day_view = (GtkDayView *) object;

  gtk_widget_destroy (day_view->event_menu);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_day_view_show (GtkWidget *widget)
{
  GtkDayView *day_view = GTK_DAY_VIEW (widget);

  day_view_update (day_view, TRUE);

  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static gboolean
day_view_row_clicked (GtkWidget *widget, gint row, gpointer d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);
  struct tm tm;
  GtkWidget *w;

  gtk_widget_hide (day_view->event_menu);

  localtime_r (&viewtime, &tm);
  tm.tm_hour = row;
  tm.tm_min = 0;

  w = new_event (mktime (&tm), 1);
  gtk_widget_show (w);

  return FALSE;
}

static gboolean
day_view_event_clicked (GtkWidget *widget, gpointer event_p, gpointer d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);
  event_t event = event_p;
  gchar *buffer = NULL;
  gchar *tbuffer = NULL;
  gchar *strstart, *strend;
  event_details_t evd;
  struct tm start_tm, end_tm;
  time_t end;

  gtk_widget_hide (day_view->event_menu);

  day_view->sel_event = event;

  end = (time_t) (event->duration + event->start);

  localtime_r (&(event->start), &start_tm);
  localtime_r (&end, &end_tm);
  evd = event_db_get_details (event);
  buffer = g_malloc (sizeof (gchar) * 256);
  strstart = strftime_strdup_utf8_locale (TIMEFMT, &start_tm);
  strend = strftime_strdup_utf8_locale (TIMEFMT, &end_tm);

  snprintf (buffer, 256, "%s %s-%s %s\n\n%s",
	    evd->summary, strstart, strend,
	    (event->flags & FLAG_ALARM ? "(A)" : ""),
	    evd->description ? evd->description : "");
  g_free(strstart);
  g_free(strend);
  tbuffer = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
  gtk_label_set_text (GTK_LABEL (day_view->event_menu_info),
		      tbuffer ? tbuffer : (buffer ? buffer : ""));

  gtk_window_set_position (GTK_WINDOW (day_view->event_menu),
			   GTK_WIN_POS_MOUSE);

  gtk_widget_show (day_view->event_menu);

  if (tbuffer)
    g_free (tbuffer);
  if (buffer)
    g_free (buffer);

  return FALSE;
}

static void
day_view_update (GtkDayView *day_view, gboolean force)
{
  struct tm vt;
  time_t end, start;
  GSList *reminders, *cleanup;
  GSList *events, *appointments, *iter;

  localtime_r (&viewtime, &vt);

  /* Check if the datesel needs updating.  */
  {
    time_t dstime;
    struct tm dstm;

    dstime = gtk_date_sel_get_time (GTK_DATE_SEL (day_view->datesel));
    localtime_r (&dstime, &dstm);

    if (dstm.tm_year != vt.tm_year || dstm.tm_yday != vt.tm_yday)
      gtk_date_sel_set_time (GTK_DATE_SEL (day_view->datesel), viewtime);
  }

  /* Check if the calendar needs updating.  */
  {
    int day, month, year;

    gtk_calendar_get_date (GTK_CALENDAR (day_view->calendar),
			   &year, &month, &day);
    if (vt.tm_year != year - 1900 || vt.tm_mon != month || vt.tm_mday != day)
      {
	gtk_calendar_select_month (GTK_CALENDAR (day_view->calendar),
				   vt.tm_mon, vt.tm_year + 1900);
	gtk_calendar_select_day (GTK_CALENDAR (day_view->calendar),
				 vt.tm_mday);
      }

    if (force)
      gtk_event_cal_reload_events (GTK_EVENT_CAL (day_view->calendar));
  }

  /* Check if the day view needs updating.  */
  if (! force)
    {
      struct tm tm;
      localtime_r (&day_view->date, &tm);

      if (tm.tm_year == vt.tm_year && tm.tm_yday == vt.tm_yday)
	/* Same day, nothing to do.  */
	{
	  day_view->date = viewtime;
	  return;
	}
    }

  day_view->date = viewtime;

  vt.tm_hour = 0;
  vt.tm_min = 0;
  vt.tm_sec = 0;
  start = mktime (&vt);
  end = start + 30 * 60 * 60 - 1;

  /* Get the appointments for the current period.  */
  events = event_db_list_for_period (start, end);
  reminders = NULL;
  appointments = NULL;
  cleanup = NULL;
  for (iter = events; iter; iter = iter->next)
    {
      event_t ev = iter->data;
      if (is_reminder (ev))
	{
	  if (start <= ev->start && ev->start <= start + 24 * 60 * 60)
	    reminders = g_slist_append (reminders, ev);
	  else
	    cleanup = g_slist_append (cleanup, ev);
	}
      else
        appointments = g_slist_append (appointments, ev);
    }

  if (cleanup)
    event_db_list_destroy (cleanup);

  if (! reminders && day_view->reminders)
    /* There are no longer any reminders, destroy its render area.  */
    {
      gtk_container_remove (GTK_CONTAINER (day_view->event_box),
			    GTK_WIDGET (day_view->reminders));
      day_view->reminders = NULL;
    }

  if (reminders)
    {
      if (! day_view->reminders)
	/* We have reminders but no render area.  Create one.  */
	{
	  GdkGC *black_gc, *cream_gc;

	  black_gc = GTK_WIDGET (day_view)->style->black_gc;
	  cream_gc = pen_new (GTK_WIDGET (day_view), 65535, 64005, 51100);

	  day_view->reminders
	    = GTK_DAY_RENDER (gtk_day_render_new (start, 24 * 60 * 60,
						  1, 0, 0,
						  cream_gc, 1, FALSE,
						  reminders));

	  g_signal_connect (G_OBJECT (day_view->reminders), "event-clicked",
			    G_CALLBACK (day_view_event_clicked), day_view);
	  g_signal_connect (G_OBJECT (day_view->reminders), "row-clicked",
			    G_CALLBACK (day_view_row_clicked), day_view);

	  g_object_unref (cream_gc);
	  gtk_widget_show (GTK_WIDGET (day_view->reminders));

	  gtk_box_pack_start (GTK_BOX (day_view->event_box),
			      GTK_WIDGET (day_view->reminders),
			      FALSE, FALSE, 0);
	}
      else
	/* Set the new reminder list.  */
	{
	  gtk_day_render_set_date (day_view->reminders, start);
	  gtk_day_render_set_events (day_view->reminders, reminders);
	}
    }

  if (day_view->appointments)
    {
      gtk_day_render_set_date (day_view->appointments, start);
      gtk_day_render_set_events (day_view->appointments, appointments);
    }
  else
    {
      /* Don't want to infinge some patents here */
      GdkGC *black_gc, *post_him_yellow;

      black_gc = GTK_WIDGET (day_view)->style->black_gc;
      post_him_yellow = pen_new (GTK_WIDGET (day_view), 63222, 59110, 33667);

      day_view->appointments
	= GTK_DAY_RENDER (gtk_day_render_new (start, 30 * 60 * 60,
					      30, 7, 12,
					      post_him_yellow,
					      4, TRUE,
					      appointments));

      g_signal_connect (G_OBJECT (day_view->appointments), "event-clicked",
			G_CALLBACK (day_view_event_clicked), day_view);
      g_signal_connect (G_OBJECT (day_view->appointments), "row-clicked",
			G_CALLBACK (day_view_row_clicked), day_view);

      g_object_unref (post_him_yellow);

      gtk_widget_show (GTK_WIDGET (day_view->appointments));

      gtk_scrolled_window_add_with_viewport
	(GTK_SCROLLED_WINDOW (day_view->appointment_window),
	 GTK_WIDGET (day_view->appointments));

      gtk_viewport_set_shadow_type
	(GTK_VIEWPORT (GTK_WIDGET (day_view->appointments)->parent),
	 GTK_SHADOW_NONE);
    }
}

static gboolean
datesel_changed (GtkWidget *widget, GtkWidget *dv)
{
  GtkDayView *day_view = GTK_DAY_VIEW (dv);

  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  day_view_update (day_view, FALSE);

  return FALSE;
}

static void
calendar_changed (GtkWidget *widget, GtkWidget *dv)
{
  GtkDayView *day_view = GTK_DAY_VIEW (dv);
  int day, month, year;
  struct tm tm;

  localtime_r (&viewtime, &tm);
  gtk_calendar_get_date (GTK_CALENDAR (widget), &year, &month, &day);

  tm.tm_year = year - 1900;
  tm.tm_mon = month;
  tm.tm_mday = day;
  viewtime = mktime (&tm);

  day_view_update (day_view, FALSE);
}

/* 
 * Go to hour h
 * If h is negative it goes to the current hour
 */
static void
scroll_to (GtkDayView *day_view, gint hour)
{
  GtkAdjustment *adj;
  gint h = 0;
  gdouble upper, lower, value;

  /* Scroll to the current hour */
  if (hour < 0)
    {
      struct tm now_tm;
      localtime_r (&viewtime, &now_tm);
      hour = MAX (now_tm.tm_hour - 1, 0);
    }

  gtk_widget_get_size_request (GTK_WIDGET (day_view->appointments), NULL, &h);
  value = (gdouble) hour / NUM_HOURS * h;
  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
					     (day_view->appointment_window));

  lower = 0.0;
  upper = h;

  if (value > (adj->upper - adj->page_size))
    value = adj->upper - adj->page_size;

  day_view->scrolling = TRUE;
  gtk_adjustment_set_value (adj, value);
  day_view->scrolling = FALSE;
}

void
gtk_day_view_scroll (GtkDayView *day_view, gboolean force)
{
  if (force || day_view->scroll_floating)
    {
      scroll_to (day_view, -1);
      day_view->scroll_floating = TRUE;
    }
}

static void
sink_scroller (GtkAdjustment *adjustment, gpointer data)
{
  GtkDayView *day_view = GTK_DAY_VIEW (data);
  if (!day_view->scrolling)
    day_view->scroll_floating = FALSE;
}

static void
update_hook_callback (GtkWidget *dv)
{
  GtkDayView *day_view = GTK_DAY_VIEW (dv);

  struct tm tm;
  gtk_date_sel_set_time (GTK_DATE_SEL (day_view->datesel), viewtime);
  localtime_r (&viewtime, &tm);
  gtk_calendar_select_month (GTK_CALENDAR (day_view->calendar),
			     tm.tm_mon, tm.tm_year + 1900);
  gtk_calendar_select_day (GTK_CALENDAR (day_view->calendar), tm.tm_mday);

  /* Force a reload of the events.  */
  day_view_update (day_view, TRUE);
}

static gboolean
event_menu_destroy (GtkWidget * widget, GdkEventButton * event, gpointer d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);

  gtk_widget_hide (day_view->event_menu);
  day_view->sel_event = NULL;
  return TRUE;
}

static void
delete_event_cb (GtkWidget *widget, GtkWidget *dv)
{
  GtkDayView *day_view = GTK_DAY_VIEW (dv);
  event_t ev_real;
  event_details_t ev_d;
  recur_t r;

  gtk_widget_hide (day_view->event_menu);

  ev_real = get_cloned_ev (day_view->sel_event);
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
	      unschedule_alarm (ev_real, gtk_widget_get_toplevel (widget));
	      schedule_next (0, 0, widget);
	    }
	  event_db_remove (ev_real);
	}
      else
	{
	  r = event_db_get_recurrence (ev_real);
	  r->exceptions = g_slist_append (r->exceptions,
					  (void *) day_view->sel_event->start);
	  event_db_flush (ev_real);
	}
    }
  else
    {
      if (ev_real->flags & FLAG_ALARM)
	{
	  unschedule_alarm (ev_real, gtk_widget_get_toplevel (widget));
	  schedule_next (0, 0, widget);
	}
      event_db_remove (ev_real);
    }

  day_view_update (day_view, TRUE);
  event_db_forget_details (ev_real);
}

static void
edit_event_cb (GtkWidget *widget, GtkWidget *d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);
  GtkWidget *w;

  w = edit_event (day_view->sel_event);
  gtk_widget_show (w);
  gtk_widget_hide (day_view->event_menu);
}

static void
save_cb (GtkWidget *widget, GtkWidget *d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);

  gtk_widget_hide (day_view->event_menu);
  vcal_do_save (day_view->sel_event);
}

static void
send_ir_cb (GtkWidget *widget, GtkWidget *d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);

  gtk_widget_hide (day_view->event_menu);
  vcal_do_send_irda (day_view->sel_event);
}

static void
send_bt_cb (GtkWidget *widget, GtkWidget *d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);

  gtk_widget_hide (day_view->event_menu);
  vcal_do_send_bluetooth (day_view->sel_event);
}

static void
gtk_event_menu_new (GtkDayView *day_view)
{
  GtkWidget *frame;
  GtkWidget *vbox;

  day_view->event_menu = gtk_window_new (GTK_WINDOW_POPUP);
  frame = gtk_frame_new (NULL);
  vbox = gtk_vbox_new (FALSE, 0);

  gtk_container_add (GTK_CONTAINER (day_view->event_menu), frame);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* The event title.  */
  day_view->event_menu_info = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (day_view->event_menu_info), 0, 0);
  gtk_misc_set_padding (GTK_MISC (day_view->event_menu_info),
			gpe_get_border (), gpe_get_border ());
  gtk_box_pack_start (GTK_BOX (vbox), day_view->event_menu_info,
		      FALSE, TRUE, 0);

  /* Create an edit button.  */
  {
    GtkWidget *edit;

#ifdef IS_HILDON
    edit = gtk_button_new_with_label (_("Edit"));
#else
    edit = gtk_button_new_from_stock (GTK_STOCK_EDIT);
#endif
    gtk_button_set_relief (GTK_BUTTON (edit), GTK_RELIEF_NONE);
    gtk_button_set_alignment (GTK_BUTTON (edit), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), edit,
			FALSE, TRUE, 0);
    g_signal_connect (G_OBJECT (edit), "clicked",
		      G_CALLBACK (edit_event_cb), day_view);
  }

  /* A delete button.  */
  {
    GtkWidget *delete;

#ifdef IS_HILDON
    delete = gtk_button_new_with_label (_("Delete"));
#else
    delete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
#endif
    gtk_button_set_relief (GTK_BUTTON (delete), GTK_RELIEF_NONE);
    gtk_button_set_alignment (GTK_BUTTON (delete), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), delete,
			FALSE, TRUE, 0);
    g_signal_connect (G_OBJECT (delete), "clicked",
		      G_CALLBACK (delete_event_cb), day_view);
  }

  /* And a save button.  */
  {
    GtkWidget *save;
#ifdef IS_HILDON
    save = gtk_button_new_with_label (_("Save"));
#else
    save = gtk_button_new_from_stock (GTK_STOCK_SAVE);
#endif
    gtk_button_set_relief (GTK_BUTTON (save), GTK_RELIEF_NONE);
    gtk_button_set_alignment (GTK_BUTTON (save), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), save, FALSE, TRUE, 0);
    g_signal_connect (G_OBJECT (save), "clicked",
		      G_CALLBACK (save_cb), day_view);

  }

  /* Create a Send via infra-red button if infra-red is available.  */
  if (export_irda_available ())
    {
      GtkWidget *send_ir_button;

      send_ir_button = gtk_button_new_with_label (_("Send via infra-red"));
      gtk_button_set_relief (GTK_BUTTON (send_ir_button), GTK_RELIEF_NONE);
      gtk_button_set_alignment (GTK_BUTTON (send_ir_button), 0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), send_ir_button,
			  FALSE, TRUE, 0);
      g_signal_connect (G_OBJECT (send_ir_button), "clicked",
			G_CALLBACK (send_ir_cb), day_view);
    }

  /* Create a Send via Bluetooth button if bluetooth is available.  */
  if (export_bluetooth_available ())
    {
      GtkWidget *send_bt_button;

      send_bt_button = gtk_button_new_with_label (_("Send via Bluetooth"));
      gtk_button_set_relief (GTK_BUTTON (send_bt_button), GTK_RELIEF_NONE);
      gtk_button_set_alignment (GTK_BUTTON (send_bt_button), 0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), send_bt_button,
			  FALSE, TRUE, 0);
      g_signal_connect (G_OBJECT (send_bt_button), "clicked",
			G_CALLBACK (send_bt_cb), day_view);
    }

  gtk_widget_add_events (GTK_WIDGET (day_view->event_menu),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (day_view->event_menu), "button-press-event",
		    G_CALLBACK (event_menu_destroy), day_view);

  gtk_widget_show_all (frame);

  gtk_window_set_decorated (GTK_WINDOW (day_view->event_menu), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (day_view->event_menu), FALSE);
  gtk_window_set_modal (GTK_WINDOW (day_view->event_menu), TRUE);
}

GtkWidget *
gtk_day_view_new (void)
{
  GtkDayView *day_view;
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkAdjustment *adj;
  gboolean landscape;

  day_view = GTK_DAY_VIEW (g_object_new (gtk_day_view_get_type (), NULL));
  gtk_event_menu_new (day_view);

  day_view->event_box = gtk_vbox_new (FALSE, 0);

  day_view->appointment_window = gtk_scrolled_window_new (NULL, NULL);
  adj = gtk_scrolled_window_get_vadjustment
    (GTK_SCROLLED_WINDOW (day_view->appointment_window));
  g_signal_connect (G_OBJECT (adj), "value-changed",
		    G_CALLBACK (sink_scroller), (gpointer) day_view);

  landscape = gdk_screen_width () > gdk_screen_height ()
    && gdk_screen_width () >= 640;

  day_view->calendar = gtk_event_cal_new ();
  GTK_WIDGET_UNSET_FLAGS (day_view->calendar, GTK_CAN_FOCUS);

  gtk_widget_show (day_view->appointment_window);
  gtk_widget_show (hbox);
  gtk_widget_show (day_view->event_box);

  gtk_calendar_set_display_options (GTK_CALENDAR (day_view->calendar),
				    GTK_CALENDAR_SHOW_DAY_NAMES
				    | (week_starts_monday ?
				       GTK_CALENDAR_WEEK_START_MONDAY : 0));

  day_view->datesel = gtk_date_sel_new (GTKDATESEL_FULL, viewtime);

  gtk_widget_show (day_view->datesel);
  
  g_signal_connect (G_OBJECT (day_view->calendar),
		    "day-selected", G_CALLBACK (calendar_changed),
		    day_view);

  gtk_scrolled_window_set_policy
    (GTK_SCROLLED_WINDOW (day_view->appointment_window),
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GTK_VBOX (day_view);
  gtk_box_pack_start (GTK_BOX (day_view), day_view->datesel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (day_view), hbox, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), day_view->event_box, TRUE, TRUE, 0);
  
  gtk_box_pack_end (GTK_BOX (day_view->event_box),
		    day_view->appointment_window,
		    TRUE, TRUE, 0);

#ifndef IS_HILDON
  if (landscape)
    {
      GtkWidget *sep;
      sep = gtk_vseparator_new ();

      gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), day_view->calendar, FALSE, FALSE, 0);

      gtk_widget_show (sep);
      gtk_widget_show (day_view->calendar);
    }
#endif

  g_object_set_data (G_OBJECT (day_view), "update_hook",
		     (gpointer) update_hook_callback);

  g_signal_connect (G_OBJECT (day_view->datesel), "changed",
		    G_CALLBACK (datesel_changed), day_view);
    
  gtk_widget_add_events (GTK_WIDGET (day_view->datesel),
			 GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  return GTK_WIDGET (day_view);
}
