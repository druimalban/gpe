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
#include <time.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#include "event-db.h"
#include "globals.h"

#include "day_view.h"
#include "week_view.h"
#include "future_view.h"
#include "year_view.h"

extern void about (void);

GdkFont *timefont;
GdkFont *datefont;
GdkFont *yearfont;
GtkWidget *day_list, *future_list;
GList *times;
time_t viewtime;

static GtkWidget *window, *day, *week, *future, *year, *current_view;

guint window_x = 240, window_y = 310;

void
update_current_view (void)
{
  if (current_view)
    {
      gpointer p = gtk_object_get_data (GTK_OBJECT (current_view),
					"update_hook");
      if (p)
	{
	  void (*f)(void) = p;
	  f();
	}
    }
}

static void
new_view (GtkWidget *widget)
{
  if (current_view)
    gtk_widget_hide (current_view);

  gtk_widget_show_all (widget);
  current_view = widget;

  update_current_view ();
}

static void
set_day_view(void)
{
  new_view (day);
}

static void
set_week_view(void)
{
  new_view (week);
}

static void
set_future_view(void)
{
  new_view (future);
}

static void
set_year_view(void)
{
  new_view (year);
}

static void
new_appointment(void)
{
  GtkWidget *appt = new_event (time (NULL), 0, NULL);
  gtk_widget_show (appt);
}

static GtkItemFactoryEntry menu_items[] = {
  { "/_File",              NULL,         NULL, 0, "<Branch>" },
  { "/File/Quit",          "",           gtk_main_quit, 0, NULL },
  { "/_View",              NULL,         NULL, 0, "<Branch>" },
  { "/View/_Future",	   NULL,	 set_future_view, 0, NULL },
  { "/View/_Day",          NULL,         set_day_view, 0, NULL },
  { "/View/_Week",         NULL,         set_week_view, 0, NULL },
  { "/View/_Year",         NULL,         set_year_view, 0, NULL },
  { "/_New",               NULL,         NULL, 0, "<Branch>" },
  { "/New/_Appointment",   NULL,         new_appointment, 0, NULL },
  { "/_Help"       ,       NULL,         NULL, 0, "<LastBranch>" },
  { "/Help/_About",        NULL,         about, 0, NULL },
};

int
main(int argc, char *argv[])
{
  GtkWidget *menubar;
  GtkWidget *vbox;
  GtkItemFactory *item_factory;
  guint hour;

  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

  event_db_start ();

  gtk_set_locale ();
  gtk_init(&argc, &argv);
  gdk_imlib_init ();

  for (hour = 0; hour < 24; hour++)
    {
      char buf[32];
      struct tm tm;
      time_t t=time(NULL);
      
      localtime_r (&t, &tm);
      tm.tm_hour = hour;
      tm.tm_min = 0;
      strftime (buf, sizeof(buf), "%X", &tm);
      times = g_list_append (times, g_strdup (buf));
      tm.tm_hour = hour;
      tm.tm_min = 30;
      strftime (buf, sizeof(buf), "%X", &tm);
      times = g_list_append (times, g_strdup (buf));
    }

  vbox = gtk_vbox_new (FALSE, 0);

  timefont = gdk_font_load ("-*-*-medium-r-normal--10-*-*-*-*-*-*");
  if (timefont == NULL)
    {
      printf ("Couldn't get time font\n");
      abort ();
    }
  datefont = gdk_font_load ("-*-*-medium-r-normal--8-*-*-*-c-*-*");
  if (datefont == NULL)
    {
      printf ("Couldn't get date font\n");
      abort ();
    }
  yearfont = gdk_font_load ("-*-*-medium-r-normal--8-*-*-*-*-*-*");
  if (datefont == NULL)
    {
      printf ("Couldn't get year font\n");
      abort ();
    }

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
                                       NULL);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  time (&viewtime);
  week = week_view ();
  day = day_view ();
  future = future_view ();
  year = year_view ();

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), day, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), week, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), future, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), year, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (menubar);

  new_view (day);

  gtk_main ();

  return 0;
}
