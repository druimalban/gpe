/*
 * Copyright (C) 2001 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>
#include <stdio.h>

#include "cotidie.h"
#include "event.h"

void
event_add(event_t ev)
{
  printf("Add event, time %x/%x text %s\n", ev->start, ev->end, ev->text);
}

struct date
{
  guint year, month, day;
  GtkWidget *entry;
  GtkWidget *cal;
  GtkWidget *button;
};

struct sens
{
  GtkWidget *frombutton;
  GtkWidget *startentry;
  GtkWidget *endentry;
  GtkWidget *timeend;

  GtkWidget *alarmbutton;
  GtkWidget *alarmspin;
  GtkWidget *alarmoption;

  GtkWidget *repeatbutton;
  GtkWidget *repeatspin;
  GtkWidget *repeatoption;

  GtkWidget *text;
  
  struct date start, end;
};

static void
destroy_user_data (gpointer p)
{
  struct sens *s = (struct sens *)p;

  if (s->start.cal)
    {
      gtk_widget_hide (s->start.cal);
      gtk_widget_destroy (s->start.cal);
      s->start.cal = NULL;
    }
  if (s->end.cal)
    {
      gtk_widget_hide (s->end.cal);
      gtk_widget_destroy (s->end.cal);
      s->start.cal = NULL;
    }

  g_free (p);
}

static void
recalculate_sensitivities(GtkWidget *widget,
			  GtkWidget *d)
{
  struct sens *s = gtk_object_get_data (GTK_OBJECT (d), "sens_list");

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->frombutton)))
    {
      gtk_widget_set_sensitive (s->startentry, 1);
      gtk_widget_set_sensitive (s->endentry, 1);
      gtk_widget_set_sensitive (s->timeend, 1);
    }
  else
    {
      gtk_widget_set_sensitive (s->startentry, 0);
      gtk_widget_set_sensitive (s->endentry, 0);
      gtk_widget_set_sensitive (s->timeend, 0);
    }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->alarmbutton)))
    {
      gtk_widget_set_sensitive (s->alarmoption, 1);
      gtk_widget_set_sensitive (s->alarmspin, 1);
    }
  else
    {
      gtk_widget_set_sensitive (s->alarmoption, 0);
      gtk_widget_set_sensitive (s->alarmspin, 0);
    }
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (s->repeatbutton)))
    {
      gtk_widget_set_sensitive (s->repeatoption, 1);
      gtk_widget_set_sensitive (s->repeatspin, 1);
    }
  else
    {
      gtk_widget_set_sensitive (s->repeatoption, 0);
      gtk_widget_set_sensitive (s->repeatspin, 0);
    }
}

static void
click_ok(GtkWidget *widget,
       GtkWidget *d)
{
  struct sens *s = gtk_object_get_data (GTK_OBJECT (d), "sens_list");
  struct event_s ev;
  struct tm tm;

  memset(&ev, 0, sizeof(ev));

  ev.text = gtk_editable_get_chars (GTK_EDITABLE (s->text), 0, -1);

  memset(&tm, 0, sizeof(tm));
  tm.tm_year = s->start.year - 1900;
  tm.tm_mon = s->start.month;
  tm.tm_mday = s->start.day;

  ev.start = mktime(&tm);

  event_add (&ev);

  g_free(ev.text);

  gtk_widget_hide(d);
  gtk_widget_destroy(d);
}

static void
click_cancel(GtkWidget *widget,
	   GtkWidget *d)
{
  gtk_widget_hide(d);
  gtk_widget_destroy(d);
}

static void
click_calendar(GtkWidget *widget,
	       struct date *dp)
{
  struct tm tm;
  char buf[256];
  
  gtk_calendar_get_date (GTK_CALENDAR (widget), &dp->year, &dp->month, &dp->day);
  memset(&tm, 0, sizeof(tm));
  tm.tm_year = dp->year - 1900;
  tm.tm_mon = dp->month;
  tm.tm_mday = dp->day;
  strftime (buf, sizeof(buf), "%a, %d %b %Y", &tm);
  gtk_entry_set_text (GTK_ENTRY (dp->entry), buf);

  gtk_widget_hide (widget);
  gtk_widget_destroy (widget);
}

static void
drop_calendar(GtkWidget *widget,
	      GtkWidget *window)
{
  struct sens *s = gtk_object_get_data (GTK_OBJECT (window), "sens_list");

  struct date *dp = (widget == s->start.button) ? &s->start : &s->end;

  if (dp->cal)
    {
      gtk_widget_hide (dp->cal);
      gtk_widget_destroy (dp->cal);

      dp->cal = NULL;
    }
  else
    {
      GtkWidget *cal = gtk_calendar_new ();
      GtkWidget *calw = gtk_window_new (GTK_WINDOW_POPUP);
      GtkRequisition requisition;
      gint x, y;
      gint screen_width;
      gint screen_height;

      gtk_calendar_select_month (GTK_CALENDAR (cal), dp->month, dp->year);
      gtk_calendar_select_day (GTK_CALENDAR (cal), dp->day);
 
      gtk_container_add (GTK_CONTAINER (calw), cal);
      gtk_window_set_policy (GTK_WINDOW (calw),
			     FALSE, FALSE, TRUE);
      
      gdk_window_get_pointer (NULL, &x, &y, NULL);
      gtk_widget_size_request (cal, &requisition);
      
      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();
      
      x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
      y = CLAMP (y - 2, 0, MAX (0, screen_height - requisition.height));
      
      gtk_widget_set_uposition (calw, MAX (x, 0), MAX (y, 0));
      
      gtk_widget_show_all (calw);
      dp->cal = calw;
      gtk_signal_connect (GTK_OBJECT (cal), "day-selected-double-click",
			  GTK_SIGNAL_FUNC (click_calendar), dp);
    }
}

GtkWidget *
new_event(time_t t, guint timesel)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *text = gtk_text_new (NULL, NULL);

  GtkWidget *startdatebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *enddatebox = gtk_hbox_new (FALSE, 0);
  GtkWidget *startdatebutton = gtk_button_new ();
  GtkWidget *enddatebutton = gtk_button_new ();
  GtkWidget *startdateentry = gtk_entry_new ();
  GtkWidget *enddateentry = gtk_entry_new ();
  GtkWidget *startdatelabel = gtk_label_new("Start date");
  GtkWidget *enddatelabel = gtk_label_new("End date");

  GtkWidget *frombutton = gtk_radio_button_new_with_label (NULL, "From");
  GtkWidget *alldaybutton = 
    gtk_radio_button_new_with_label (gtk_radio_button_group 
				     (GTK_RADIO_BUTTON (frombutton)), 
				     "All day");
  GtkWidget *repeatbutton = gtk_check_button_new_with_label ("Repeat every");
  GtkWidget *repeathbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *timehbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *alldayhbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *timeend = gtk_label_new ("to");

  GtkWidget *repeatmenu = gtk_menu_new ();
  GtkWidget *repeatoption = gtk_option_menu_new ();
  GtkObject *repeatadj = gtk_adjustment_new (1.0, 1.0, 100.0, 1.0, 5.0, 5.0);
  GtkWidget *repeatspin = gtk_spin_button_new (GTK_ADJUSTMENT (repeatadj), 
					       1.0, 0);

  GtkWidget *buttonbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *buttonok = gtk_button_new_with_label ("OK");
  GtkWidget *buttoncancel = gtk_button_new_with_label ("Cancel");

  GtkWidget *startcombo = gtk_combo_new ();
  GtkWidget *endcombo = gtk_combo_new ();

  GtkWidget *alarmhbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *alarmmenu = gtk_menu_new ();
  GtkWidget *alarmbutton = gtk_check_button_new_with_label ("Alarm");
  GtkObject *alarmadj = gtk_adjustment_new (5.0, 1.0, 100.0, 1.0, 5.0, 5.0);
  GtkWidget *alarmspin = gtk_spin_button_new (GTK_ADJUSTMENT (alarmadj), 
					      1.0, 0);
  GtkWidget *alarmoption = gtk_option_menu_new ();

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  struct sens *s = g_malloc (sizeof (struct sens));
  struct tm tm;
  char buf[32];

  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("minutes before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("hours before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("days before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("weeks before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("months before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("years before"));
		
  gtk_menu_append (GTK_MENU (repeatmenu), gtk_menu_item_new_with_label ("hours"));
  gtk_menu_append (GTK_MENU (repeatmenu), gtk_menu_item_new_with_label ("days"));
  gtk_menu_append (GTK_MENU (repeatmenu), gtk_menu_item_new_with_label ("weeks"));
  gtk_menu_append (GTK_MENU (repeatmenu), gtk_menu_item_new_with_label ("months"));
  gtk_menu_append (GTK_MENU (repeatmenu), gtk_menu_item_new_with_label ("years"));

  gtk_option_menu_set_menu (GTK_OPTION_MENU (repeatoption), repeatmenu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (repeatoption), 1);
  gtk_widget_set_usize (repeatoption, 70, -1);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (alarmoption), alarmmenu);
  gtk_widget_set_usize (alarmoption, 120, -1);

  gtk_widget_set_usize (startcombo, 64, -1);
  gtk_widget_set_usize (endcombo, 64, -1);
  gtk_combo_set_popdown_strings (GTK_COMBO (startcombo), times);
  gtk_combo_set_popdown_strings (GTK_COMBO (endcombo), times);

  localtime_r (&t, &tm);
  strftime (buf, sizeof(buf), "%H:%M", &tm);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (startcombo)->entry), buf);
  tm.tm_hour++;
  strftime (buf, sizeof(buf), "%H:%M", &tm);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (endcombo)->entry), buf);
  strftime (buf, sizeof(buf), "%a, %d %b %Y", &tm);
  gtk_entry_set_text (GTK_ENTRY (startdateentry), buf);
  gtk_entry_set_text (GTK_ENTRY (enddateentry), buf);
  s->start.year = s->end.year = 1900 + tm.tm_year;
  s->start.month = s->end.month = tm.tm_mon;
  s->start.day = s->end.day = tm.tm_mday;

  gtk_container_add (GTK_CONTAINER (startdatebutton), 
		     gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT));
  gtk_container_add (GTK_CONTAINER (enddatebutton), 
		     gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (frombutton), timesel);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (alldaybutton), ! timesel);

  gtk_text_set_editable (GTK_TEXT(text), TRUE);
  gtk_text_set_word_wrap (GTK_TEXT(text), TRUE);
  gtk_widget_set_usize (text, -1, 88);

  gtk_widget_set_usize (buttonok, 60, -1);
  gtk_widget_set_usize (buttoncancel, 60, -1);

  gtk_signal_connect (GTK_OBJECT (frombutton), "clicked",
		     GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (repeatbutton), "clicked",
		     GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (alarmbutton), "clicked",
		     GTK_SIGNAL_FUNC (recalculate_sensitivities), window);

  gtk_signal_connect (GTK_OBJECT (startdatebutton), "clicked",
		      GTK_SIGNAL_FUNC (drop_calendar), window);
  gtk_signal_connect (GTK_OBJECT (enddatebutton), "clicked",
		      GTK_SIGNAL_FUNC (drop_calendar), window);
  gtk_entry_set_editable (GTK_ENTRY (startdateentry), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (enddateentry), FALSE);

  gtk_box_pack_start (GTK_BOX (startdatebox), startdatelabel, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (enddatebox), enddatelabel, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (startdatebox), startdateentry, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (enddatebox), enddateentry, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (startdatebox), startdatebutton, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (enddatebox), enddatebutton, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (alldayhbox), alldaybutton, FALSE, FALSE, 5);

  gtk_box_pack_start (GTK_BOX (timehbox), frombutton, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (timehbox), startcombo, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (timehbox), timeend, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (timehbox), endcombo, FALSE, FALSE, 5);

  gtk_box_pack_start (GTK_BOX (repeathbox), repeatbutton, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (repeathbox), repeatspin, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (repeathbox), repeatoption, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmbutton, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmspin, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmoption, TRUE, TRUE, 2);

  gtk_box_pack_end (GTK_BOX (buttonbox), buttoncancel, FALSE, FALSE, 4);
  gtk_box_pack_end (GTK_BOX (buttonbox), buttonok, FALSE, FALSE, 4);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);

  gtk_box_pack_start (GTK_BOX (vbox), startdatebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), enddatebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), alldayhbox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), timehbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), repeathbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), alarmhbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), text, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), buttonbox, FALSE, FALSE, 2);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  s->frombutton = frombutton;
  s->startentry = startcombo;
  s->timeend = timeend;
  s->endentry = endcombo;
  s->alarmbutton = alarmbutton;
  s->alarmspin = alarmspin;
  s->alarmoption = alarmoption;
  s->repeatbutton = repeatbutton;
  s->repeatspin = repeatspin;
  s->repeatoption = repeatoption;
  s->text = text;
  s->start.button = startdatebutton;
  s->end.button = enddatebutton;
  s->start.entry = startdateentry;
  s->end.entry = enddateentry;

  gtk_object_set_data_full (GTK_OBJECT (window), "sens_list", s, destroy_user_data);

  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);

  recalculate_sensitivities (NULL, window);

  return window;
}

