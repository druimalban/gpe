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
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include "cotidie.h"
#include "event.h"

void
event_add(event_t ev)
{
  printf("Add event, time %x/%x text %s\n", (int)ev->start, (int)ev->end, ev->text);
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

  GtkWidget *text;
  
  GtkWidget *radiobuttonforever, *radiobuttonendafter;
  GtkWidget *endspin, *endlabel;
  GtkWidget *radiobuttonnone, *radiobuttondaily, *radiobuttonweekly, 
    *radiobuttonmonthly, *radiobuttonyearly;

  GtkWidget *dailyvbox, *dailylabelevery, *dailyspin, *dailylabels;
  GtkWidget *monthlyvbox, *monthlylabelevery, *monthlyspin, *monthlylabels;
  GtkWidget *yearlyvbox, *yearlylabelevery, *yearlyspin, *yearlylabels;

  GtkWidget *weeklyhbox, *weeklyvbox, *weeklyvbox2, *checkbuttonsun, 
    *checkbuttonmon, *checkbuttontue, *checkbuttonwed,
    *checkbuttonthu, *checkbuttonfri, *checkbuttonsat;

  GtkAdjustment *endspin_adj, *dailyspin_adj, *monthlyspin_adj, *yearlyspin_adj;
  
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
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s->radiobuttonnone))) {
	gtk_widget_hide (s->dailyvbox);
  	gtk_widget_hide (s->weeklyhbox);
  	gtk_widget_hide (s->monthlyvbox);
  	gtk_widget_hide (s->yearlyvbox);
	gtk_widget_set_sensitive (s->endspin, 0);
	gtk_widget_set_sensitive (s->endlabel, 0);
	gtk_widget_set_sensitive (s->radiobuttonforever, 0);
	gtk_widget_set_sensitive (s->radiobuttonendafter, 0);
  }
  else {
	  
	gtk_widget_set_sensitive (s->radiobuttonforever, 1);
	gtk_widget_set_sensitive (s->radiobuttonendafter, 1);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s->radiobuttonforever))) {
	  gtk_widget_set_sensitive (s->endspin, 0);
	  gtk_widget_set_sensitive (s->endlabel, 0);
  	}
  	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s->radiobuttonendafter))) {
	  gtk_widget_set_sensitive (s->endspin, 1);
	  gtk_widget_set_sensitive (s->endlabel, 1);
  	}
  
  	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s->radiobuttondaily))) {
	  gtk_widget_show (s->dailyvbox);
	  gtk_widget_hide (s->weeklyhbox);
	  gtk_widget_hide (s->monthlyvbox);
	  gtk_widget_hide (s->yearlyvbox);
  	}
  	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s->radiobuttonweekly))) {
	  gtk_widget_hide (s->dailyvbox);
	  gtk_widget_show (s->weeklyhbox);
	  gtk_widget_hide (s->monthlyvbox);
	  gtk_widget_hide (s->yearlyvbox);
  	}
  	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s->radiobuttonmonthly))) {
	  gtk_widget_hide (s->dailyvbox);
	  gtk_widget_hide (s->weeklyhbox);
	  gtk_widget_show (s->monthlyvbox);
	  gtk_widget_hide (s->yearlyvbox);
  	}
  	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s->radiobuttonyearly))) {
	  gtk_widget_hide (s->dailyvbox);
	  gtk_widget_hide (s->weeklyhbox);
	  gtk_widget_hide (s->monthlyvbox);
	  gtk_widget_show (s->yearlyvbox);
  	}
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

  GSList *vboxrecur_group, *vboxend_group;
  GtkWidget *radiobuttonforever, *radiobuttonendafter;
  GtkWidget *endspin, *endlabel;
  GtkWidget *radiobuttonnone, *radiobuttondaily, *radiobuttonweekly, 
    *radiobuttonmonthly, *radiobuttonyearly;
  GtkWidget *recurborder, *recurendborder;
  GtkWidget *vboxrepeattop;

  GtkWidget *dailyvbox, *dailylabelevery, *dailyspin, *dailylabels;
  GtkWidget *monthlyvbox, *monthlylabelevery, *monthlyspin, *monthlylabels;
  GtkWidget *yearlyvbox, *yearlylabelevery, *yearlyspin, *yearlylabels;

  GtkWidget *weeklyhbox, *weeklyvbox, *weeklyvbox2, *checkbuttonsun, 
    *checkbuttonmon, *checkbuttontue, *checkbuttonwed,
    *checkbuttonthu, *checkbuttonfri, *checkbuttonsat;

  GtkAdjustment *endspin_adj, *dailyspin_adj, *monthlyspin_adj, *yearlyspin_adj;
   
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *labeleventpage = gtk_label_new ("Event");
  GtkWidget *labelrecurpage = gtk_label_new ("Recurrence");
  
  GtkWidget *vboxevent = gtk_vbox_new (FALSE, 0);
  GtkWidget *hboxrepeat = gtk_hbox_new (FALSE, 0);
  GtkWidget *vboxend = gtk_vbox_new (FALSE, 0);
  
  GtkWidget *vboxrecur = gtk_vbox_new (FALSE, 0);
  GtkWidget *hboxendafter = gtk_hbox_new (FALSE, 0);
  
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
  GtkWidget *timehbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *alldayhbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *timeend = gtk_label_new ("to");

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

/* Begin event vbox */

  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("minutes before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("hours before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("days before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("weeks before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("months before"));
  gtk_menu_append (GTK_MENU (alarmmenu), gtk_menu_item_new_with_label ("years before"));
		
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

  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmbutton, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmspin, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (alarmhbox), alarmoption, TRUE, TRUE, 2);

  gtk_box_pack_end (GTK_BOX (buttonbox), buttoncancel, FALSE, FALSE, 4);
  gtk_box_pack_end (GTK_BOX (buttonbox), buttonok, FALSE, FALSE, 4);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), window);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), window);

  gtk_box_pack_start (GTK_BOX (vboxevent), startdatebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxevent), enddatebox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxevent), alldayhbox, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vboxevent), timehbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxevent), alarmhbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxevent), text, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vboxevent), buttonbox, FALSE, FALSE, 2);
  
  gtk_widget_show_all (vboxevent);
  
/* Begin repeat vbox */

  gtk_widget_show (hboxrepeat);
  
  vboxrepeattop = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vboxrepeattop);
  
  recurborder = gtk_frame_new ("Type:");
  gtk_widget_show (recurborder);
  gtk_box_pack_start (GTK_BOX (vboxrepeattop), recurborder, FALSE, FALSE, 0); 
  gtk_container_add (GTK_CONTAINER (recurborder), hboxrepeat);
   
  vboxrecur = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vboxrecur);
  gtk_box_pack_start (GTK_BOX (hboxrepeat), vboxrecur, FALSE, FALSE, 0);

/* recurrence radio buttons */
  radiobuttonnone = gtk_radio_button_new_with_label (NULL, "None");
  gtk_widget_show (radiobuttonnone);
  gtk_box_pack_start (GTK_BOX (vboxrecur), radiobuttonnone, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonnone), TRUE);
  
  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonnone));
  radiobuttondaily = gtk_radio_button_new_with_label (vboxrecur_group, "Daily");
  gtk_widget_show (radiobuttondaily);
  gtk_box_pack_start (GTK_BOX (vboxrecur), radiobuttondaily, FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttondaily));
  radiobuttonweekly = gtk_radio_button_new_with_label (vboxrecur_group, "Weekly");
  gtk_widget_show (radiobuttonweekly);
  gtk_box_pack_start (GTK_BOX (vboxrecur), radiobuttonweekly, FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonweekly));
  radiobuttonmonthly = gtk_radio_button_new_with_label (vboxrecur_group, "Monthly");
  gtk_widget_show (radiobuttonmonthly);
  gtk_box_pack_start (GTK_BOX (vboxrecur), radiobuttonmonthly, FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonmonthly));
  radiobuttonyearly = gtk_radio_button_new_with_label (vboxrecur_group, "Yearly");
  gtk_widget_show (radiobuttonyearly);
  gtk_box_pack_start (GTK_BOX (vboxrecur), radiobuttonyearly, FALSE, FALSE, 0);

  vboxrecur_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonyearly));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonnone), TRUE);

  gtk_signal_connect (GTK_OBJECT (radiobuttonnone), "toggled", GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttondaily), "toggled", GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonweekly), "toggled", GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonmonthly), "toggled", GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonyearly), "toggled", GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  
/* daily vbox */
  dailyvbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (dailyvbox);
  gtk_box_pack_start (GTK_BOX (hboxrepeat), dailyvbox, FALSE, FALSE, 0);

  /* "every" label */
  dailylabelevery = gtk_label_new ("Every");
  gtk_widget_show (dailylabelevery);
  gtk_box_pack_start (GTK_BOX (dailyvbox), dailylabelevery, FALSE, FALSE, 0);
  
  /* daily spinner */
  dailyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  dailyspin = gtk_spin_button_new (GTK_ADJUSTMENT (dailyspin_adj), 1, 0);
  gtk_widget_set_usize(dailyspin,50, 20);
  gtk_widget_show (dailyspin);
  gtk_box_pack_start (GTK_BOX (dailyvbox), dailyspin, FALSE, FALSE, 0);

  /* days label */
  dailylabels = gtk_label_new ("day(s)");
  gtk_widget_show (dailylabels);
  gtk_box_pack_start (GTK_BOX (dailyvbox), dailylabels, FALSE, FALSE, 0);
  
/* weekly hbox */
  weeklyhbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (weeklyhbox);
  gtk_box_pack_start (GTK_BOX (hboxrepeat), weeklyhbox, FALSE, FALSE, 0);

  /* weekly vbox */
  weeklyvbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (weeklyvbox);
  gtk_box_pack_start (GTK_BOX (weeklyhbox), weeklyvbox, FALSE, FALSE, 0);

  checkbuttonmon = gtk_check_button_new_with_label ("Mon");
  gtk_widget_show (checkbuttonmon);
  gtk_box_pack_start (GTK_BOX (weeklyvbox), checkbuttonmon, FALSE, FALSE, 0);

  checkbuttontue = gtk_check_button_new_with_label ("Tue");
  gtk_widget_show (checkbuttontue);
  gtk_box_pack_start (GTK_BOX (weeklyvbox), checkbuttontue, FALSE, FALSE, 0);

  checkbuttonwed = gtk_check_button_new_with_label ("Wed");
  gtk_widget_show (checkbuttonwed);
  gtk_box_pack_start (GTK_BOX (weeklyvbox), checkbuttonwed, FALSE, FALSE, 0);

  checkbuttonthu = gtk_check_button_new_with_label ("Thu");
  gtk_widget_show (checkbuttonthu);
  gtk_box_pack_start (GTK_BOX (weeklyvbox), checkbuttonthu, FALSE, FALSE, 0);

  /* weekly vbox2 */
  weeklyvbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (weeklyvbox2);
  gtk_box_pack_start (GTK_BOX (weeklyhbox), weeklyvbox2, FALSE, FALSE, 0);

  checkbuttonfri = gtk_check_button_new_with_label ("Fri");
  gtk_widget_show (checkbuttonfri);
  gtk_box_pack_start (GTK_BOX (weeklyvbox2), checkbuttonfri, FALSE, FALSE, 0);

  checkbuttonsat = gtk_check_button_new_with_label ("Sat");
  gtk_widget_show (checkbuttonsat);
  gtk_box_pack_start (GTK_BOX (weeklyvbox2), checkbuttonsat, FALSE, FALSE, 0);

  checkbuttonsun = gtk_check_button_new_with_label ("Sun");
  gtk_widget_show (checkbuttonsun);
  gtk_box_pack_start (GTK_BOX (weeklyvbox2), checkbuttonsun, FALSE, FALSE, 0);

/* monthly vbox */
  monthlyvbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (monthlyvbox);
  gtk_box_pack_start (GTK_BOX (hboxrepeat), monthlyvbox, FALSE, FALSE, 0);

  /* "every" label */
  monthlylabelevery = gtk_label_new ("Every");
  gtk_widget_show (monthlylabelevery);
  gtk_box_pack_start (GTK_BOX (monthlyvbox), monthlylabelevery, FALSE, FALSE, 0);
  
  /* monthly spinner */
  monthlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  monthlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (monthlyspin_adj), 1, 0);
  gtk_widget_set_usize(monthlyspin,50, 20);
  gtk_widget_show (monthlyspin);
  gtk_box_pack_start (GTK_BOX (monthlyvbox), monthlyspin, FALSE, FALSE, 0);

  /* months label */
  monthlylabels = gtk_label_new ("month(s)");
  gtk_widget_show (monthlylabels);
  gtk_box_pack_start (GTK_BOX (monthlyvbox), monthlylabels, FALSE, FALSE, 0);
  
/* yearly hbox */
  yearlyvbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (yearlyvbox);
  gtk_box_pack_start (GTK_BOX (hboxrepeat), yearlyvbox, FALSE, FALSE, 0);

  /* "every" label */
  yearlylabelevery = gtk_label_new ("Every");
  gtk_widget_show (yearlylabelevery);
  gtk_box_pack_start (GTK_BOX (yearlyvbox), yearlylabelevery, FALSE, FALSE, 0);
  
  /* yearly spinner */
  yearlyspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  yearlyspin = gtk_spin_button_new (GTK_ADJUSTMENT (yearlyspin_adj), 1, 0);
  gtk_widget_set_usize(yearlyspin,50, 20);
  gtk_widget_show (yearlyspin);
  gtk_box_pack_start (GTK_BOX (yearlyvbox), yearlyspin, FALSE, FALSE, 0);

  /* years label */
  yearlylabels = gtk_label_new ("year(s)");
  gtk_widget_show (yearlylabels);
  gtk_box_pack_start (GTK_BOX (yearlyvbox), yearlylabels, FALSE, FALSE, 0);
  
  gtk_widget_hide (dailyvbox);
  gtk_widget_hide (weeklyhbox);
  gtk_widget_hide (monthlyvbox);
  gtk_widget_hide (yearlyvbox);

/* Begin end-date vbox */
 
  gtk_widget_show (vboxend);

  recurendborder = gtk_frame_new ("End Date:");
  gtk_widget_show (recurendborder);
  gtk_container_add (GTK_CONTAINER (recurendborder), vboxend);
  gtk_box_pack_start (GTK_BOX (vboxrepeattop), recurendborder, FALSE, FALSE, 0);
 
  /* forever radio button */
  radiobuttonforever = gtk_radio_button_new_with_label (NULL, "forever");
  gtk_widget_show (radiobuttonforever );
  gtk_box_pack_start (GTK_BOX (vboxend), radiobuttonforever , FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonforever , FALSE);

  /* endafter hbox */
  hboxendafter = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (hboxendafter);
  gtk_box_pack_start (GTK_BOX (vboxend), hboxendafter, FALSE, FALSE, 0);

  /* end after radio button */
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonforever));
  radiobuttonendafter = gtk_radio_button_new_with_label (vboxend_group, "end after");
  gtk_widget_show (radiobuttonendafter);
  gtk_box_pack_start (GTK_BOX (hboxendafter), radiobuttonendafter, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (radiobuttonendafter, FALSE);
  vboxend_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobuttonendafter));
  		  
  /* end after spinner */
  endspin_adj = (GtkAdjustment *) gtk_adjustment_new (1, 1, 365, 1, 5, 5);
  endspin = gtk_spin_button_new (GTK_ADJUSTMENT (endspin_adj), 1, 0);
  gtk_widget_set_usize(endspin,50, 20);
  gtk_widget_show (endspin);
  gtk_box_pack_start (GTK_BOX (hboxendafter), endspin, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endspin, FALSE);

  /* events label */
  endlabel= gtk_label_new ("event(s)");
  gtk_widget_show (endlabel);
  gtk_box_pack_start (GTK_BOX (hboxendafter), endlabel, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (endlabel, FALSE);

  gtk_signal_connect (GTK_OBJECT (radiobuttonforever), "toggled", GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  gtk_signal_connect (GTK_OBJECT (radiobuttonendafter), "toggled", GTK_SIGNAL_FUNC (recalculate_sensitivities), window);
  		  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobuttonforever), TRUE);

  gtk_widget_show (labeleventpage);
  gtk_widget_show (labelrecurpage);
  
  gtk_widget_show(notebook);
  
  gtk_container_add (GTK_CONTAINER (window), notebook);
  
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vboxevent, labeleventpage);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vboxrepeattop, labelrecurpage);
  
  s->frombutton = frombutton;
  s->startentry = startcombo;
  s->timeend = timeend;
  s->endentry = endcombo;
  s->alarmbutton = alarmbutton;
  s->alarmspin = alarmspin;
  s->alarmoption = alarmoption;
  s->text = text;
  s->start.button = startdatebutton;
  s->end.button = enddatebutton;
  s->start.entry = startdateentry;
  s->end.entry = enddateentry;
  s->radiobuttonforever = radiobuttonforever;
  s->radiobuttonendafter = radiobuttonendafter;
  s->endspin = endspin;
  s->endlabel = endlabel;
  s->radiobuttonnone = radiobuttonnone;
  s->radiobuttondaily = radiobuttondaily;
  s->radiobuttonweekly = radiobuttonweekly;
  s->radiobuttonmonthly = radiobuttonmonthly;
  s->radiobuttonyearly = radiobuttonyearly;
  s->dailyvbox = dailyvbox;
  s->dailylabelevery = dailylabelevery;
  s->dailyspin = dailyspin;
  s->dailylabels = dailylabels;
  s->monthlyvbox = monthlyvbox;
  s->monthlylabelevery = monthlylabelevery;
  s->monthlyspin = monthlyspin;
  s->monthlylabels = monthlylabels;
  s->yearlyvbox = yearlyvbox;
  s->yearlylabelevery = yearlylabelevery;
  s->yearlyspin = yearlyspin;
  s->yearlylabels = yearlylabels;
  s->weeklyhbox = weeklyhbox;
  s->weeklyvbox = weeklyvbox;
  s->weeklyvbox2 = weeklyvbox2;
  s->checkbuttonsun = checkbuttonsun;
  s->checkbuttonmon = checkbuttonmon;
  s->checkbuttontue = checkbuttontue;
  s->checkbuttonwed = checkbuttonwed;
  s->checkbuttonthu = checkbuttonthu;
  s->checkbuttonfri = checkbuttonfri;
  s->checkbuttonsat = checkbuttonsat;
  s->endspin_adj = endspin_adj;
  s->dailyspin_adj = dailyspin_adj;
  s->monthlyspin_adj = monthlyspin_adj;
  s->yearlyspin_adj = yearlyspin_adj;
   
  gtk_object_set_data_full (GTK_OBJECT (window), "sens_list", s, destroy_user_data);

  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);

  recalculate_sensitivities (NULL, window);
  
  return window;
}

