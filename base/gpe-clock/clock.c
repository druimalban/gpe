/*
 * Copyright (C) 2003, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
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
#include <string.h>
#include <libintl.h>
#include <langinfo.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/tray.h>
#include <gpe/init.h>
#include <gpe/popup.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/gpetimesel.h>
#include <gpe/gpeclockface.h>
#include <gpe/schedule.h>
#include <gpe/launch.h>
#include <gpe/infoprint.h>

Display *dpy;

GtkWidget *panel_window, *time_label, *face, *menu;
GtkObject *hour_adj, *minute_adj;

#define _(x) gettext(x)

static gchar *alarm_file, *prefs_file;

#define FORMAT_NONE		-1
#define FORMAT_12	        0
#define FORMAT_24		1
#define FORMAT_ANALOGUE		2

static gboolean show_seconds;
static int format = FORMAT_NONE;

gboolean prefs_window_open;
gboolean alarm_window_open;
gboolean auto_format;

#define SHORT_TERM_ALARMS

struct alarm_state
{
  gboolean active;

  guint hour, minute;

  gboolean date_flag;
  int day, month, year;
  guint day_mask;
};

struct alarm_context
{
  struct alarm_state state;

  GtkWidget *window;
  GtkWidget *active_button;
  GtkWidget *date_combo, *time_sel;
  GtkWidget *date_button, *days_button;
  GtkWidget *day_button[7];
};

static struct alarm_state state;

void select_new_format (int new_format);

static void
set_defaults (void)
{
  time_t t;
  struct tm tm;

  memset (&state, 0, sizeof (state));

  time (&t);
  localtime_r (&t, &tm);

  state.hour = tm.tm_hour;
  state.minute = tm.tm_min;
  state.date_flag = TRUE;
  state.year = tm.tm_year + 1900;
  state.month = tm.tm_mon;
  state.day = tm.tm_mday;
}

static void
flush_prefs (void)
{
  FILE *fp;

  fp = fopen (prefs_file, "w");
  
  if (fp)
    {
      fprintf (fp, "%d %d\n", show_seconds, format);
      fclose (fp);
    }
}

static void
load_prefs (void)
{
 FILE *fp;

  fp = fopen (prefs_file, "r");
  
  if (fp)
    {
      int seconds;

      fscanf (fp, "%d %d", &seconds, &format);
      show_seconds = (seconds != 0) ? TRUE : FALSE;
      fclose (fp);
    }
}

static void
flush_alarm_details (void)
{
  FILE *fp;

  fp = fopen (alarm_file, "w");

  if (fp)
    {
      fprintf (fp, "ACTIVE: %d\n", state.active);
      fprintf (fp, "TIME: %02d:%02d\n", state.hour, state.minute);
      if (state.date_flag)
	fprintf (fp, "DATE: %04d-%02d-%02d\n", state.year, state.month, state.day);
      else
	fprintf (fp, "DAYS: %02x\n", state.day_mask);
      fclose (fp);
    }
}

static gboolean
time_past_already (struct alarm_state *state, struct tm *tm, gboolean use_date)
{
  if (use_date)
    {
      if (state->year < (tm->tm_year + 1900))
	return TRUE;
      if (state->year > (tm->tm_year + 1900))
	return FALSE;

      if (state->month < tm->tm_mon)
	return TRUE;
      if (state->month > tm->tm_mon)
	return FALSE;

      if (state->day < tm->tm_mday)
	return TRUE;
      if (state->day > tm->tm_mday)
	return FALSE;
    }

  if (state->hour < tm->tm_hour)
    return TRUE;
  if (state->hour > tm->tm_hour)
    return FALSE;

  if (state->minute < tm->tm_min)
    return TRUE;
  /* if (state->minute > tm->tm_min) return FALSE; */

  return FALSE;
}

static void
load_alarm_details (void)
{
  FILE *fp;

  fp = fopen (alarm_file, "r");

  if (fp)
    {
      while (!feof (fp))
	{
	  char buf[128];

	  if (fgets (buf, 128, fp))
	    {
	      char *p = strchr (buf, ':');
	      if (p)
		{
		  *p++ = 0;

		  while (isspace (*p))
		    p++;
 
		  if (!strcasecmp (buf, "ACTIVE"))
		    {
		      state.active = atoi (p);
		    }
		  else if (!strcasecmp (buf, "TIME"))
		    {
		      sscanf (p, "%02d:%02d", &state.hour, &state.minute);
		    }
		  else if (!strcasecmp (buf, "DATE"))
		    {
		      state.date_flag = TRUE;
		      sscanf (p, "%04d-%02d-%02d", &state.year, &state.month, &state.day);
		    }
		  else if (!strcasecmp (buf, "DAYS"))
		    {
		      state.date_flag = FALSE;
		      sscanf (p, "%02x", &state.day_mask);
		    }
		}
	    }
	}

      fclose (fp);
    }

  if (state.date_flag)
    {
      time_t now;
      struct tm tm;
      time (&now);
      localtime_r (&now, &tm);

#ifdef SHORT_TERM_ALARMS
      state.hour = tm.tm_hour;
      state.minute = tm.tm_min;
#endif

      if (time_past_already (&state, &tm, TRUE))
	{
	  state.active = FALSE;
	  
	  if (time_past_already (&state, &tm, FALSE))
	    {
	      /* Alarm time already passed for today.  Set it for
		 tomorrow instead.  */
	      now += 24 * 60 * 60;
	      localtime_r (&now, &tm);
	    }

	  state.day = tm.tm_mday;
	  state.month = tm.tm_mon;
	  state.year = tm.tm_year + 1900;
	}
    }
}

static time_t
extract_time (struct alarm_state *alarm)
{
  struct tm tm;
  time_t t;
  int wasdst;

  memset (&tm, 0, sizeof (tm));

  tm.tm_year = alarm->year - 1900;
  tm.tm_mon = alarm->month;
  tm.tm_mday = alarm->day;
  tm.tm_hour = alarm->hour;
  tm.tm_min = alarm->minute;

  t = mktime (&tm);

  wasdst = tm.tm_isdst;

  /* Break it out again to get DST right */
  memset (&tm, 0, sizeof (tm));

  localtime_r (&t, &tm);

  tm.tm_year = alarm->year - 1900;
  tm.tm_mon = alarm->month;
  tm.tm_mday = alarm->day;
  tm.tm_hour = alarm->hour;
  tm.tm_min = alarm->minute;
  tm.tm_isdst = wasdst;

  return mktime (&tm);
}

static void
cancel_alarm (struct alarm_state *alarm)
{
  if (alarm->active)
    {
      time_t t;

      t = extract_time (alarm);

      schedule_cancel_alarm (1, t);
    }
}

static gboolean
day_matches (struct alarm_state *alarm, int day)
{
  day = (day + 6) % 7;

  return (alarm->day_mask & (1 << day)) ? TRUE : FALSE;
}

static void
set_alarm (struct alarm_state *alarm)
{
  if (alarm->active)
    {
      time_t t;

      if (alarm->date_flag)
	{
	  t = extract_time (alarm);
	}
      else
	{
	  struct tm tm;
	  time_t now;

	  if (alarm->day_mask == 0)
	    return;

	  time (&now);
	  localtime_r (&now, &tm);
	  
	  tm.tm_hour = alarm->hour;
	  tm.tm_min = alarm->minute;

	  t = mktime (&tm);

	  while (t < now || !day_matches (alarm, tm.tm_mday))
	    {
	      t += 24 * 60 * 60;
	      localtime_r (&t, &tm);
	    }

	  alarm->year = tm.tm_year + 1900;
	  alarm->month = tm.tm_mon;
	  alarm->day = tm.tm_mday;
	}

      if (schedule_set_alarm (1, t, "gpe-announce\ngpe-clock --check-alarm\n", FALSE) == FALSE)
	{
	  gpe_error_box (_("Unable to set alarm"));
	  alarm->active = FALSE;
	}
    }
}

static void
update_time_label (GtkWidget *label)
{
  char buf[256];
  static time_t t_stat = 0;
  time_t t;
  struct tm tm;
  const char *time_format;
  
  time (&t);
  /* Check if we need to update, if not save power.
   * Supress update if no seconds to show, 
   * set if time set back, 60 sec no update or new minute started. */
  if (!show_seconds && t_stat && (t_stat > t) && (((t - t_stat) < 60) || (t % 60)))
    return;
      
  t_stat = t;
  
  localtime_r (&t, &tm);

  if (format == FORMAT_24)
    {
      time_format = show_seconds ? "<span font_desc=\"Sans\"><b>%H:%M:%S</b></span>": "<span font_desc=\"Sans\"><b>%H:%M</b></span>";
    }
  else
    {
      time_format = show_seconds ? "<span font_desc=\"Sans\"><b>%I:%M:%S%p</b></span>" : "<span font_desc=\"Sans\"><b>%I:%M%p</b></span>";
    }
    
  strftime (buf, sizeof (buf), time_format, &tm);
  gtk_label_set_markup (GTK_LABEL (label), buf);
}

static void
update_time_face (void)
{
  static time_t t_stat = 0;
  time_t t;
  struct tm tm;
  
  time (&t);
  /* Check if we need to update, if not save power.
   * Supress update if no seconds to show, 
   * set if time set back, 60 sec no update or new minute started. */
  if (!show_seconds && t_stat && (t_stat > t) && (((t - t_stat) < 60) || (t % 60)))
    return;
      
  t_stat = t;

  localtime_r (&t, &tm);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (hour_adj), tm.tm_hour);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (minute_adj), tm.tm_min);
}

static gboolean
update_time (void)
{
  if (format == FORMAT_ANALOGUE)
    update_time_face ();
  else
    update_time_label (time_label);

  return TRUE;
}

#ifdef DO_SECONDS
static void
set_seconds (GtkWidget *w, GtkWidget *time_label)
{
  show_seconds = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
  update_time ();
}
#endif

static void
set_format (GtkWidget *w, void *value)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    {
      select_new_format ((int)value);
      update_time ();
    }
}

static void
click_ok (GtkWidget *w, GtkWidget *window)
{
  flush_prefs ();
  gtk_widget_destroy (window);
  auto_format = FALSE;
}

static void
note_closed (GtkWidget *widget, gboolean *false)
{
  *false = FALSE;
}

static void
prefs_window (GtkWidget *w, GtkWidget *time_label)
{
  GtkWidget *window;
  GtkWidget *ok_button;
  GtkWidget *format_12_button;
  GtkWidget *format_24_button;
  GtkWidget *format_analogue_button;
#ifdef DO_SECONDS
  GtkWidget *seconds_button;
#endif
  GSList    *radiogroup;
  int spacing = gpe_get_boxspacing ();

  if (prefs_window_open)
    return;

  prefs_window_open = TRUE;

  window = gtk_dialog_new ();

  gtk_window_set_default_size (GTK_WINDOW (window), 200, 150);
  
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (note_closed), &prefs_window_open);

  ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);

  gtk_window_set_title (GTK_WINDOW (window), _("Clock preferences"));

  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (window)->vbox), spacing);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (window)->vbox), gpe_get_border ());

  format_12_button = gtk_radio_button_new_with_label (NULL, _("12-hour format"));
  radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (format_12_button));
  format_24_button = gtk_radio_button_new_with_label (radiogroup, _("24-hour format"));
  radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (format_12_button));
  format_analogue_button = gtk_radio_button_new_with_label (radiogroup, _("Analogue format"));

#ifdef DO_SECONDS
  seconds_button = gtk_check_button_new_with_label (_("Show seconds"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (seconds_button), show_seconds);
#endif

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (format_24_button), format == FORMAT_24);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (format_analogue_button), format == FORMAT_ANALOGUE);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), format_analogue_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), format_12_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), format_24_button, FALSE, FALSE, 0);
#ifdef DO_SECONDS
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), seconds_button, FALSE, FALSE, 0);
#endif

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_widget_show_all (window);

#ifdef DO_SECONDS
  g_signal_connect (G_OBJECT (seconds_button), "toggled", G_CALLBACK (set_seconds), time_label);
#endif
  g_signal_connect (G_OBJECT (format_24_button), "toggled", G_CALLBACK (set_format), (void *)FORMAT_24);
  g_signal_connect (G_OBJECT (format_12_button), "toggled", G_CALLBACK (set_format), (void *)FORMAT_12);
  g_signal_connect (G_OBJECT (format_analogue_button), "toggled", G_CALLBACK (set_format), (void *)FORMAT_ANALOGUE);
 
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);
  g_signal_connect (G_OBJECT (ok_button), "clicked", G_CALLBACK (click_ok), window);
}

static void
set_time_window (void)
{
  static const gchar *cmd = "gpe-conf time";
  Window w;

  w = gpe_launch_get_window_for_binary (dpy, cmd);
  if (w)
    {
      gpe_launch_activate_window (dpy, w);
    }
  else if (! gpe_launch_startup_is_pending (dpy, cmd))
    {
      gpe_popup_infoprint (dpy, "Starting Time Setup");
      gpe_launch_program_with_callback (dpy, cmd, cmd, TRUE, NULL, NULL);
    }
}

static void
button_toggled (GtkWidget *w, struct alarm_context *ctx)
{
  int i;
  gboolean days_selected, active;
  
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ctx->active_button));
  days_selected = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ctx->days_button));
 
  gtk_widget_set_sensitive (ctx->time_sel, active);
  gtk_widget_set_sensitive (ctx->date_combo, active);
  gtk_widget_set_sensitive (ctx->date_button, active);
  gtk_widget_set_sensitive (ctx->days_button, active);

  for (i = 0; i < 7; i++)
    gtk_widget_set_sensitive (ctx->day_button[i], active && days_selected);
}

static void
free_alarm_context (struct alarm_context *ctx)
{
  gtk_widget_destroy (ctx->window);
  
  g_free (ctx);
}

static void
do_set_alarm (struct alarm_context *ctx)
{
  gboolean days_selected, active;
  
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ctx->active_button));
  days_selected = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ctx->days_button));

  cancel_alarm (&state);

  state.active = active;
  state.date_flag = !days_selected;

  gpe_time_sel_get_time (GPE_TIME_SEL (ctx->time_sel), &state.hour, &state.minute);

  if (days_selected)
    {
      int i;

      state.day_mask = 0;

      for (i = 0; i < 7; i++)
	{
	  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ctx->day_button[i])))
	    state.day_mask |= (1 << i);
	}
    }
  else
    {
      state.year = GTK_DATE_COMBO (ctx->date_combo)->year;
      state.month = GTK_DATE_COMBO (ctx->date_combo)->month;
      state.day = GTK_DATE_COMBO (ctx->date_combo)->day;
    }

  set_alarm (&state);

  flush_alarm_details ();

  free_alarm_context (ctx);
}

static const nl_item days[] = { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5,
				ABDAY_6, ABDAY_7, ABDAY_1 };

static void
alarm_window (void)
{
  GtkWidget *ok_button, *cancel_button;
  GtkWidget *time_label;
  GtkWidget *date_hbox;
  GtkWidget *time_hbox;
  GtkWidget *weeklytable;
  GtkWidget *scrolled_window;
  GtkWidget *scrolled_vbox;
  int spacing;
  GSList *radiogroup;
  int i;
  struct alarm_context *ctx;

  if (alarm_window_open)
    return;

  alarm_window_open = TRUE;

  time_label = gtk_label_new (_("Time:"));

  spacing = gpe_get_boxspacing ();
  ctx = g_malloc0 (sizeof (struct alarm_context));

  ctx->window = gtk_dialog_new ();
  g_signal_connect (G_OBJECT (ctx->window), "destroy", G_CALLBACK (note_closed), &alarm_window_open);

  gtk_window_set_title (GTK_WINDOW (ctx->window), _("Set alarm"));
  gtk_container_set_border_width (GTK_CONTAINER (ctx->window), gpe_get_border ());

  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (ctx->window)->vbox), spacing);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (ctx->window)->vbox), gpe_get_border ());

  ctx->active_button = gtk_check_button_new_with_label (_("Alarm active"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ctx->window)->vbox), ctx->active_button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (ctx->active_button), "toggled", G_CALLBACK (button_toggled), ctx);

  scrolled_vbox = gtk_vbox_new (FALSE, 0);
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_NONE);

#if 0
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ctx->window)->vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), scrolled_vbox);
#else
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ctx->window)->vbox), scrolled_vbox, TRUE, TRUE, 0);
#endif

  ctx->date_button = gtk_radio_button_new_with_label (NULL, _("Date:"));
  radiogroup = gtk_radio_button_group (GTK_RADIO_BUTTON (ctx->date_button));
  ctx->days_button = gtk_radio_button_new_with_label (radiogroup, _("Weekly, on:"));
  g_signal_connect (G_OBJECT (ctx->days_button), "toggled", G_CALLBACK (button_toggled), ctx);

  date_hbox = gtk_hbox_new (FALSE, spacing);
  ctx->date_combo = gtk_date_combo_new ();
  gtk_box_pack_start (GTK_BOX (date_hbox), ctx->date_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (date_hbox), ctx->date_combo, TRUE, TRUE, 0);

  time_hbox = gtk_hbox_new (FALSE, spacing);
  ctx->time_sel = gpe_time_sel_new ();
  gtk_box_pack_start (GTK_BOX (time_hbox), time_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (time_hbox), ctx->time_sel, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (scrolled_vbox), time_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (scrolled_vbox), date_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (scrolled_vbox), ctx->days_button, FALSE, FALSE, 0);

  weeklytable = gtk_table_new (2, 4, FALSE);
  for (i = 0; i < 4; i++)
    {
      GtkWidget *b = gtk_check_button_new_with_label (nl_langinfo (days[i]));
      gtk_table_attach_defaults (GTK_TABLE (weeklytable), b, i, i + 1, 0, 1);
      ctx->day_button[i] = b;
    }
  for (i = 4; i < 7; i++)
    {
      GtkWidget *b = gtk_check_button_new_with_label (nl_langinfo (days[i]));
      gtk_table_attach_defaults (GTK_TABLE (weeklytable), b, i - 4, i - 3, 1, 2);
      ctx->day_button[i] = b;
    }
  
  gtk_box_pack_start (GTK_BOX (scrolled_vbox), weeklytable, FALSE, FALSE, 0);

  load_alarm_details ();

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctx->active_button), state.active);
  gtk_date_combo_set_date (GTK_DATE_COMBO (ctx->date_combo), state.year, state.month, state.day);
  gpe_time_sel_set_time (GPE_TIME_SEL (ctx->time_sel), state.hour, state.minute);
  if (state.date_flag)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctx->date_button), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctx->days_button), TRUE);

  for (i = 0; i < 7; i++)
    {
      if (state.day_mask & (1 << i))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctx->day_button[i]), TRUE);
    }

  ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ctx->window)->action_area), cancel_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ctx->window)->action_area), ok_button, FALSE, FALSE, 0);

  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);

  gtk_widget_show_all (ctx->window);

  g_signal_connect_swapped (G_OBJECT (ok_button), "clicked", G_CALLBACK (do_set_alarm), ctx);
  g_signal_connect_swapped (G_OBJECT (cancel_button), "clicked", G_CALLBACK (free_alarm_context), ctx);

  button_toggled (NULL, ctx);
}

static gboolean
clicked (GtkWidget *w, GdkEventButton *ev, GtkWidget *menu)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gpe_popup_menu_position, w, ev->button, ev->time);

  return TRUE;
}

static void
do_check_alarm (void)
{
  if (state.date_flag == FALSE)
    set_alarm (&state);
  else
    state.active = FALSE;

  flush_alarm_details ();
}

void
select_new_format (int new_format)
{
  if (format == FORMAT_ANALOGUE)
    {
      gtk_widget_destroy (face);
      gtk_object_destroy (hour_adj);
      gtk_object_destroy (minute_adj);
    }
  else if (format == FORMAT_24 || format == FORMAT_12)
    gtk_widget_destroy (time_label);

  format = new_format;
  
  if (format == FORMAT_ANALOGUE)
    {
      hour_adj = gtk_adjustment_new (0, 0, 23, 1, 15, 15);
      minute_adj = gtk_adjustment_new (0, 0, 59, 1, 15, 15);
      
      face = gpe_clock_face_new (GTK_ADJUSTMENT (hour_adj), GTK_ADJUSTMENT (minute_adj), NULL);
      gpe_clock_face_set_do_grabs (GPE_CLOCK_FACE (face), FALSE);
      gpe_clock_face_set_label_hours (GPE_CLOCK_FACE (face), FALSE);
      gpe_clock_face_set_hand_width (GPE_CLOCK_FACE (face), 1.0);
      gtk_widget_show (face);
      gtk_container_add (GTK_CONTAINER (panel_window), face);
      gdk_window_resize (face->window, face->allocation.width, face->allocation.height);
      
      g_signal_connect (G_OBJECT (face), "button-press-event", G_CALLBACK (clicked), menu);
    }
  else
    {
      time_label = gtk_label_new (NULL);
      gtk_widget_set_name (time_label, "time_label");
      
      gtk_container_add (GTK_CONTAINER (panel_window), time_label);
      gtk_widget_show (time_label);
	    
      g_signal_connect (G_OBJECT (panel_window), "button-press-event", G_CALLBACK (clicked), menu);
      gtk_widget_add_events (panel_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    }
					       
  gtk_widget_queue_resize (panel_window);
}

/* handle resizing */
gboolean 
external_event (GtkWindow *window, GdkEventConfigure *event, gpointer user_data)
{
  if (auto_format)
    {
      if (event->height >= 32 && format != FORMAT_ANALOGUE)
	select_new_format (FORMAT_ANALOGUE);
      else if (event->height < 32 && format != FORMAT_24)
	select_new_format (FORMAT_24);
    }

  update_time();
    
  return FALSE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *menu_set_time, *menu_alarm, *menu_prefs;
  GtkTooltips *tooltips;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);
  
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  while (1)
    {
      int option_index = 0;
      int c;

      static struct option long_options[] = {
	{"graphical", 0, 0, 'g'},
	{0, 0, 0, 0}
      };
      
      c = getopt_long (argc, argv, "d",
		       long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 'g':
	  format = FORMAT_ANALOGUE;
	  break;
	}
    }

  alarm_file = g_strdup_printf ("%s/.gpe/alarm", g_get_home_dir ());
  prefs_file = g_strdup_printf ("%s/.gpe/clock_prefs", g_get_home_dir ());

  if (argc > 1 && !strcmp (argv[1], "--check-alarm"))
    {
      load_alarm_details ();

      do_check_alarm ();

      exit (0);
    }

  set_defaults ();
  load_prefs ();

  if (format == FORMAT_NONE)
    auto_format = TRUE;

  menu = gtk_menu_new ();
  menu_prefs = gtk_menu_item_new_with_label (_("Preferences"));
  menu_set_time = gtk_menu_item_new_with_label (_("Set the time"));
  menu_alarm = gtk_menu_item_new_with_label (_("Alarm ..."));

  gtk_widget_show (menu_prefs);
  gtk_widget_show (menu_set_time);
  gtk_widget_show (menu_alarm);

  gtk_menu_append (GTK_MENU (menu), menu_alarm);
  gtk_menu_append (GTK_MENU (menu), menu_set_time);
  gtk_menu_append (GTK_MENU (menu), menu_prefs);

  g_signal_connect (G_OBJECT (menu_prefs), "activate", G_CALLBACK (prefs_window), time_label);
  g_signal_connect (G_OBJECT (menu_set_time), "activate", G_CALLBACK (set_time_window), NULL);
  g_signal_connect (G_OBJECT (menu_alarm), "activate", G_CALLBACK (alarm_window), NULL);

  panel_window = gtk_plug_new (0);
  gtk_widget_realize (panel_window);
  gtk_window_set_title (GTK_WINDOW (panel_window), _("Alarm clock"));
  gtk_widget_set_name (panel_window, "gpe-clock");
  gtk_window_set_icon_from_file (GTK_WINDOW (panel_window), PREFIX "/share/pixmaps/gpe-clock.png", NULL);

  if (format == FORMAT_ANALOGUE)
    {
      hour_adj = gtk_adjustment_new (0, 0, 23, 1, 15, 15);
      minute_adj = gtk_adjustment_new (0, 0, 59, 1, 15, 15);
  
      face = gpe_clock_face_new (GTK_ADJUSTMENT (hour_adj), GTK_ADJUSTMENT (minute_adj), NULL);
      gpe_clock_face_set_do_grabs (GPE_CLOCK_FACE (face), FALSE);
      gpe_clock_face_set_label_hours (GPE_CLOCK_FACE (face), FALSE);
      gpe_clock_face_set_hand_width (GPE_CLOCK_FACE (face), 1.0);

      gtk_widget_show (face);
      
      gtk_container_add (GTK_CONTAINER (panel_window), face);

      g_signal_connect (G_OBJECT (face), "button-press-event", G_CALLBACK (clicked), menu);
    }
  else if (format == FORMAT_24 || format == FORMAT_12)
    {
      time_label = gtk_label_new (NULL);
      gtk_widget_set_name (time_label, "time_label");

      update_time_label (time_label);

      gtk_container_add (GTK_CONTAINER (panel_window), time_label);

      gtk_widget_show (time_label);

      g_signal_connect (G_OBJECT (panel_window), "button-press-event", G_CALLBACK (clicked), menu);
      gtk_widget_add_events (panel_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    }

  gtk_widget_show (panel_window);
  g_signal_connect (G_OBJECT (panel_window), "configure-event", G_CALLBACK (external_event), NULL);

  gdk_window_resize (panel_window->window, panel_window->allocation.width, panel_window->allocation.height);
  gtk_window_resize (GTK_WINDOW (panel_window), panel_window->allocation.width, panel_window->allocation.height);
  if (format == FORMAT_ANALOGUE)
    gdk_window_resize (face->window, face->allocation.width, face->allocation.height);

  dpy = GDK_WINDOW_XDISPLAY (panel_window->window);
  gpe_system_tray_dock (panel_window->window);

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), panel_window, _("This is the clock.\nTap here to set the alarm, set the time, or change the display format."), NULL);

  g_timeout_add (10000, (GtkFunction) update_time, NULL);

  XSelectInput (dpy, DefaultRootWindow (dpy), PropertyChangeMask);
  gpe_launch_install_filter ();
  gpe_launch_monitor_display (dpy);

  gtk_main ();

  exit (0);
}
