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
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <gtk/gtk.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include <gpe/event-db.h>
#include "event-ui.h"
#include "globals.h"

#include "day_view.h"
#include "week_view.h"
#include "future_view.h"
#include "month_view.h"

#include <libdm.h>

#define _(_x) gettext (_x)

extern gboolean gpe_calendar_start_xsettings (void);

GList *times;
time_t viewtime;

GtkWidget *main_window, *pop_window;
GtkWidget *notebook;

struct gpe_icon my_icons[] = {
  { "future_view", "future_view" },
  { "day_view", "day_view" },
  { "week_view", "week_view" },
  { "month_view", "month_view" },
  { "bell", "bell" },
  { "recur", "recur" },
  { "bell_recur", "bell_recur" },
  { "icon", PREFIX "/share/pixmaps/gpe-calendar.png" },
  { NULL, NULL }
};

static GtkWidget *day, *week, *month, *future, *current_view;
static GtkWidget *day_button, *week_button, *month_button, *future_button;

guint window_x = 240, window_y = 310;

gboolean week_starts_monday = TRUE;
gboolean day_view_combined_times;

static guint nr_days[] = { 31, 28, 31, 30, 31, 30, 
			   31, 31, 30, 31, 30, 31 };

guint
days_in_month (guint year, guint month)
{
  if (month == 1)
    {
      return ((year % 4) == 0
	      && ((year % 100) != 0
		  || (year % 400) == 0)) ? 29 : 28;
    }

  return nr_days[month];
}

void
update_current_view (void)
{
  if (current_view)
    {
      gpointer p = g_object_get_data (G_OBJECT (current_view), "update_hook");
      if (p)
	{
	  void (*f)(void) = p;
	  f ();
	}
    }
}

static void
new_view (GtkWidget *widget)
{
  guint i = 0;
  GtkWidget *w;

  if (pop_window)
    gtk_widget_destroy (pop_window);

  do
    {
      w = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), i);
      if (w == widget)
	{
	  current_view = w;
	  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), i);
	  return;
	}
      i++;
    } while (w != NULL);
}

void
set_day_view (void)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (day_button), TRUE);
  new_view (day);
  update_current_view ();
}

void
init_views (void)
{
  new_view (day);
  update_current_view ();
  new_view (week);
  update_current_view ();
  new_view (month);
  update_current_view ();
  new_view (future);
  update_current_view ();
}

static void
new_appointment (void)
{
  GtkWidget *appt = new_event (viewtime, 0);
  gtk_widget_show_all (appt);
}

static void
set_today(void)
{
  time (&viewtime);
  update_current_view ();
}

static void
button_toggled (GtkWidget *widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    new_view (data);
}

static void
gpe_cal_exit (void)
{
  schedule_next (0, 0);
  event_db_stop ();
  gtk_main_quit ();
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *toolbar;
  GdkPixbuf *p;
  GtkWidget *pw;

  guint hour, skip=0, uid=0;
  int option_letter;
  gboolean schedule_only=FALSE;
  extern char *optarg;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  libdm_init ();

  if (event_db_start () == FALSE)
    exit (1);

  while ((option_letter = getopt (argc, argv, "s:e:")) != -1)
    {
      if (option_letter == 's')
	{
	  skip = atol (optarg);
	  schedule_only = TRUE;
	}

      if (option_letter == 'e')
	uid = atol (optarg);
    }

  schedule_next (skip, uid);

  if (schedule_only)
    exit (EXIT_SUCCESS);

  for (hour = 0; hour < 24; hour++)
    {
      char buf[32];
      struct tm tm;
      time_t t=time(NULL);

      localtime_r (&t, &tm);
      tm.tm_hour = hour;
      tm.tm_min = 0;
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      times = g_list_append (times, g_strdup (buf));
      tm.tm_hour = hour;
      tm.tm_min = 30;
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      times = g_list_append (times, g_strdup (buf));
    }

  vbox = gtk_vbox_new (FALSE, 0);
  notebook = gtk_notebook_new ();

  time (&viewtime);
  week = week_view ();
  day = day_view ();
  month = month_view ();
  future = future_view ();

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), _("Calendar"));
  g_signal_connect (G_OBJECT (main_window), "destroy",
                    G_CALLBACK (gpe_cal_exit), NULL);

  libdm_mark_window (main_window);

  gtk_widget_realize (main_window);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("New appointment"), _("Tap here to add a new appointment or reminder"),
			    G_CALLBACK (new_appointment), NULL, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  pw = gtk_image_new_from_stock (GTK_STOCK_HOME, gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Today"),
			   _("Today"), _("Today"), pw, set_today, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon_scaled ("future_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  future_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					      GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL,
					      _("Future"), _("Future View"),
					      _("Tap here to see all future events."),
					      pw, GTK_SIGNAL_FUNC (button_toggled), future);

  p = gpe_find_icon_scaled ("day_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  day_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					   GTK_TOOLBAR_CHILD_RADIOBUTTON, future_button,
					   ("Day"), _("Day View"),
					   _("Tap here to select day-at-a-time view."),
					   pw, GTK_SIGNAL_FUNC (button_toggled), day);
  
  p = gpe_find_icon_scaled ("week_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  week_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					    GTK_TOOLBAR_CHILD_RADIOBUTTON, day_button,
					    _("Week"), _("Week View"),
					    _("Tap here to select week-at-a-time view."),
					    pw, GTK_SIGNAL_FUNC (button_toggled), week);

  p = gpe_find_icon_scaled ("month_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  month_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					     GTK_TOOLBAR_CHILD_RADIOBUTTON, week_button,
					     _("Month"), _("Month View"),
					     _("Tap here to select month-at-a-time view."),
					     pw, GTK_SIGNAL_FUNC (button_toggled), month);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    _("Exit"), _("Tap here to exit the program"),
			    G_CALLBACK (gpe_cal_exit), NULL, -1);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

  gtk_widget_show (day);
  gtk_widget_show (week);
  gtk_widget_show (month);
  gtk_widget_show (future);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), day, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), week, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), month, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), future, NULL);

  gtk_widget_show (notebook);

  gtk_container_add (GTK_CONTAINER (main_window), vbox);

  gtk_window_set_default_size (GTK_WINDOW (main_window), window_x, window_y);

  gpe_set_window_icon (main_window, "icon");

  gtk_widget_show (main_window);
  gtk_widget_show (vbox);
  gtk_widget_show (toolbar);

  gpe_calendar_start_xsettings ();

  init_views();

  set_day_view ();

  gtk_main ();

  return 0;
}
