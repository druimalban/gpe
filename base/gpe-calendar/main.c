/*
 * Copyright (C) 2001 Philip Blundell <philb@gnu.org>
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

#include "globals.h"

extern GtkWidget *day_view(void);
extern GtkWidget *week_view(void);
extern GtkWidget *future_view(void);
extern void about(void);

extern GtkWidget *new_event(time_t t, guint timesel);

GdkFont *timefont;
GdkFont *datefont;
GList *times;

GtkWidget *window, *day, *week, *future;

guint window_x = 240, window_y = 320;

static void
set_day_view(void)
{
  gtk_widget_hide(week);
  gtk_widget_hide(future);
  gtk_widget_show_all(day);
}

static void
set_week_view(void)
{
  gtk_widget_hide(day);
  gtk_widget_hide(future);
  gtk_widget_show_all(week);
}

static void
set_future_view(void)
{
  gtk_widget_hide(day);
  gtk_widget_hide(week);
  gtk_widget_show_all(future);
}

static void
new_appointment(void)
{
  GtkWidget *appt = new_event(time(NULL), 0);
  gtk_widget_show (appt);
}

static GtkItemFactoryEntry menu_items[] = {
  { "/_File",              NULL,         NULL, 0, "<Branch>" },
  { "/File/Quit",          "",           gtk_main_quit, 0, NULL },
  { "/_View",              NULL,         NULL, 0, "<Branch>" },
  { "/View/_Future",	   NULL,	 set_future_view, 0, NULL },
  { "/View/_Day",          NULL,         set_day_view, 0, NULL },
  { "/View/_Week",         NULL,         set_week_view, 0, NULL },
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

  gtk_set_locale ();
  gtk_init(&argc, &argv);
  gdk_imlib_init ();

  for (hour = 0; hour < 24; hour++)
    {
      char buf[32];
      g_snprintf (buf, sizeof(buf), "%02d:%02d", hour, 0);
      times = g_list_append(times, g_strdup (buf));
      g_snprintf (buf, sizeof(buf), "%02d:%02d", hour, 30);
      times = g_list_append(times, g_strdup (buf));
    }

  vbox = gtk_vbox_new(FALSE, 0);

  timefont = gdk_font_load("-*-*-medium-r-normal--12-*-*-*-*-*-*");
  if (timefont == NULL)
    {
      printf("Couldn't get time font\n");
      abort();
    }
  datefont = gdk_font_load("-*-*-medium-r-normal--9-*-*-*-c-*-*");
  if (datefont == NULL)
    {
      printf("Couldn't get date font\n");
      abort();
    }

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
                                       NULL);
  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  week = week_view();
  day = day_view();
  future = future_view();

  gtk_widget_set_usize (GTK_WIDGET (menubar), -1, 20);

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), day, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);

  gtk_widget_show_all(window);
  gtk_container_add(GTK_CONTAINER(vbox), week);
  week->allocation = day->allocation;  
  gtk_container_add(GTK_CONTAINER(vbox), future);
  future->allocation = day->allocation;  

  gtk_main();

  return 0;
}
