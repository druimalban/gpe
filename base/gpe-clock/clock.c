/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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

#include <gpe/tray.h>
#include <gpe/init.h>
#include <gpe/popup.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/gtktimesel.h>

GtkWidget *panel_window, *time_label;

#define _(x) gettext(x)

static gboolean show_seconds;
static gboolean format_24 = TRUE;

static void
update_time (GtkWidget *label)
{
  char buf[256];
  time_t t;
  struct tm tm;
  time (&t);
  localtime_r (&t, &tm);
  const char *format;

  if (format_24)
    {
      format = show_seconds ? "%H:%M:%S" : "%H:%M";
    }
  else
    {
      format = show_seconds ? "%I:%M:%S%p" : "%I:%M%p";
    }
  
  strftime (buf, sizeof (buf), format, &tm);
  gtk_label_set_text (GTK_LABEL (label), buf);
}

static void
set_seconds (GtkWidget *w, GtkWidget *time_label)
{
  show_seconds = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
  update_time (time_label);
}

static void
set_format (GtkWidget *w, GtkWidget *time_label)
{
  format_24 = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
  update_time (time_label);
}

static void
prefs_window (GtkWidget *w, GtkWidget *time_label)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  GtkWidget *format_12_button;
  GtkWidget *format_24_button;
  GtkWidget *seconds_button;

  GSList    *radiogroup;
  int spacing = gpe_get_boxspacing ();

  gtk_window_set_title (GTK_WINDOW (window), _("Clock preferences"));

  format_12_button = gtk_radio_button_new_with_label (NULL, _("12-hour format"));
  radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (format_12_button));
  format_24_button = gtk_radio_button_new_with_label (radiogroup, _("24-hour format"));

  seconds_button = gtk_check_button_new_with_label (_("Show seconds"));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (seconds_button), show_seconds);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (format_24_button), format_24);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), format_12_button, FALSE, FALSE, spacing);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), format_24_button, FALSE, FALSE, spacing);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), seconds_button, FALSE, FALSE, spacing);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (window), gpe_get_border ());

  gtk_widget_show_all (window);

  g_signal_connect (G_OBJECT (seconds_button), "toggled", G_CALLBACK (set_seconds), time_label);
  g_signal_connect (G_OBJECT (format_24_button), "toggled", G_CALLBACK (set_format), time_label);
 
  g_signal_connect_swapped (G_OBJECT (ok_button), "clicked", G_CALLBACK (gtk_widget_destroy), window);
}

static void
set_time_window (void)
{
  if (fork () == 0)
    {
      execlp ("gpe-conf", "gpe-conf", "time", NULL);
      gpe_perror_box ("gpe-conf");
      _exit (1);
    }
}

static void
do_set_alarm (GtkWidget *window)
{
  gtk_widget_destroy (window);
}

static void
alarm_window (void)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  GtkWidget *cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  GtkWidget *time_label = gtk_label_new (_("Time:"));
  GtkWidget *date_button, *days_button;
  GtkWidget *date_entry = gtk_date_combo_new ();
  GtkWidget *date_hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *time_hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *time_sel = gtk_time_sel_new ();
  GtkWidget *clock = clock_widget (GTK_TIME_SEL (time_sel)->minute_adj);
  int spacing = gpe_get_boxspacing ();
  GSList *radiogroup;

  gtk_window_set_title (GTK_WINDOW (window), _("Set alarm"));

  date_button = gtk_radio_button_new_with_label (NULL, _("Date:"));
  radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (date_button));
  days_button = gtk_radio_button_new_with_label (radiogroup, _("Each week, on:"));

  gtk_box_pack_start (GTK_BOX (date_hbox), date_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (date_hbox), date_entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (time_hbox), time_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (time_hbox), time_sel, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), clock, TRUE, TRUE, spacing);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), time_hbox, FALSE, FALSE, spacing);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), date_hbox, FALSE, FALSE, spacing);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), days_button, FALSE, FALSE, spacing);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (window), gpe_get_border ());

  gtk_widget_show_all (window);

  g_signal_connect_swapped (G_OBJECT (ok_button), "clicked", G_CALLBACK (do_set_alarm), window);
  g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked", G_CALLBACK (gtk_widget_destroy), window);
}

static void
clicked (GtkWidget *w, GdkEventButton *ev, GtkWidget *menu)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gpe_popup_menu_position, w, ev->button, ev->time);
}

int
main (int argc, char *argv[])
{
  GtkWidget *menu, *menu_set_time, *menu_alarm, *menu_remove, *menu_prefs;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);
  
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  panel_window = gtk_plug_new (0);

  time_label = gtk_label_new (NULL);

  update_time (time_label);

  gtk_container_add (GTK_CONTAINER (panel_window), time_label);

  gtk_widget_realize (panel_window);

  gtk_widget_show_all (panel_window);

  gpe_system_tray_dock (panel_window->window);

  menu = gtk_menu_new ();
  menu_prefs = gtk_menu_item_new_with_label (_("Preferences"));
  menu_set_time = gtk_menu_item_new_with_label (_("Set the time"));
  menu_alarm = gtk_menu_item_new_with_label (_("Alarm ..."));
  menu_remove = gtk_menu_item_new_with_label (_("Remove from panel"));

  gtk_widget_show (menu_prefs);
  gtk_widget_show (menu_set_time);
  gtk_widget_show (menu_alarm);
  gtk_widget_show (menu_remove);

  gtk_menu_append (GTK_MENU (menu), menu_alarm);
  gtk_menu_append (GTK_MENU (menu), menu_set_time);
  gtk_menu_append (GTK_MENU (menu), menu_prefs);
  gtk_menu_append (GTK_MENU (menu), menu_remove);

  g_signal_connect (G_OBJECT (menu_prefs), "activate", G_CALLBACK (prefs_window), time_label);
  g_signal_connect (G_OBJECT (menu_set_time), "activate", G_CALLBACK (set_time_window), NULL);
  g_signal_connect (G_OBJECT (menu_alarm), "activate", G_CALLBACK (alarm_window), NULL);
  g_signal_connect (G_OBJECT (menu_remove), "activate", G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (panel_window), "button-press-event", G_CALLBACK (clicked), menu);
  gtk_widget_add_events (panel_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_timeout_add (1000, (GtkFunction) update_time, time_label);

  gtk_main ();

  exit (0);
}
