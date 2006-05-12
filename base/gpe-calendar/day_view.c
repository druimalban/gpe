/*
 * Copyright (C) 2001, 2002, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

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
#include <gpe/spacing.h>
#include "view.h"
#include "event-ui.h"
#include "globals.h"
#include "day_view.h"
#include "day_render.h"
#include "export-vcal.h"

#define _(x) gettext(x)
#define NUM_HOURS 24

struct _GtkDayView
{
  GtkView widget;

  /*
      Basic layout of a DayView:

       /-DayView----------------\
       | /-reminders----------\ |
       | |                    | |
       | \--------------------/ |
       | /-appointment_window-\ |
       | |/-appointments-----\| |
       | ||                  || |
       | |\------------------/| |
       | \--------------------/ |
       \------------------------/
   */

  gboolean scrolling;
  gboolean scroll_floating;

  GtkDayRender *reminders;
  GtkWidget *appointment_window;
  GtkDayRender *appointments;

  /* Currently selected event (if any).  */
  Event *sel_event;

  /* A popup menu for events.  */
  GtkWidget *event_menu;
  /* A label describing the clicked event.  */
  GtkWidget *event_menu_info;
};

typedef struct
{
  GtkViewClass view_class;
  GObjectClass parent_class;
} DayViewClass;

static void gtk_day_view_base_class_init (gpointer klass);
static void gtk_day_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_day_view_dispose (GObject *obj);
static void gtk_day_view_finalize (GObject *object);
static gboolean gtk_day_view_key_press_event (GtkWidget *widget,
					      GdkEventKey *event);
static void gtk_day_view_set_time (GtkView *view, time_t time);
static void gtk_day_view_reload_events (GtkView *view);

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

      type = g_type_register_static (gtk_view_get_type (),
				     "GtkDayView", &info, 0);
    }

  return type;
}

static void
gtk_day_view_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_view_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_day_view_finalize;
  object_class->dispose = gtk_day_view_dispose;

  widget_class = (GtkWidgetClass *) klass;
  widget_class->key_press_event = gtk_day_view_key_press_event;

  view_class = (GtkViewClass *) klass;
  view_class->set_time = gtk_day_view_set_time;
  view_class->reload_events = gtk_day_view_reload_events;
}

static void
gtk_day_view_init (GTypeInstance *instance, gpointer klass)
{
  GtkDayView *day_view = GTK_DAY_VIEW (instance);

  day_view->reminders = 0;
  day_view->appointments = 0;
  day_view->sel_event = 0;
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

static gboolean
gtk_day_view_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  switch (event->keyval)
    {
    case GDK_Left:
    case GDK_Up:
    case GDK_Page_Up:
      gtk_view_set_time (GTK_VIEW (widget),
			 gtk_view_get_time (GTK_VIEW (widget))
			 - 24 * 60 * 60);
      return TRUE;
    case GDK_Right:
    case GDK_Down:
    case GDK_Page_Down:
      gtk_view_set_time (GTK_VIEW (widget),
			 gtk_view_get_time (GTK_VIEW (widget))
			 + 24 * 60 * 60);
      return TRUE;
    default:
      return FALSE;
    }
}

static void
gtk_day_view_set_time (GtkView *view, time_t current)
{
  time_t new = gtk_view_get_time (view);
  struct tm c_tm;
  struct tm n_tm;

  localtime_r (&current, &c_tm);
  localtime_r (&new, &n_tm);

  if (c_tm.tm_year != n_tm.tm_year || c_tm.tm_yday != n_tm.tm_yday)
    /* Day changed.  */
    gtk_day_view_reload_events (view);
}

static gboolean
day_view_row_clicked (GtkWidget *widget, gint row, gpointer d)
{
  GtkDayView *day_view = GTK_DAY_VIEW (d);
  time_t t = gtk_view_get_time (GTK_VIEW (day_view));
  struct tm tm;
  GtkWidget *w;

  gtk_widget_hide (day_view->event_menu);

  localtime_r (&t, &tm);
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
  Event *event = event_p;
  gchar *tbuffer = NULL;
  gchar *strstart, *strend;
  struct tm start_tm, end_tm;

  gtk_widget_hide (day_view->event_menu);

  day_view->sel_event = event;

  time_t start = event_get_start (event);
  time_t end = (time_t) (start + event_get_duration (event));

  localtime_r (&start, &start_tm);
  strstart = strftime_strdup_utf8_locale (TIMEFMT, &start_tm);

  if (end == start)
    strend = 0;
  else
    {
      localtime_r (&end, &end_tm);
      strend = strftime_strdup_utf8_locale (TIMEFMT, &end_tm);
    }

  const char *summary = event_get_summary (event);
  const char *description = event_get_description (event);

  char buffer[64];
  snprintf (buffer, 64, "%s %s%s%s %s%s%s",
	    summary,
	    strstart, strend ? "-" : "", strend ?: "",
	    event_get_alarm (event) ? "(A)" : "",
	    description ? "\n" : "",
	    description ? description : "");
  buffer[64] = 0;
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

  return FALSE;
}

static void
gtk_day_view_reload_events (GtkView *view)
{
  GtkDayView *day_view = GTK_DAY_VIEW (view);
  time_t t = gtk_view_get_time (view);
  struct tm vt;
  time_t end, start;
  GSList *reminders;
  GSList *events, *appointments, *iter;

  localtime_r (&t, &vt);

  vt.tm_hour = 0;
  vt.tm_min = 0;
  vt.tm_sec = 0;
  start = mktime (&vt);
  end = start + 30 * 60 * 60 - 1;

  /* Get the appointments for the current period.  */
  events = event_db_list_for_period (event_db, start, end);
  reminders = NULL;
  appointments = NULL;
  for (iter = events; iter; iter = iter->next)
    {
      Event *ev = iter->data;
      if (is_reminder (ev))
	{
	  if (start <= event_get_start (ev)
	      && event_get_start (ev) < start + 24 * 60 * 60)
	    reminders = g_slist_append (reminders, ev);
	  else
	    g_object_unref (ev);
	}
      else
	appointments = g_slist_append (appointments, ev);
    }

  g_slist_free (events);

  if (! reminders && day_view->reminders)
    /* There are no longer any reminders, destroy its render area.  */
    {
      gtk_container_remove (GTK_CONTAINER (day_view),
			    GTK_WIDGET (day_view->reminders));
      day_view->reminders = NULL;
    }

  if (reminders)
    {
      if (! day_view->reminders)
	/* We have reminders but no render area.  Create one.  */
	{
	  GdkColor cream;
	  cream.red = 65535;
	  cream.green = 64005;
	  cream.blue = 51100;

	  day_view->reminders
	    = GTK_DAY_RENDER (gtk_day_render_new (start, 24 * 60 * 60,
						  1, 0, 0,
						  cream, 1, FALSE,
						  reminders));

	  g_signal_connect (G_OBJECT (day_view->reminders), "event-clicked",
			    G_CALLBACK (day_view_event_clicked), day_view);
	  g_signal_connect (G_OBJECT (day_view->reminders), "row-clicked",
			    G_CALLBACK (day_view_row_clicked), day_view);

	  gtk_widget_show (GTK_WIDGET (day_view->reminders));

	  gtk_box_pack_start (GTK_BOX (day_view),
			      GTK_WIDGET (day_view->reminders),
			      FALSE, FALSE, 0);
	}
      else
	/* Set the new reminder list.  */
	gtk_day_render_set_events (day_view->reminders, reminders, start);
    }

  if (day_view->appointments)
    gtk_day_render_set_events (day_view->appointments, appointments, start);
  else
    {
      /* Don't want to infinge some patents here */
      GdkColor post_him_yellow;
      post_him_yellow.red = 63222;
      post_him_yellow.green = 59110;
      post_him_yellow.blue = 33667;

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

      gtk_widget_show (GTK_WIDGET (day_view->appointments));

      gtk_scrolled_window_add_with_viewport
	(GTK_SCROLLED_WINDOW (day_view->appointment_window),
	 GTK_WIDGET (day_view->appointments));

      gtk_viewport_set_shadow_type
	(GTK_VIEWPORT (GTK_WIDGET (day_view->appointments)->parent),
	 GTK_SHADOW_NONE);
    }
}

/* 
 * Go to hour h
 * If h is negative it goes to the current hour
 */
static void
scroll_to (GtkDayView *day_view, gint hour)
{
  time_t t = gtk_view_get_time (GTK_VIEW (day_view));
  GtkAdjustment *adj;
  gint h = 0;
  gdouble upper, lower, value;

  /* Scroll to the current hour */
  if (hour < 0)
    {
      struct tm now_tm;
      localtime_r (&t, &now_tm);
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
  Event *ev = day_view->sel_event;

  gtk_widget_hide (day_view->event_menu);

  if (event_is_recurrence (ev))
    {
      if (gpe_question_ask
	  (_
	   ("Delete all recurring entries?\n"
	    "(If no, delete this instance only)"),
	   _("Question"), "question", "!gtk-no", NULL, "!gtk-yes", NULL,
	   NULL))
	event_remove (ev);
      else
	{
	  event_add_recurrence_exception (ev, event_get_start (ev));
	  event_flush (ev);
	}
    }
  else
    event_remove (ev);

  update_view ();
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
gtk_day_view_new (time_t time)
{
  GtkDayView *day_view;
  GtkAdjustment *adj;

  day_view = GTK_DAY_VIEW (g_object_new (gtk_day_view_get_type (), NULL));
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (day_view), GTK_CAN_FOCUS);
  gtk_event_menu_new (day_view);

  day_view->appointment_window = gtk_scrolled_window_new (NULL, NULL);
  adj = gtk_scrolled_window_get_vadjustment
    (GTK_SCROLLED_WINDOW (day_view->appointment_window));
  g_signal_connect (G_OBJECT (adj), "value-changed",
		    G_CALLBACK (sink_scroller), (gpointer) day_view);
  gtk_scrolled_window_set_policy
    (GTK_SCROLLED_WINDOW (day_view->appointment_window),
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end (GTK_BOX (day_view), day_view->appointment_window,
		    TRUE, TRUE, 0);
  gtk_widget_show (day_view->appointment_window);

  gtk_view_set_time (GTK_VIEW (day_view), time);

  return GTK_WIDGET (day_view);
}
