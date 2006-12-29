/*
 * Copyright (C) 2001, 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
 * Hildon adaption 2005 by Matthias Steinbauer <matthias@steinbauer.org>
 * Copyright 2005, 2006 by Florian Boor <florian@kernelconcepts.de>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

#include <gpe/pim-categories-ui.h>
#include <gpe/event-db.h>
#include <gpe/vcal.h>

#ifdef WITH_LIBSCHEDULE
#include <gpe/schedule.h>
#endif

#include <handoff.h>

#ifdef IS_HILDON
/* Hildon includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <libosso.h>
#define APPLICATION_DBUS_SERVICE "gpe_calendar"
#define ICON_PATH "/usr/share/icons/hicolor/26x26/hildon"
#endif

#include "event-ui.h"
#include "globals.h"

#include "view.h"
#include "day_view.h"
#include "week_view.h"
#include "month_view.h"
#include "gtkdatesel.h"
#include "event-cal.h"
#include "event-list.h"
#include "alarm-dialog.h"
#include "calendars-dialog.h"
#include "calendar-update.h"
#include "calendars-widgets.h"
#include "event-menu.h"
#include "import-export.h"

#include <gpe/pim-categories.h>

#include <locale.h>

extern gboolean gpe_calendar_start_xsettings (void (*push_changes) (void));

#define CONF_FILE_ "/.gpe-calendar"
#define CONF_FILE() g_strdup_printf ("%s" CONF_FILE_, g_get_home_dir ())

/* Absolute path to the executable.  */
static const char *gpe_calendar;

/* If the display should be considered tiny.  */
static gboolean display_tiny;
/* If the display is in landscape mode.  */
static gboolean display_landscape;

EventDB *event_db;

static time_t viewtime;

static struct gpe_icon my_icons[] = {
  { "day_view",   DAY_ICON },
  { "week_view",  WEEK_ICON },
  { "month_view", MONTH_ICON },
  { "future_view", FUTURE_ICON },
  { "bell",        BELL_ICON },
  { "recur",       RECUR_ICON },
  { "bell_recur", BELLRECUR_ICON },
  { "icon", APP_ICON },
  { NULL, NULL }
};

/* disabled: user doesn't want to see the control.

   hidden: control not useful at the moment.  Each blocker must
   increment the appropriate hidden by 1 and decrements it by 1 when
   the blocking condition is cleared.  */
GtkWidget *main_window;
#ifdef IS_HILDON
GtkWidget *main_appview;
#endif
static GtkBox *main_box;
static GtkWidget *date_toolbar;
static GtkWidget *today_button;
static GtkToolItem *datesel_item;
static GtkDateSel *datesel;
static GtkContainer *main_panel;
static int current_view_hidden;
static GtkContainer *current_view_container;
static enum
  {
    view_day_view,
    view_week_view,
    view_month_view,
    view_event_list_view
  } current_view_mode;
static GtkWidget *current_view;

static gboolean sidebar_disabled;
static int sidebar_hidden;
static GtkContainer *sidebar_container;
static GtkBox *sidebar;
static gboolean calendar_disabled;
static int calendar_hidden;
static GtkContainer *calendar_container;
static GtkEventCal *calendar;
static gboolean calendars_disabled;
static int calendars_hidden;
static GtkContainer *calendars_container;
static GtkTreeView *calendars;
static gboolean event_list_disabled;
static int event_list_hidden;
static GtkContainer *event_list_container;
static EventList *event_list;

static int main_toolbar_width;
static GtkWidget *main_toolbar;
static GtkWidget *day_button, *month_button;
#ifdef IS_HILDON
static GtkCheckMenuItem *fullscreen_button;
#endif

static void propagate_time (void);

guint week_offset;
gboolean week_starts_sunday;
gboolean day_view_combined_times;
const gchar *TIMEFMT;

static guint reload_source;

/* Schedule atd to wake us up when the next alarm goes off.  */
static gboolean
schedule_wakeup (gboolean reload)
{
  reload_source = 0;

#ifdef WITH_LIBSCHEDULE
  static int broken_at;
  if (broken_at)
    return FALSE;

  static unsigned long uid;
  static time_t wakeup;
  if (uid && ! reload)
    return FALSE;

  if (uid)
    schedule_cancel_alarm (uid, wakeup);

  Event *ev = event_db_next_alarm (event_db, time (NULL));
  if (ev)
    {
      char *action = g_strdup_printf ("%s -s 0", gpe_calendar);
      wakeup = event_get_start (ev) - event_get_alarm (ev);
      if (! schedule_set_alarm (event_get_uid (ev), wakeup,
				action, TRUE))
	{
	  g_warning ("Failed to run at to schedule next alarm");
	  broken_at = 1;
	}
      g_free (action);
      g_object_unref (ev);
    }
#endif

  return FALSE;
}

static void
import_file_list (GSList *import_files, const gchar *selected_calendar)
{
  GSList *i;
  gint n = g_slist_length (import_files);
  gchar *files[n+1];
  
  EventCalendar *ec = NULL;

  if (selected_calendar) 
      ec = event_db_find_calendar_by_name (event_db, selected_calendar);
  if (! ec)
      ec = event_db_get_default_calendar (event_db, selected_calendar);
  
  files[n] = NULL;  
  for (i = import_files; i; i = i->next)
    {
      n--;
      files[n] = i->data;
    }

  GError *error = NULL;
  if (! cal_import_from_files (ec, files, &error) && error)
    {
      fprintf (stderr, "%s", error->message);
      g_error_free (error);
    }

  g_object_unref (ec);
}

static gboolean
export_calendars (EventDB *edb, const gchar *filename, const gchar *name)
{
  GSList *calendars;

  if (!filename)
      filename = "/tmp/gpe-calendar.ics";
    
  if (name)
    {
        EventCalendar *ec = event_db_find_calendar_by_name (edb, name);
        
        if (ec)
          {
            cal_export_to_file (ec, filename, NULL);
            g_object_unref (ec);
          }
        else
          return FALSE;
    }
  else
    {
      gboolean result;
      calendars = event_db_list_event_calendars (edb);
      result = list_export_to_file (calendars, filename, NULL);
      g_slist_free (calendars);
      return result;
    }
    
  return TRUE;
}

static gboolean
delete_event (gchar *id)
{
  Event *ev;
    
  ev = event_db_find_by_eventid (event_db, id);
  if (!ev)
    return FALSE;

  return event_remove (ev);
}

static gboolean
flush_deleted_events (const gchar *calname)
{
  GSList *calendars;
  GSList *iter;

  if (calname)
    {
      EventCalendar *ec = event_db_find_calendar_by_name (event_db, calname);
      
      if (ec)
        {
          event_calendar_flush_deleted (ec);
          g_object_unref (ec);
          return TRUE;
        }
      return FALSE;
    }
  else
    {      
      calendars = event_db_list_event_calendars (event_db);
      for (iter = calendars; iter; iter = iter->next)
        {
          EventCalendar *ec = iter->data;
          event_calendar_flush_deleted (ec);
        }
      g_slist_free (calendars);
  }
  return TRUE;
}

static gboolean
list_calendars (const gchar *filename)
{
  GSList *calendars;
  GSList *iter;
  gint i = 0;
  gint n;
  gchar *s;

  calendars = event_db_list_event_calendars (event_db);
    
  if (!calendars)
    return TRUE;
  
  n = g_slist_length (calendars);
  gchar **list = g_malloc0 ((n + 1) * sizeof (gchar *));
  
  for (iter = calendars; iter; iter = iter->next)
    {
      EventCalendar *ec = iter->data;

      list[i] = event_calendar_get_title (ec);
      i++;
    }
  g_slist_free (calendars);

  s = g_strjoinv ("\n", list);
  g_strfreev (list);
  
  if (filename)
    {
      FILE *f = fopen (filename, "w");
      if (! f)
        {
          g_printerr ("Opening %s: %s", filename, strerror (errno));
          goto error;
        }
    
      if (fputs(s, f) == EOF)
        {
          g_printerr ("Writing to %s: %s", filename, strerror (errno));
          goto error;
        }
      
      fclose (f);
      g_free (s);
      return TRUE;
      
     error:
      if (f)
        fclose (f);
      g_free (s);
      return FALSE;
    }
  else
    g_print ("%s\n", s);
  g_free (s);
  
  return TRUE;
}


static gboolean
save_deleted_events (const gchar *filename, const gchar *calendarname)
{
  GSList *calendars;
  GSList *iter;
  GSList *events = NULL;
  gboolean result = TRUE;

  if (calendarname)
    {
      EventCalendar *ec = event_db_find_calendar_by_name (event_db, calendarname);
      
      if (ec)
        {
          events = event_calendar_list_deleted (ec);
          g_object_unref (ec);
        }
      else  
        return FALSE;
    }
  else
    {      
      calendars = event_db_list_event_calendars (event_db);
      for (iter = calendars; iter; iter = iter->next)
        {
          EventCalendar *ec = iter->data;
    
          events = g_slist_concat (events, event_calendar_list_deleted (ec));
        }
      g_slist_free (calendars);
    }
    
  if (events) 
    {
      result = list_export_to_file (events, filename, NULL);
      g_slist_free (events);
    }
      
  return result;
}

static void
update_view (void)
{
  if (current_view)
    {
      if (GTK_IS_VIEW (current_view))
	gtk_view_reload_events (GTK_VIEW (current_view));
      else if (IS_EVENT_LIST (current_view))
	event_list_reload_events (EVENT_LIST (current_view));
    }

  if (calendar)
    gtk_event_cal_reload_events (calendar);
  if (event_list)
    event_list_reload_events (event_list);
  schedule_wakeup (TRUE);

  if (! reload_source)
    reload_source = g_idle_add ((GSourceFunc) schedule_wakeup, 0);
}

static gboolean just_new;

static gboolean
do_reset_new(gpointer d)
{
  just_new = FALSE;
  return FALSE;
}

static gchar collected_keys[32];
static gint ccpos;

static gboolean
do_insert_text (GtkWidget *window)
{
  GtkWidget *entry;
  
  entry = g_object_get_data(G_OBJECT(window), "default-entry");
  if (entry && ccpos)
    {
      gtk_entry_prepend_text(GTK_ENTRY(entry), collected_keys);
      gtk_editable_set_position(GTK_EDITABLE(entry),-1);
      memset (collected_keys, 0, ccpos);
      ccpos = 0;
    }
  return FALSE;  
}


static void
new_appointment (void)
{
  GtkWidget *appt;
  
  if (just_new)
    return;
  just_new = TRUE;
  g_timeout_add(1000, do_reset_new, NULL);
  
  appt = new_event (viewtime);
  g_timeout_add(500, (GSourceFunc)(do_insert_text), (gpointer)appt);
  gtk_widget_show (appt);
}

static void
set_today (void)
{
  static time_t saved_time;
  static time_t saved_time_at;

  if (saved_time)
    {
      time_t now;
      struct tm ntm;
      struct tm vtm;

      time (&now);
      localtime_r (&now, &ntm);
      localtime_r (&viewtime, &vtm);

      if (now - saved_time_at >= 5 * 60
	  || (ntm.tm_year != vtm.tm_year || ntm.tm_yday != vtm.tm_yday))
	/* The last toggle was more than five minutes ago or the
	   current view time is not today.  Go to today.  */
	{
	  saved_time = viewtime;
	  saved_time_at = now;
	  viewtime = now;
	}
      else
	/* Current view time is today.  Go to the saved time.  */
	{
	  viewtime = saved_time;
	  saved_time = 0;
	}
    }
  else
    /* No saved time.  Just go to today.  */
    {
      saved_time = viewtime;
      time (&viewtime);
    }
  
  propagate_time ();
}

static void
calendars_button_clicked (GtkWidget *widget, gpointer user_data)
{
  static GtkWidget *dialog;

  if (! dialog)
    {
      dialog = calendars_dialog_new (NULL);
      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer *) &dialog);
      gtk_window_set_transient_for
	(GTK_WINDOW (dialog), GTK_WINDOW (gtk_widget_get_toplevel (widget)));
    }

  gtk_widget_show (dialog);
  gtk_window_present (GTK_WINDOW (dialog));
}

static gboolean
set_title (void)
{
  static guint set_title_source;

  if (set_title_source > 0)
    g_source_remove (set_title_source);

  GDate viewing;
  g_date_set_time_t (&viewing, viewtime);

  time_t now = time (NULL);
  GDate today;
  g_date_set_time_t (&today, now);

  char *date;
  char temp[100];
  GDate refresh_at;
  g_date_clear (&refresh_at, 1);

  if (g_date_get_julian (&today) - 1 == g_date_get_julian (&viewing))
    {
      date = _("Yesterday");
      refresh_at = today;
      g_date_add_days (&refresh_at, 1);
    }
  else if (g_date_get_julian (&today) == g_date_get_julian (&viewing))
    {
      date = _("Today");
      refresh_at = today;
      g_date_add_days (&refresh_at, 1);
    }
  else if (g_date_get_julian (&today) + 1 == g_date_get_julian (&viewing))
    {
      date = _("Tomorrow");
      refresh_at = today;
      g_date_add_days (&refresh_at, 1);
    }
  else
    {
      if (g_date_get_year (&today) == g_date_get_year (&viewing))
	g_date_strftime (temp, sizeof (temp), _("%a %b %e"), &viewing);
      else
	g_date_strftime (temp, sizeof (temp), _("%a %b %e, %Y"), &viewing);
      date = temp;

      if (now < viewtime)
	{
	  refresh_at = viewing;
	  g_date_subtract_days (&refresh_at, 1);
	}
    }

  if (g_date_valid (&refresh_at))
    /* The title is relative to the current time, change it at
       midnight.  */
    {
      struct tm tm;
      g_date_to_struct_tm (&refresh_at, &tm);

      set_title_source = g_timeout_add ((mktime (&tm) - now + 1) * 1000,
					(GSourceFunc) set_title, NULL);
    }

  char buffer[200];
  snprintf (buffer, sizeof (buffer), _("Calendar - %s"), date);

#ifdef IS_HILDON
  hildon_app_set_title (HILDON_APP (main_window), buffer);
#else
  gtk_window_set_title (GTK_WINDOW (main_window), buffer);
#endif

  return FALSE;
}

static void
propagate_time (void)
{
  unsigned int day, month, year;
  struct tm tm;

  static gboolean propagating;

  if (propagating)
    return;
  propagating = TRUE;

  localtime_r (&viewtime, &tm);

  if (calendar)
    {
      /* Calendar needs an update?  */
      gtk_calendar_get_date (GTK_CALENDAR (calendar), &year, &month, &day);
      if (tm.tm_year != year - 1900 || tm.tm_mon != month)
	gtk_calendar_select_month (GTK_CALENDAR (calendar),
				   tm.tm_mon, tm.tm_year + 1900);
      if (tm.tm_mday != day)
	gtk_calendar_select_day (GTK_CALENDAR (calendar), tm.tm_mday);
    }

  GDate date;
  gtk_date_sel_get_date (GTK_DATE_SEL (datesel), &date);
  GDate viewing;
  g_date_set_time_t (&viewing, viewtime);

  if (g_date_compare (&date, &viewing) != 0)
    gtk_date_sel_set_date (GTK_DATE_SEL (datesel), &viewing);

  if (current_view && GTK_IS_VIEW (current_view))
    {
      time_t t = gtk_view_get_time (GTK_VIEW (current_view));
      if (t != viewtime)
	gtk_view_set_time (GTK_VIEW (current_view), viewtime);
    }

  set_title ();

  propagating = FALSE;
}

static gboolean
datesel_changed (GtkWidget *datesel, gpointer data)
{
  GDate date;
  gtk_date_sel_get_date (GTK_DATE_SEL (datesel), &date);

  struct tm tm;
  localtime_r (&viewtime, &tm);
  tm.tm_year = g_date_get_year (&date) - 1900;
  tm.tm_mon = g_date_get_month (&date) - 1;
  tm.tm_mday = g_date_get_day (&date);
  tm.tm_isdst = -1;

  viewtime = mktime (&tm);
  propagate_time ();

  return FALSE;
}

static void
calendar_changed (GtkWidget *cal, gpointer data)
{
  unsigned int day, month, year;
  struct tm tm;
  gtk_calendar_get_date (GTK_CALENDAR (cal), &year, &month, &day);
  localtime_r (&viewtime, &tm);
  tm.tm_year = year - 1900;
  tm.tm_mon = month;
  tm.tm_mday = day;
  viewtime = mktime (&tm);

  propagate_time ();
}

static gboolean sidebar_consider (void);

static gboolean
calendar_consider (void)
{
  gboolean s = sidebar_consider ();

  if (calendar_hidden || calendar_disabled || ! s)
    /* Calendar disabled.  Destroy it.  */
    {
      if (! calendar)
	/* Already destroyed, bye.  */
	return FALSE;

      gtk_container_remove (calendar_container,
			    GTK_BIN (calendar_container)->child);
      gtk_widget_hide (GTK_WIDGET (calendar_container));

      return FALSE;
    }
  else
    /* Calendar enabled.  Create it.  */
    {
      if (calendar)
	/* Already enabled, bye.  */
	return TRUE;

      gtk_widget_show (GTK_WIDGET (calendar_container));

      GDate viewing;
      g_date_set_time_t (&viewing, viewtime);

      calendar = GTK_EVENT_CAL (gtk_event_cal_new ());

      /* Set the time.  We set the day to the first as we know that is
	 always valid.  If we set the month and the current day is not
	 valid, GtkCalendar complains.  Pah.  */
      gtk_calendar_select_day (GTK_CALENDAR (calendar), 1);
      gtk_calendar_select_month (GTK_CALENDAR (calendar),
				 g_date_get_month (&viewing) - 1,
				 g_date_get_year (&viewing));
      gtk_calendar_select_day (GTK_CALENDAR (calendar),
			       g_date_get_day (&viewing));

      g_object_add_weak_pointer (G_OBJECT (calendar), (gpointer *) &calendar);
      GTK_WIDGET_UNSET_FLAGS (calendar, GTK_CAN_FOCUS);
      gtk_calendar_set_display_options (GTK_CALENDAR (calendar),
					GTK_CALENDAR_SHOW_DAY_NAMES
					| (week_starts_sunday ? 0
					   : GTK_CALENDAR_WEEK_START_MONDAY));
      g_signal_connect (G_OBJECT (calendar),
			"day-selected", G_CALLBACK (calendar_changed), NULL);
      gtk_container_add (calendar_container, GTK_WIDGET (calendar));
      gtk_widget_show (GTK_WIDGET (calendar));

      return TRUE;
    }
}

static void
calendar_toggle (GtkCheckMenuItem *menuitem, gpointer data)
{
  calendar_disabled = ! gtk_check_menu_item_get_active (menuitem);
  calendar_consider ();
}

static void
calendars_row_inserted (GtkTreeModel *treemodel, GtkTreePath *path,
			GtkTreeIter *iter, gpointer user_data)
{
  GtkTreeView *tree_view = GTK_TREE_VIEW (user_data);

  gtk_tree_view_expand_to_path (tree_view, path);
}

static void
calendars_visible_toggled (GtkCellRendererToggle *cell_renderer,
			   gchar *p, gpointer user_data)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (p);
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      EventCalendar *ec;

      gtk_tree_model_get (model, &iter, COL_CALENDAR, &ec, -1);
      event_calendar_set_visible (ec, ! event_calendar_get_visible (ec));
    }
}

struct sig_info
{
  gpointer instance;
  gulong sig;
};

static void
calendars_cleanup (gpointer data, GObject *tree_view)
{
  struct sig_info *info = data;
  g_signal_handler_disconnect (info->instance, info->sig);
  g_free (info);
}

static gboolean
calendars_consider (void)
{
  gboolean s = sidebar_consider ();

  if (calendars_hidden || calendars_disabled || ! s)
    /* Calendars disabled.  Destroy it.  */
    {
      if (! calendars)
	/* Already destroyed, bye.  */
	return FALSE;

      gtk_container_remove (calendars_container,
			    GTK_BIN (calendars_container)->child);
      gtk_widget_hide (GTK_WIDGET (calendars_container));

      return FALSE;
    }

  /* Calendars enabled.  Create it.  */

  if (calendars)
    /* Already enabled, bye.  */
    return TRUE;

  gtk_widget_show (GTK_WIDGET (calendars_container));

  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (calendars_container, scrolled_window);
  gtk_widget_show (scrolled_window);

  GtkTreeModel *model = calendars_tree_model (event_db);
  GtkTreeView *tree_view
    = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
  g_object_add_weak_pointer (G_OBJECT (tree_view), (gpointer *) &tree_view);
  /* XXX: Ugly maemo hack.  See:
     https://maemo.org/bugzilla/show_bug.cgi?id=538 . */
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (tree_view),
				    "allow-checkbox-mode"))
    g_object_set (tree_view, "allow-checkbox-mode", FALSE, NULL);
  calendars = tree_view;
  g_object_add_weak_pointer (G_OBJECT (calendars), (gpointer *) &calendars);
  gtk_tree_view_expand_all (tree_view);
  gtk_tree_view_set_rules_hint (tree_view, TRUE);
  gtk_tree_view_set_headers_visible (tree_view, FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
		     GTK_WIDGET (tree_view));
  gtk_widget_show (GTK_WIDGET (tree_view));

  struct sig_info *info = g_malloc (sizeof (*info));
  info->instance = model;
  info->sig
    = g_signal_connect (G_OBJECT (model), "row-inserted",
			G_CALLBACK (calendars_row_inserted), tree_view);
  g_object_weak_ref (G_OBJECT (tree_view),
		     calendars_cleanup, info);

  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (calendars_visible_toggled), tree_view);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_cell_layout_set_cell_data_func
    (GTK_CELL_LAYOUT (col), renderer,
     calendar_visible_toggle_cell_data_func, tree_view, NULL);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = calendar_text_cell_renderer_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func
    (GTK_CELL_LAYOUT (col), renderer,
     calendar_text_cell_data_func, tree_view, NULL);

  return TRUE;
}

static void
calendars_toggle (GtkCheckMenuItem *menuitem, gpointer data)
{
  calendars_disabled = ! gtk_check_menu_item_get_active (menuitem);
  calendars_consider ();
}

static void
event_list_event_clicked (EventList *el, Event *ev, GdkEventButton *b,
			  gpointer data)
{
  if (b->button == 1)
    set_time_and_day_view (event_get_start (ev));
  else if (b->button == 3)
    {
      GtkMenu *event_menu = event_menu_new (ev, TRUE);
      gtk_menu_popup (event_menu, NULL, NULL, NULL, NULL,
		      b->button, b->time);
    }
}

static void
event_list_event_key_pressed (EventList *el, Event *ev, GdkEventKey *k,
			      gpointer data)
{
  switch (k->keyval)
    {
    case GDK_space:
    case GDK_Return:
      {
	set_time_and_day_view (event_get_start (ev));
      }
    }
}

static GtkWidget *
event_list_create (EventDB *edb)
{
  GtkWidget *el = event_list_new (edb);
  event_list_set_period_box_visible (EVENT_LIST (el), FALSE);
  g_signal_connect (el, "event-clicked",
		    G_CALLBACK (event_list_event_clicked), NULL);
  g_signal_connect (el, "event-key-pressed",
		    G_CALLBACK (event_list_event_key_pressed), NULL);

  return el;
}

static gboolean
event_list_consider (void)
{
  gboolean s = sidebar_consider ();

  if (event_list_hidden || event_list_disabled || ! s)
    /* Event list disabled.  Destroy it.  */
    {
      if (! event_list)
	/* Already destroyed, bye.  */
	return FALSE;

      gtk_container_remove (event_list_container,
			    GTK_BIN (event_list_container)->child);
      event_list = NULL;
      gtk_widget_hide (GTK_WIDGET (event_list_container));

      return FALSE;
    }
  else
    /* Event list enabled.  Create it.  */
    {
      if (event_list)
	/* Already enabled, bye.  */
	return TRUE;

      gtk_widget_show (GTK_WIDGET (event_list_container));

      event_list = EVENT_LIST (event_list_create (event_db));
      g_object_add_weak_pointer (G_OBJECT (event_list),
				 (gpointer *) &event_list);
      gtk_widget_show (GTK_WIDGET (event_list));
      gtk_container_add (event_list_container, GTK_WIDGET (event_list));
      return TRUE;
    }
}

static void
event_list_toggle (GtkCheckMenuItem *menuitem, gpointer data)
{
  event_list_disabled = ! gtk_check_menu_item_get_active (menuitem);
  event_list_consider ();
}

/* Sidebar:

    +-sidebar----+
    |+-Calendar-+|
    ||          ||
    |+----------+|
    |+-Calendars+|
    ||          ||
    |+----------+|
    |+-Agenda---+|
    ||          ||
    |+----------+|
    +------------+ */
static gboolean
sidebar_consider (void)
{
  int empty = FALSE;
  if ((calendar_hidden || calendar_disabled)
      && (calendars_hidden || calendars_disabled)
      && (event_list_hidden || event_list_disabled))
    empty = TRUE;

  if (sidebar_hidden || sidebar_disabled || empty)
    /* Sidebar disabled.  Destroy it.  */
    {
      if (! sidebar)
	/* Already destroyed, bye.  */
	return FALSE;

      gtk_container_remove (sidebar_container,
			    GTK_BIN (sidebar_container)->child);
      gtk_widget_hide (GTK_WIDGET (sidebar_container));

      calendar_consider ();
      calendars_consider ();
      event_list_consider ();

      return FALSE;
    }
  else
    /* Sidebar enabled.  Create it.  */
    {
      if (sidebar)
	/* Already created, bye.  */
	return TRUE;

      gtk_widget_show (GTK_WIDGET (sidebar_container));

      if (display_landscape)
	sidebar = GTK_BOX (gtk_vbox_new (FALSE, 0));
      else
	sidebar = GTK_BOX (gtk_hbox_new (FALSE, 0));
      g_object_add_weak_pointer (G_OBJECT (sidebar), (gpointer *) &sidebar);
      gtk_container_add (sidebar_container, GTK_WIDGET (sidebar));
      gtk_widget_show (GTK_WIDGET (sidebar));

      GtkPaned *pane1;
      if (display_landscape)
	pane1 = GTK_PANED (gtk_vpaned_new ());
      else
	pane1 = GTK_PANED (gtk_hpaned_new ());
      gtk_box_pack_start (sidebar, GTK_WIDGET (pane1), TRUE, TRUE, 0);
      gtk_widget_show (GTK_WIDGET (pane1));

      GtkWidget *f = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (f), GTK_SHADOW_NONE);
      calendar_container = GTK_CONTAINER (f);
      gtk_paned_pack1 (pane1, f, FALSE, TRUE);

      calendar_consider ();

      GtkPaned *pane2;
      if (display_landscape)
	pane2 = GTK_PANED (gtk_vpaned_new ());
      else
	pane2 = GTK_PANED (gtk_hpaned_new ());
      gtk_paned_pack2 (pane1, GTK_WIDGET (pane2), TRUE, TRUE);
      gtk_widget_show (GTK_WIDGET (pane2));

      f = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (f), GTK_SHADOW_NONE);
      calendars_container = GTK_CONTAINER (f);
      gtk_paned_pack1 (pane2, GTK_WIDGET (f), TRUE, TRUE);

      calendars_consider ();

      f = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (f), GTK_SHADOW_NONE);
      event_list_container = GTK_CONTAINER (f);
      gtk_paned_pack2 (pane2, GTK_WIDGET (f), TRUE, TRUE);

      event_list_consider ();

      return TRUE;
    }
}

static void
sidebar_toggle (GtkCheckMenuItem *menuitem, gpointer data)
{
  sidebar_disabled = ! gtk_check_menu_item_get_active (menuitem);
  sidebar_consider ();
}

static void
time_changed (GtkView *view, time_t time, gpointer data)
{
  viewtime = time;
  propagate_time ();
}

/* Build the layout for the cal view.

     +-current_view_container-----------------+
     |+-current_view-----------++-sidebar----+|
     ||                        ||            ||
     ||                        ||            ||
     ||                        ||            ||
     ||                        ||            ||
     ||                        ||            ||
     |+------------------------++------------+|
     +----------------------------------------+
  */
static gboolean
current_view_consider (void)
{
  if (current_view_hidden)
    /* Disable current view.  */
    {
      if (! current_view)
	/* Nothing to do.  */
	return FALSE;

      gtk_container_remove (current_view_container, current_view);

      if (IS_EVENT_LIST (current_view))
        event_list_hidden --;
      if (GTK_IS_MONTH_VIEW (current_view))
        calendar_hidden --;

      event_list_consider ();
      calendar_consider ();

      return FALSE;
    }

  gtk_widget_show (GTK_WIDGET (current_view_container));

  if (current_view)
    {
      switch (current_view_mode)
        {
        case view_day_view:
          if (IS_DAY_VIEW (current_view))
            return TRUE;
          break;
        case view_week_view:
          if (GTK_IS_WEEK_VIEW (current_view))
            return TRUE;
          break;
        case view_month_view:
          if (GTK_IS_MONTH_VIEW (current_view))
            return TRUE;
          break;
        case view_event_list_view:
          if (IS_EVENT_LIST (current_view))
            return TRUE;
          break;
        }

      if (GTK_IS_MONTH_VIEW (current_view))
        calendar_hidden --;
      if (IS_EVENT_LIST (current_view))
        {
          event_list_hidden --;
          event_list = EVENT_LIST (current_view);
          gtk_widget_reparent (GTK_WIDGET (event_list),
                       GTK_WIDGET (event_list_container));
          event_list_set_period_box_visible (event_list, FALSE);
          gtk_widget_show (GTK_WIDGET (event_list_container));
        }
      else
        gtk_container_remove (current_view_container, current_view);
    }

  switch (current_view_mode)
    {
    case view_day_view:
      current_view = day_view_new (viewtime);
      gtk_date_sel_set_mode (datesel, GTKDATESEL_FULL);
      gtk_widget_show (GTK_WIDGET (datesel));
      gtk_widget_show (today_button);
      break;
    case view_week_view:
      current_view = gtk_week_view_new (viewtime);
      gtk_date_sel_set_mode (datesel, GTKDATESEL_WEEK);
      gtk_widget_show (GTK_WIDGET (datesel));
      gtk_widget_show (today_button);
      break;
    case view_month_view:
      calendar_hidden ++;
      current_view = gtk_month_view_new (viewtime);
      gtk_date_sel_set_mode (datesel, GTKDATESEL_MONTH);
      gtk_widget_show (GTK_WIDGET (datesel));
      gtk_widget_show (today_button);
      break;
    case view_event_list_view:
      event_list_hidden ++;
      if (event_list)
        {
          g_object_ref (event_list);
          gtk_container_remove (event_list_container,
                    GTK_WIDGET (event_list));
          current_view = GTK_WIDGET (event_list);
          event_list = NULL;
          gtk_widget_hide (GTK_WIDGET (event_list_container));
        }
      else
        current_view = event_list_create (event_db);
      event_list_set_period_box_visible (EVENT_LIST (current_view), TRUE);
      gtk_widget_hide (GTK_WIDGET (datesel));
      gtk_widget_hide (today_button);
      break;
    }

  if (GTK_IS_VIEW (current_view))
    g_signal_connect (G_OBJECT (current_view),
		      "time-changed", G_CALLBACK (time_changed), NULL);

  gtk_container_add (current_view_container, current_view);
  gtk_widget_show (current_view);

  gtk_widget_grab_focus (current_view);

  event_list_consider ();
  calendar_consider ();

  return TRUE;
}

void
set_time_and_day_view (time_t selected_time)
{
  viewtime = selected_time;
  propagate_time ();
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (day_button),
				     TRUE);
}

static void
day_view_button_clicked (GtkWidget *widget, gpointer d)
{
  if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    return;

  current_view_mode = view_day_view;
  current_view_consider ();
}

static void
week_view_button_clicked (GtkWidget *widget, gpointer d)
{
  if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    return;

  current_view_mode = view_week_view;
  current_view_consider ();
}

static void
month_view_button_clicked (GtkWidget *widget, gpointer d)
{
  if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    return;

  current_view_mode = view_month_view;
  current_view_consider ();
}

static void
event_list_button_clicked (GtkWidget *widget, gpointer d)
{
  if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    return;

  current_view_mode = view_event_list_view;
  current_view_consider ();
}

static AlarmDialog *alarm_dialog;

static void
alarm_dialog_show_event (AlarmDialog *alarm_dialog, Event *ev)
{
  set_time_and_day_view (event_get_start (ev));
  gtk_widget_hide (GTK_WIDGET (alarm_dialog));
}

static void
alarm_dialog_required (void)
{
  if (! alarm_dialog)
    {
      alarm_dialog = ALARM_DIALOG (alarm_dialog_new ());
      gtk_window_set_transient_for (GTK_WINDOW (alarm_dialog),
				    GTK_WINDOW (main_window));

      g_signal_connect (G_OBJECT (alarm_dialog), "show-event",
			G_CALLBACK (alarm_dialog_show_event), NULL);

    }
}

static void
alarm_fired (EventDB *edb, Event *ev)
{
  alarm_dialog_required ();

  alarm_dialog_add_event (alarm_dialog, ev);

  gtk_window_present (GTK_WINDOW (alarm_dialog));
}

static gboolean
alarms_process_pending (gpointer data)
{
  EventDB *event_db = EVENT_DB (data);

  g_signal_connect (G_OBJECT (event_db), "alarm-fired",
                    G_CALLBACK (alarm_fired), NULL);
  GSList *list = event_db_list_unacknowledged_alarms (event_db);
  list = g_slist_sort (list, event_alarm_compare_func);
  GSList *i;
  for (i = list; i; i = g_slist_next (i))
    alarm_fired (event_db, EVENT (i->data));
  event_list_unref (list);

  /* Don't run again.  */
  return FALSE;
}

static void
alarm_button_clicked (GtkWidget *widget, gpointer d)
{
  alarm_dialog_required ();
  gtk_widget_show (GTK_WIDGET (alarm_dialog));
  gtk_window_present (GTK_WINDOW (alarm_dialog));
}

static void
import_callback (GtkWidget *widget, gpointer user_data)
{
  cal_import_dialog (NULL, NULL);
}

static void
view_toggled (GtkWidget *widget, gpointer data)
{
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (data), TRUE);
}

/* Write the configuration file.  */
static void
conf_write (void)
{
  GKeyFile *conf = g_key_file_new ();
  char *filename = CONF_FILE ();
  g_key_file_load_from_file (conf, filename, G_KEY_FILE_KEEP_COMMENTS, NULL);
  g_free (filename);

  const char *mode = NULL;
  switch (current_view_mode)
    {
    case view_day_view:
      mode = "day";
      break;
    case view_week_view:
      mode = "week";
      break;
    case view_month_view:
      mode = "month";
      break;
    case view_event_list_view:
      mode = "event list";
      break;
    }

  if (mode)
    g_key_file_set_string (conf, "gpe-calendar", "current-view-mode", mode);

  g_key_file_set_boolean (conf, "gpe-calendar", "sidebar-disabled",
			  sidebar_disabled);
  g_key_file_set_boolean (conf, "gpe-calendar", "calendar-disabled",
			  calendar_disabled);
  g_key_file_set_boolean (conf, "gpe-calendar", "calendars-disabled",
			  calendars_disabled);
  g_key_file_set_boolean (conf, "gpe-calendar", "event-list-disabled",
			  event_list_disabled);

  g_key_file_set_integer (conf, "gpe-calendar", "window-width",
			  main_window->allocation.width);
  g_key_file_set_integer (conf, "gpe-calendar", "window-height",
			  main_window->allocation.height);

  gsize length;
  char *data = g_key_file_to_data (conf, &length, NULL);
  g_key_file_free (conf);
  if (data)
    {
      char *filename = CONF_FILE ();
      FILE *f = fopen (filename, "w");
      g_free (filename);
      if (f)
	{
	  fwrite (data, length, 1, f);
	  fclose (f);
	}
      g_free (data);
    }
}

static void
gpe_cal_exit (void)
{
  g_object_unref (event_db);

  conf_write ();

  gtk_main_quit ();
}

static void
gpe_cal_iconify (void)
{
  gtk_window_iconify (GTK_WINDOW (main_window));
}

#ifdef IS_HILDON
static void
toggle_fullscreen (GtkCheckMenuItem *menuitem, gpointer user_data)
{
  hildon_appview_set_fullscreen (HILDON_APPVIEW (main_appview),
				 gtk_check_menu_item_get_active (menuitem));
}
#endif /*IS_HILDON*/

static void
toggle_toolbar (GtkCheckMenuItem *menuitem, GtkWidget *toolbar)
{
  if (gtk_check_menu_item_get_active(menuitem))
    gtk_widget_show(toolbar);
  else
    gtk_widget_hide(toolbar);
}

static void
edit_categories (GtkWidget *w)
{
  GtkWidget *dialog;

  /* XXX: Braindamage.  */
#ifdef IS_HILDON
  dialog = gpe_pim_categories_dialog (NULL, FALSE, NULL, NULL);
#else
  dialog = gpe_pim_categories_dialog (NULL, NULL, NULL);
#endif
  gtk_window_set_transient_for(GTK_WINDOW(dialog), 
                               GTK_WINDOW(gtk_widget_get_toplevel(w)));
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
}

static gboolean
main_window_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *data)
{
#ifdef IS_HILDON
    /* in hildon there is nothing like control, shift etc buttons */
    switch (k->keyval)
      {
      case GDK_F6:
	/* toggle button for going full screen */
	gtk_check_menu_item_set_active
	  (fullscreen_button,
	   ! gtk_check_menu_item_get_active (fullscreen_button));
        break;	
      }
#else
  if (k->state & GDK_CONTROL_MASK)
    switch (k->keyval)
      {
        case GDK_n:
          new_appointment();
        break;	
        case GDK_o:
        case GDK_i:
          cal_import_dialog (NULL, NULL);
        break;	
        case GDK_t:
          set_today();
        break;	
        case GDK_q:
          gpe_cal_iconify();
        break;	
      }
#endif /*IS_HILDON*/
  /* ignore if ctrl or alt pressed */    
  if ((k->state & GDK_CONTROL_MASK) 
       || (k->state & GDK_MOD1_MASK))
    return FALSE;
  if (k->keyval == '-'
#if IS_HILDON
      || k->keyval == GDK_F8
#endif
      )
    /* Zoom out.  */
    if (current_view && IS_DAY_VIEW (current_view))
      {
	gtk_toggle_tool_button_set_active
	  (GTK_TOGGLE_TOOL_BUTTON (month_button), TRUE);
	current_view_consider ();
	month_view_set_zoom (GTK_MONTH_VIEW (current_view), -1);
	return TRUE;
      }

  if (k->keyval == GDK_Home)
    {
      viewtime = time (NULL);
      propagate_time ();
      return TRUE;
    }

  /* automatic event */
  if (k->string && isalpha(k->string[0]))
    {
        if (!just_new) 
          new_appointment();
        if (ccpos < sizeof (collected_keys) - 1) 
          collected_keys[ccpos] = k->string[0];
        ccpos++;
        return TRUE;
    }
    
  return FALSE;
}

/* Another instance started and has passed some state to us.  */
static void
handoff_callback (Handoff *handoff, char *data)
{
  char *line = data;
  while (line && *line)
    {
      char *var = line;

      char *end = strchr (line, '\n');
      if (! end)
        {
          end = line + strlen (line);
          line = 0;
        }
      else
        line = end + 1;
      *end = 0;

      char *equal = strchr (var, '=');
      if (equal)
        *equal = 0;

      char *value;
      if (equal)
        value = equal + 1;
      else
        value = NULL;

      if (strcmp (var, "IMPORT_FILE") == 0 && value)
        {
          const char *files[] = { value, NULL };
          gchar *calendar;
          EventCalendar *ec;

          calendar = strchr (value, '*');
          if (calendar)
            {
              calendar [0] = 0;
              calendar++;
              ec = event_db_find_calendar_by_name (event_db, calendar);
              if (! ec)
                ec = event_db_get_default_calendar(event_db, strlen(calendar) ? calendar : NULL);
	    }
          else
	    ec = event_db_get_default_calendar(event_db, NULL);
	  cal_import_from_files (ec, files, NULL);
	  g_object_unref (ec);
        }
      else if (strcmp (var, "VIEWTIME") == 0 && value)
        {
          time_t t = atoi (value);
          if (t > 0)
            {
              viewtime = t;
              propagate_time ();
            }
        }
    else if (strcmp (var, "EXPORT") == 0 && value)
      {
        gchar *calendar, *file;
        
        file = value;
        calendar = strchr (value, '*');
        if (calendar)
          {
            calendar [0] = 0;
            calendar++;
            if (strlen (calendar))
              export_calendars (event_db, file, calendar);
            else
	          export_calendars (event_db, file, NULL);
          }
        else
          {
	        export_calendars (event_db, file, NULL);
          }
      }
    else if (strcmp (var, "FLUSH") == 0)
      flush_deleted_events (value);
    else if (strcmp (var, "DELETE") == 0 && value)
	  delete_event (value);
    else if (strcmp (var, "LIST_DELETED") == 0 && value)
      {
        gchar *calendar, *file;

        file = value;
        calendar = strchr (value, '*');
        if (calendar)
          {
            calendar [0] = 0;
            calendar++;
            if (strlen (calendar))
                save_deleted_events (file, calendar);
            else
  	            save_deleted_events (file, NULL);
          }
        else
          {
	        save_deleted_events (file, NULL);
          }
      }
    else if (strcmp (var, "LIST_CALENDARS") == 0)
      list_calendars (value);
    else if (strcmp (var, "FOCUS") == 0)
      gtk_window_present (GTK_WINDOW (main_window));
    else
	  g_warning ("%s: Unknown command: %s", __func__, var);
    }
}

/* Serialize our state: another instance will take over (e.g. on
   another display).  */
static char *
handoff_serialize (Handoff *handoff)
{
  conf_write ();
  return g_strdup_printf ("VIEWTIME=%ld\n", viewtime);
}

#ifdef IS_HILDON
static void
osso_top_callback (const gchar *arguments, gpointer data)
{
  handoff_callback (NULL, arguments);
}
#endif

static void
toolbar_size_allocate (GtkWidget *widget, GtkAllocation *allocation,
		       gpointer user_data)
{
  if (! datesel_item || ! today_button)
    return;

  if (! date_toolbar
      && main_toolbar_width > main_toolbar->allocation.width)
    /* Not enough space with a single toolbar.  */
    {
      date_toolbar = gtk_toolbar_new ();
      gtk_toolbar_set_orientation (GTK_TOOLBAR (date_toolbar),
				   GTK_ORIENTATION_HORIZONTAL);
      gtk_toolbar_set_style (GTK_TOOLBAR (date_toolbar), GTK_TOOLBAR_ICONS);
      GTK_WIDGET_UNSET_FLAGS (date_toolbar, GTK_CAN_FOCUS);
      gtk_widget_show (date_toolbar);
      gtk_box_pack_end (main_box, date_toolbar, FALSE, FALSE, 0);

      g_object_ref (G_OBJECT (today_button));
      gtk_container_remove (GTK_CONTAINER (main_toolbar), today_button);
      gtk_toolbar_insert (GTK_TOOLBAR (date_toolbar),
			  GTK_TOOL_ITEM (today_button), -1);

      g_object_ref (G_OBJECT (datesel_item));
      gtk_container_remove (GTK_CONTAINER (main_toolbar),
			    GTK_WIDGET (datesel_item));
      gtk_toolbar_insert (GTK_TOOLBAR (date_toolbar),
			  datesel_item, -1);
    }
  else if (date_toolbar
	   && main_toolbar_width <= main_toolbar->allocation.width)
    /* Enough space for a single toolbar.  */
    {
      g_object_ref (G_OBJECT (today_button));
      gtk_container_remove (GTK_CONTAINER (date_toolbar), today_button);
      gtk_toolbar_insert (GTK_TOOLBAR (main_toolbar),
			  GTK_TOOL_ITEM (today_button), -1);

      g_object_ref (G_OBJECT (datesel_item));
      gtk_container_remove (GTK_CONTAINER (date_toolbar),
			    GTK_WIDGET (datesel_item));
      gtk_toolbar_insert (GTK_TOOLBAR (main_toolbar),
			  datesel_item, -1);

      gtk_widget_destroy (date_toolbar);
      date_toolbar = NULL;
    }
}

static void
show_help_and_exit (void)
{
  g_print ("\nUsage: gpe-calendar [-hsf] -C [<file>] [-c <calendar>] [-i <file>] [-e <file>] [-d <id>] [-D <file>]\n\n");
  g_print ("-h          : Show this help\n");
  g_print ("-s          : Schedule and exit\n");
  g_print ("-d <id>     : Delete an event with the given id.\n");
  g_print ("-C          : List defined calendar names.\n");
  g_print ("              If no file is given the list is written to stdout.\n");
  g_print ("-c <name>   : Specify a calendar for the actions below.\n");
  g_print ("-f          : Flush list of deleted events. If a calendar name\n");
  g_print ("              is given only the events of this calendar are flushed.\n");
  g_print ("-i <file>   : Import a given file (to the specified calendar if set).\n");
  g_print ("-D <file>   : Write list of deleted events in a calendar to a given file.\n");
  g_print ("-e <file>   : Write calendar from database to ics file using the given filename.\n");
  g_print ("              If no calendar is selected the given filename is treated as a prefix.\n\n");
  g_print ("Without command line option the GUI is launched or an already running GUI is activated.\n\n");    
  exit (EXIT_SUCCESS);
}

int
main (int argc, char *argv[])
{
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkTooltips *tooltips;    
#ifdef IS_HILDON
  osso_context_t *osso_context;
#endif

  /* What thread?!  Yes, threads.  gpe-calendar is entirely event
     driven.  No threads thank you very much.  But libsoup (which is
     used to do calendar synchronization) requires threads.  FUCK!
     But it appears to make the promise that all callbacks will be
     executed in the main context.  At least that is how I've read the
     documentation.  Hopefully that is correct.  */
  g_thread_init (NULL);

  char *current_dir = NULL;

  if (g_path_is_absolute (argv[0]))
    gpe_calendar = argv[0];
  else
    {
      current_dir = g_get_current_dir ();
      gpe_calendar = g_build_filename (current_dir, argv[0], NULL);
    }

  setlocale (LC_ALL, "");

  /* Initialize the g_object system.  */
  g_type_init ();

  /* Parse the arguments.  */
  gboolean schedule_only = FALSE;
  gchar *state = NULL;
  GSList *import_files = NULL;
  gboolean export_only = FALSE;
  gchar *export_file = NULL; /* make this a list */
  gboolean delete_only = FALSE;
  gchar *delete_id = NULL;
  gboolean flush_deleted_only = FALSE;
  gboolean list_calendars_only = FALSE;
  gchar *calendar_list_file = NULL;
  gchar *delete_list_file = NULL;
  gboolean list_deleted_only = FALSE;
  gchar *selected_calendar = NULL; /* make this a list */

  int option_letter;
  extern char *optarg;
  while ((option_letter = getopt (argc, argv, "c:s:e:i:hd:fD:C::")) != -1)
    {
      if (option_letter == 'c')
        selected_calendar = g_strdup (optarg);
      if (option_letter == 'h')
        show_help_and_exit ();
      if (option_letter == 's')
        schedule_only = TRUE;
      if (option_letter == 'f')
        {
          char *s = g_strdup_printf ("%s%sFLUSH", state ?: "", state ? "\n" : "");
          g_free (state);
          state = s;
          flush_deleted_only = TRUE;
        }
      if (option_letter == 'C')
        {
          list_calendars_only = TRUE;
          if (optarg)
            {
              calendar_list_file = g_strdup (optarg);
              char *s
                = g_strdup_printf ("%s%sLIST_CALENDARS=%s%s%s",
                            state ?: "", state ? "\n" : "",
                            *optarg == '/' ? "" : current_dir,
                            *optarg == '/' ? "" : G_DIR_SEPARATOR_S,
                           optarg);
              g_free (state);
              state = s;
            }
          if (! current_dir)
            current_dir = g_get_current_dir ();
        }
      if (option_letter == 'e')
        {
          if (export_only) /* only one so far */
              continue;
          export_only = TRUE;
          export_file = g_strdup (optarg);
          if (! current_dir)
            current_dir = g_get_current_dir ();
        }
      if (option_letter == 'D')
        {
          if (list_deleted_only)
              continue;
          list_deleted_only = TRUE;
          delete_list_file = g_strdup (optarg);
          if (! current_dir)
            current_dir = g_get_current_dir ();
        }
      if (option_letter == 'd')
        {
          if (delete_only) /* only one so far */
              continue;
          delete_only = TRUE;
          delete_id = g_strdup (optarg);
          char *s
            = g_strdup_printf ("%s%sDELETE=%s",
                       state ?: "", state ? "\n" : "",
                       optarg);
          g_free (state);
          state = s;
        }
      else if (option_letter == 'i')
        {
          if (! current_dir)
            current_dir = g_get_current_dir ();
    
          import_files = g_slist_append (import_files, optarg);
        }
    }

    if (import_files)
      {
          GSList *iter;
          
          for (iter = import_files; iter; iter = iter->next)
            {
              gchar *file = iter->data; 
              gchar *s
                = g_strdup_printf ("%s%sIMPORT_FILE=%s%s%s%s%s",
                           state ?: "", state ? "\n" : "",
                           *file == '/' ? "" : current_dir,
                           *file == '/' ? "" : G_DIR_SEPARATOR_S,
                           file, selected_calendar ? "*" : "", 
                           selected_calendar ? selected_calendar : "" );
              g_free (state);
              state = s;
            }
      }
    
  if (!schedule_only && !export_only && !delete_only 
      && !flush_deleted_only && !list_deleted_only && !list_calendars_only)
    {
      char *s = g_strdup_printf ("%s%sFOCUS", state ?: "", state ? "\n" : "");
      g_free (state);
      state = s;
    }

  if (export_only)
    {
      char *s
        = g_strdup_printf ("%s%sEXPORT=%s%s%s*%s",
                   state ?: "", state ? "\n" : "",
                   *export_file == '/' ? "" : current_dir,
                   *export_file == '/' ? "" : G_DIR_SEPARATOR_S,
                   export_file, selected_calendar ? selected_calendar : "" );
      g_free (state);
      state = s;
    }
  if (list_deleted_only)
    {
      char *s
        = g_strdup_printf ("%s%sLIST_DELETED=%s%s%s*%s",
                   state ?: "", state ? "\n" : "",
                   *delete_list_file == '/' ? "" : current_dir,
                   *delete_list_file == '/' ? "" : G_DIR_SEPARATOR_S,
                   delete_list_file, selected_calendar ? selected_calendar : "");
      g_free (state);
      state = s;
    }
    
    
  if (current_dir)
    g_free (current_dir);
  
  /* See if there is another instance of gpe-calendar already running.
     If so, try to handoff any arguments and exit.  Otherwise, take
     over.  */

  Handoff *handoff = handoff_new ();

  const char *home = g_get_home_dir ();
#define RENDEZ_VOUS "/.gpe-calendar-rendezvous"
  char *rendez_vous = alloca (strlen (home) + strlen (RENDEZ_VOUS) + 1);
  sprintf (rendez_vous, "%s" RENDEZ_VOUS, home);

  g_signal_connect (G_OBJECT (handoff), "handoff",
                    G_CALLBACK (handoff_callback), NULL);

  if (handoff_handoff (handoff, rendez_vous, state,
		       schedule_only ? FALSE : TRUE,
		       handoff_serialize, NULL))
    exit (EXIT_SUCCESS);
  


  if (schedule_only || export_only || delete_only || flush_deleted_only 
      || list_deleted_only || list_calendars_only)
    {
      char *filename = CALENDAR_FILE ();
      event_db = event_db_new (filename);
      if (! event_db)
        {
          g_critical ("Failed to open event database.");
          exit (1);
        }
    }
    
  /* No instance running but called with -s. */
  if (schedule_only)
    {      
      schedule_wakeup (1);
      exit (EXIT_SUCCESS);
    }
    
  /* No instance running but called with -e. */
  if (export_only)
    {
      if (export_calendars (event_db, export_file, selected_calendar))
        exit (EXIT_SUCCESS);
      else
        exit (EXIT_FAILURE);
    }
    
  /* No instance running but called with -d. */
  if (delete_only)
    {
      if (delete_event (delete_id))
        exit (EXIT_SUCCESS);
      else
        exit (EXIT_FAILURE);
    }
    
  /* No instance running but called with -D. */
  if (list_deleted_only)
    {
      if (save_deleted_events (delete_list_file, selected_calendar))
        exit (EXIT_SUCCESS);
      else
        exit (EXIT_FAILURE);
    }
    
  /* No instance running but called with -C. */
  if (list_calendars_only)
    {
      if (list_calendars (calendar_list_file))
        exit (EXIT_SUCCESS);
      else
        exit (EXIT_FAILURE);
    }
    
  /* Called with -f. */
  if (flush_deleted_only)
    {      
      if (flush_deleted_events (selected_calendar))
        exit (EXIT_SUCCESS);
      else
        exit (EXIT_FAILURE);
    }
    
  g_free (state);

  /* Start gpe-calendar.  */

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  /* Set the TIMEFMT.  */

  gchar timebuf[3];
  struct tm tm;
  memset (&tm, 0, sizeof (tm));
  if (strftime (timebuf, sizeof (timebuf), "%p", &tm))
    TIMEFMT = "%I:%M %p";
  else
    TIMEFMT = "%R";

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  /* Load the event database.  */
  char *filename = CALENDAR_FILE ();
  event_db = event_db_new (filename);
  if (! event_db)
    {
      g_critical ("Failed to open event database: %s.", filename);
      exit (1);
    }
  g_free (filename);

  /* Process the pending alarms once the system is up and running.  */
  g_idle_add (alarms_process_pending, event_db);
  /* And schedule the next wake up.  */
  g_idle_add ((GSourceFunc) schedule_wakeup, 0);
  /* And schedule the next wake up.  */
  g_idle_add ((GSourceFunc) calendars_sync_start, 0);

  /* XXX: We should be more intelligent about changes but this will at
     least work.  */
  g_signal_connect (G_OBJECT (event_db), "calendar-changed",
		    G_CALLBACK (update_view), NULL);
  g_signal_connect (G_OBJECT (event_db), "calendar-deleted",
		    G_CALLBACK (update_view), NULL);
  g_signal_connect (G_OBJECT (event_db), "calendar-reparented",
		    G_CALLBACK (update_view), NULL);
  g_signal_connect (G_OBJECT (event_db), "event-new",
		    G_CALLBACK (update_view), NULL);
  g_signal_connect (G_OBJECT (event_db), "event-removed",
		    G_CALLBACK (update_view), NULL);
  g_signal_connect (G_OBJECT (event_db), "event-modified",
		    G_CALLBACK (update_view), NULL);

  if (gpe_pim_categories_init () == FALSE)
    exit (1);

  /* Import any files specified on the command line.  */
  if (import_files)
    {
      import_file_list (import_files, selected_calendar);
      g_slist_free (import_files);
      exit (EXIT_SUCCESS);
    }
	
#ifdef IS_HILDON
  /* Initialize maemo application */
  osso_context = osso_initialize(APPLICATION_DBUS_SERVICE, "0.1", TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL)
    return OSSO_ERROR;

  osso_application_set_top_cb (osso_context, osso_top_callback, NULL);
#endif

  /* If a display migration occurred, this may already be set.  */
  if (! viewtime)
    time (&viewtime);

  /* Load some defaults.  */
  guint window_x = CLAMP (gdk_screen_width () * 7 / 8, 240, 1000);
  guint window_y = CLAMP (gdk_screen_height () * 7 / 8, 310, 800);
  display_tiny = gdk_screen_width () < 300;
  display_landscape = gdk_screen_width () > gdk_screen_height ()
    && gdk_screen_width () >= 640;

  if (display_tiny)
    sidebar_disabled = TRUE;
  if (MIN (gdk_screen_width (), gdk_screen_height ()) < 640)
    calendar_disabled = TRUE;

  /* Read the configuration file.  */
  GKeyFile *conf = g_key_file_new ();
  filename = CONF_FILE ();
  if (g_key_file_load_from_file (conf, filename, 0, NULL))
    {
      char *v;

      v = g_key_file_get_string (conf, "gpe-calendar",
				 "current-view-mode", NULL);
      if (v)
	{
	  if (strcmp (v, "day") == 0)
	    current_view_mode = view_day_view;
	  else if (strcmp (v, "week") == 0)
	    current_view_mode = view_week_view;
	  else if (strcmp (v, "month") == 0)
	    current_view_mode = view_month_view;
	  else if (strcmp (v, "event list") == 0)
	    current_view_mode = view_event_list_view;

	  g_free (v);
	}

      GError *error = NULL;
      gboolean b;
      b = g_key_file_get_boolean (conf, "gpe-calendar",
				  "sidebar-disabled", &error);
      if (error)
	{
	  g_error_free (error);
	  error = NULL;
	}
      else
	sidebar_disabled = b;

      b = g_key_file_get_boolean (conf, "gpe-calendar",
				  "calendar-disabled", &error);
      if (error)
	{
	  g_error_free (error);
	  error = NULL;
	}
      else
	calendar_disabled = b;

      b = g_key_file_get_boolean (conf, "gpe-calendar",
				  "calendars-disabled", &error);
      if (error)
	{
	  g_error_free (error);
	  error = NULL;
	}
      else
	calendars_disabled = b;

      b = g_key_file_get_boolean (conf, "gpe-calendar",
				  "event-list-disabled", &error);
      if (error)
	{
	  g_error_free (error);
	  error = NULL;
	}
      else
	event_list_disabled = b;

      int i;
      i = g_key_file_get_boolean (conf, "gpe-calendar",
				  "window-width", &error);
      if (error)
	{
	  g_error_free (error);
	  error = NULL;
	}
      else
	window_x = i;

      i = g_key_file_get_boolean (conf, "gpe-calendar",
				  "window-height", &error);
      if (error)
	{
	  g_error_free (error);
	  error = NULL;
	}
      else
	window_y = i;
    }
  g_free (filename);
  g_key_file_free (conf);

#ifdef IS_HILDON
  main_window = hildon_app_new ();
  hildon_app_set_two_part_title (HILDON_APP (main_window), FALSE);
  main_appview = hildon_appview_new (_("Main"));
  hildon_app_set_appview (HILDON_APP (main_window),
			  HILDON_APPVIEW (main_appview));
#else    
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (main_window), window_x, window_y);
#endif
  set_title ();
  g_signal_connect (G_OBJECT (main_window), "delete-event",
                    G_CALLBACK (gpe_cal_iconify), NULL);
  gtk_widget_show (main_window);

  main_box = GTK_BOX (gtk_vbox_new (FALSE, 0));
#if IS_HILDON
  gtk_container_add (GTK_CONTAINER (main_appview), GTK_WIDGET (main_box));
#else
  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (main_box));
#endif
  gtk_widget_show (GTK_WIDGET (main_box));

  /* Menu bar.  */
  GtkMenuShell *menu_main;
#ifdef IS_HILDON
  menu_main
    = GTK_MENU_SHELL (hildon_appview_get_menu (HILDON_APPVIEW (main_appview)));
#else
  menu_main = GTK_MENU_SHELL (gtk_menu_bar_new ());
  gtk_box_pack_start (main_box, GTK_WIDGET (menu_main), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (menu_main));
#endif

  /* Tool bar.  We fill it before the menu bar as the menu bar
     requires some of its widgets.  */
  tooltips = gtk_tooltips_new ();
  gtk_tooltips_enable (tooltips);

  GtkWidget *toolbar = gtk_toolbar_new ();
  g_signal_connect (G_OBJECT (toolbar), "size-allocate",
		    G_CALLBACK (toolbar_size_allocate), NULL);
  main_toolbar = toolbar;
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);
  gtk_widget_show (toolbar);

#ifdef IS_HILDON
  hildon_appview_set_toolbar (HILDON_APPVIEW (main_appview),
			      GTK_TOOLBAR (toolbar));
  gtk_widget_show_all (main_appview);
#else
  gtk_box_pack_start (GTK_BOX (main_box), toolbar, FALSE, FALSE, 0);
#endif


  GtkToolItem *item;

  /* Initialize the day view button.  */
  p = gpe_find_icon_scaled ("day_view", 
                            gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  gtk_widget_show (GTK_WIDGET (pw));
  item = gtk_radio_tool_button_new (NULL);
  day_button = GTK_WIDGET(item);    
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), _("Day"));
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (item), pw);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item),
				     current_view_mode == view_day_view);
  g_signal_connect(G_OBJECT(item), "toggled",
		   G_CALLBACK (day_view_button_clicked), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Tap here to select day-at-a-time view."), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);
  gtk_widget_show (GTK_WIDGET (item));
    
  /* Initialize the week view button.  */
  p = gpe_find_icon_scaled ("week_view",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  gtk_widget_show (GTK_WIDGET (pw));
  item = gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Week"));
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), pw);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item),
				     current_view_mode == view_week_view);
  g_signal_connect(G_OBJECT(item), "toggled",
		   G_CALLBACK (week_view_button_clicked), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Tap here to select week-at-a-time view."), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);
  gtk_widget_show (GTK_WIDGET (item));
  GtkWidget *week_button = GTK_WIDGET (item);    
  
  /* Initialize the month view button.  */
  p = gpe_find_icon_scaled ("month_view",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  gtk_widget_show (GTK_WIDGET (pw));
  item = gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(item));
  month_button = GTK_WIDGET (item);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Month"));
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), pw);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item),
				     current_view_mode == view_month_view);
  g_signal_connect (G_OBJECT(item), "toggled",
		    G_CALLBACK (month_view_button_clicked), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Tap here to select month-at-a-time view."), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);
  gtk_widget_show (GTK_WIDGET (item));
  GtkWidget *month_button = GTK_WIDGET (item);

  /* Initialize the upcoming view button.  */
  p = gpe_find_icon_scaled ("future_view",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  gtk_widget_show (GTK_WIDGET (pw));
  item = gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(item));
  gtk_tool_button_set_label (GTK_TOOL_BUTTON(item), _("Agenda"));
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON(item), pw);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item),
				     current_view_mode
				     == view_event_list_view);
  g_signal_connect (G_OBJECT (item), "toggled",
		    G_CALLBACK (event_list_button_clicked), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), 
			_("Tap here to select the agenda."), NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);
  gtk_widget_show (GTK_WIDGET (item));
  GtkWidget *event_list_button = GTK_WIDGET (item);

  if (window_x > 260)
    {	  
      item = gtk_separator_tool_item_new();
      gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
  
  /* Initialize the alarm button.  */
#ifndef IS_HILDON
  p = gpe_find_icon_scaled ("bell",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  gtk_widget_show (GTK_WIDGET (pw));
  item = gtk_tool_button_new (pw, _("Alarms"));
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (alarm_button_clicked), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), 
			_("Tap here to view alarms pending acknowledgement."),
			NULL);
  gtk_widget_show (GTK_WIDGET (item));
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);
  
  if (window_x > 260)
    {	  
      item = gtk_separator_tool_item_new();
      gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
#endif
  
  /* Initialize the "now" button.  */
  pw = gtk_image_new_from_stock (GTK_STOCK_HOME, 
                                 gtk_toolbar_get_icon_size
				 (GTK_TOOLBAR (toolbar)));
  gtk_widget_show (GTK_WIDGET (pw));
  item = gtk_tool_button_new (pw, _("Today"));
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(set_today), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET(item), 
			_("Switch to today."), NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);
  gtk_widget_show (GTK_WIDGET (item));
  today_button = GTK_WIDGET (item);


  item = gtk_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_widget_show (GTK_WIDGET (item));
  datesel_item = GTK_TOOL_ITEM (item);

  GDate date;
  g_date_set_time_t (&date, viewtime);
  datesel = GTK_DATE_SEL (gtk_date_sel_new (GTKDATESEL_FULL, &date));
  g_signal_connect (G_OBJECT (datesel), "changed",
		    G_CALLBACK (datesel_changed), NULL);
  gtk_widget_show (GTK_WIDGET (datesel));
  gtk_container_add (GTK_CONTAINER (item), GTK_WIDGET (datesel));

  main_toolbar_width = 20;
  void iter (GtkWidget *widget, gpointer data)
    {
      GtkRequisition req;
      gtk_widget_size_request (widget, &req);

      main_toolbar_width += req.width;
    }
  gtk_container_foreach (GTK_CONTAINER (main_toolbar), iter, NULL);


  gpe_set_window_icon (main_window, "icon");

  g_signal_connect (G_OBJECT (main_window), "key_press_event", 
		    G_CALLBACK (main_window_key_press_event), NULL);
            
  gtk_widget_add_events (GTK_WIDGET (main_window), 
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  GtkWidget *mitem;
  GtkMenuShell *menu;

  /* File menu.  */
  menu = GTK_MENU_SHELL (gtk_menu_new ());
  mitem = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (menu_main, GTK_WIDGET (mitem));
  gtk_widget_show (mitem);
    
  /* File -> New.  */
#ifdef IS_HILDON
  mitem = gtk_menu_item_new_with_label (_("New"));
#else
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
#endif  
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (new_appointment), NULL);
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);

  /* File -> Open.  */
#ifdef IS_HILDON
  mitem = gtk_menu_item_new_with_label (_("Import"));
#else
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, NULL);
#endif
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (import_callback), NULL);
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);

#ifndef IS_HILDON            
  mitem = gtk_separator_menu_item_new ();
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);
#endif

  /* File -> Minimize.  */
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE, NULL);
  gtk_menu_shell_append (menu, mitem);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (gpe_cal_iconify), NULL);
  gtk_widget_show (mitem);

  /* File -> Quit.  */
#ifdef IS_HILDON
  GtkWidget *quit_item = mitem = gtk_menu_item_new_with_label (_("Close"));
 #else
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  gtk_menu_shell_append (menu, mitem);
#endif
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (gpe_cal_exit), NULL);
  gtk_widget_show (mitem);


  /* View menu.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("_View"));
  menu = GTK_MENU_SHELL (gtk_menu_new ());
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (menu_main, mitem);
  gtk_widget_show (mitem);

  /* View -> Today.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("_Today"));
  pw = gtk_image_new_from_stock (GTK_STOCK_HOME, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (set_today), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Day.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("_Day"));
  p = gpe_find_icon_scaled ("day_view", GTK_ICON_SIZE_MENU);
  pw = gtk_image_new_from_pixbuf (p);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (view_toggled), day_button);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Week.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("_Week"));
  p = gpe_find_icon_scaled ("week_view", GTK_ICON_SIZE_MENU);
  pw = gtk_image_new_from_pixbuf (p);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (view_toggled), week_button);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Month.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("_Month"));
  p = gpe_find_icon_scaled ("month_view", GTK_ICON_SIZE_MENU);
  pw = gtk_image_new_from_pixbuf (p);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (view_toggled), month_button);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Agenda.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("_Agenda"));
  p = gpe_find_icon_scaled ("future_view", GTK_ICON_SIZE_MENU);
  pw = gtk_image_new_from_pixbuf (p);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (view_toggled), event_list_button);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Sidebar.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("_Sidebar"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mitem),
				  ! sidebar_disabled);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (sidebar_toggle), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Calendar.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("_Calendar"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mitem),
				  ! calendar_disabled);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (calendar_toggle), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Selector.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("Calendar _Selector"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mitem),
				  ! calendars_disabled);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (calendars_toggle), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* View -> Agenda.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("_Agenda"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mitem),
				  ! event_list_disabled);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (event_list_toggle), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* Tools menu.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("_Tools"));
  menu = GTK_MENU_SHELL (gtk_menu_new ());
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (menu_main, mitem);
  gtk_widget_show (mitem);

  /* Tools -> Calendars.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("_Calendars"));
  p = gpe_find_icon_scaled ("icon", GTK_ICON_SIZE_MENU);
  pw = gtk_image_new_from_pixbuf (p);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (calendars_button_clicked), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* Tools -> Categories.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("Cate_gories"));
#if 0
  pw = gtk_image_new_from_file (ICON_PATH "/qgn_list_gene_bullets.png");
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
#endif
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (edit_categories), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* Tools -> Alarms.  */
  mitem = gtk_image_menu_item_new_with_mnemonic (_("_Alarms"));
  p = gpe_find_icon_scaled ("bell", GTK_ICON_SIZE_MENU);
  pw = gtk_image_new_from_pixbuf (p);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mitem), pw);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (alarm_button_clicked), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* Tools -> Toolbar.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("_Toolbar"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mitem), TRUE);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (toggle_toolbar), toolbar);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

#ifdef IS_HILDON
  /* Tools -> Full Screen.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("_Full Screen"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (toggle_fullscreen), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);
  fullscreen_button = GTK_CHECK_MENU_ITEM (mitem);
  
  /* Finally attach close item. */
  gtk_menu_shell_append (menu_main, quit_item);
#endif

  /* The rest of the application window.  */
  if (display_tiny)
    {
      sidebar_hidden = TRUE;
      main_panel = GTK_CONTAINER (gtk_frame_new (NULL));
      gtk_frame_set_shadow_type (GTK_FRAME (main_panel), GTK_SHADOW_NONE);
    }
  else if (display_landscape)
    main_panel = GTK_CONTAINER (gtk_hpaned_new ());
  else
    main_panel = GTK_CONTAINER (gtk_vpaned_new ());

  gtk_box_pack_start (main_box, GTK_WIDGET (main_panel), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (main_panel));

  GtkWidget *f = gtk_frame_new (NULL);
  current_view_container = GTK_CONTAINER (f);
  gtk_frame_set_shadow_type (GTK_FRAME (f), GTK_SHADOW_NONE);
  if (GTK_IS_PANED (main_panel))
    gtk_paned_pack1 (GTK_PANED (main_panel), f, TRUE, FALSE);
  else
    gtk_container_add (main_panel, f);

  if (! sidebar_hidden)
    {
      f = gtk_frame_new (NULL);
      sidebar_container = GTK_CONTAINER (f);
      gtk_frame_set_shadow_type (GTK_FRAME (f), GTK_SHADOW_NONE);
      gtk_paned_pack2 (GTK_PANED (main_panel), f, FALSE, TRUE);
      sidebar_consider ();
    }
  
  current_view_consider ();

  gpe_calendar_start_xsettings (update_view);

  gtk_main ();

  return 0;
}
