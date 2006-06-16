/*
 * Copyright (C) 2001, 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
 * Hildon adaption 2005 by Matthias Steinbauer <matthias@steinbauer.org>
 * Toolbar new API conversion 2005 by Florian Boor <florian@kernelconcepts.de>
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

#include <gpe/pim-categories-ui.h>
#include <gpe/event-db.h>

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
#include "import-vcal.h"
#include "export-vcal.h"
#include "gtkdatesel.h"
#include "event-cal.h"
#include "event-list.h"
#include "alarm-dialog.h"
#include "calendars-dialog.h"
#include "calendar-update.h"
#include "calendars-widgets.h"

#include <gpe/pim-categories.h>

#include <locale.h>

extern gboolean gpe_calendar_start_xsettings (void (*push_changes) (void));

#define CALENDAR_FILE_ "/.gpe/calendar"
#define CALENDAR_FILE \
  ({ \
    const char *home = g_get_home_dir (); \
    char *buffer = alloca (strlen (home) + strlen (CALENDAR_FILE_) + 1); \
    sprintf (buffer, "%s" CALENDAR_FILE_, home); \
    buffer; \
   })

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
static GtkEventList *event_list;

static GtkWidget *day_button;
#ifdef IS_HILDON
static GtkCheckMenuItem *fullscreen_button;
#endif

static void propagate_time (void);

guint week_offset;
gboolean week_starts_sunday;
gboolean day_view_combined_times;
const gchar *TIMEFMT;

/* Schedule atd to wake us up when the next alarm goes off.  */
static gboolean
schedule_wakeup (gboolean reload)
{
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

static guint reload_source;

static gboolean
hard_reload (gpointer data)
{
  if (calendar)
    gtk_event_cal_reload_events (calendar);
  if (event_list)
    gtk_event_list_reload_events (event_list);
  schedule_wakeup (TRUE);

  reload_source = 0;
  /* Don't run again.  */
  return FALSE;
}

static void
update_view (void)
{
  if (current_view)
    {
      if (GTK_IS_VIEW (current_view))
	gtk_view_reload_events (GTK_VIEW (current_view));
      else if (GTK_IS_EVENT_LIST (current_view))
	gtk_event_list_reload_events (GTK_EVENT_LIST (current_view));
    }
  if (! reload_source)
    reload_source = g_idle_add (hard_reload, 0);
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

static void
propagate_time (void)
{
  unsigned int day, month, year;
  struct tm tm;

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

  time_t ds;
  struct tm dstm;

  ds = gtk_date_sel_get_time (GTK_DATE_SEL (datesel));
  localtime_r (&ds, &dstm);

  if (tm.tm_year != dstm.tm_year || tm.tm_yday != dstm.tm_yday)
    gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);


  if (current_view)
    {
      time_t t = gtk_view_get_time (GTK_VIEW (current_view));
      if (t != viewtime)
	gtk_view_set_time (GTK_VIEW (current_view), viewtime);
    }
}

static gboolean
datesel_changed (GtkWidget *datesel, gpointer data)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (datesel));
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

      calendar = GTK_EVENT_CAL (gtk_event_cal_new ());
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

  GtkTreeModel *model = calendars_tree_model ();
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

      event_list = GTK_EVENT_LIST (gtk_event_list_new ());
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
	/* Already enabled, bye.  */
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
    {
      if (! current_view)
	return FALSE;

      gtk_container_remove (current_view_container, current_view);

      if (GTK_IS_EVENT_LIST (current_view))
	event_list_hidden --;

      return FALSE;
    }

  gtk_widget_show (GTK_WIDGET (current_view_container));

  if (current_view)
    {
      switch (current_view_mode)
	{
	case view_day_view:
	  if (GTK_IS_DAY_VIEW (current_view))
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
	  if (GTK_IS_EVENT_LIST (current_view))
	    return TRUE;
	  break;
	}

      if (GTK_IS_EVENT_LIST (current_view))
	{
	  event_list_hidden --;
	  event_list = GTK_EVENT_LIST (current_view);
	  gtk_widget_reparent (GTK_WIDGET (event_list),
			       GTK_WIDGET (event_list_container));
	}
      else
	gtk_container_remove (current_view_container, current_view);
    }

  switch (current_view_mode)
    {
    case view_day_view:
      current_view = gtk_day_view_new (viewtime);
      gtk_date_sel_set_mode (datesel, GTKDATESEL_FULL);
      gtk_widget_show (GTK_WIDGET (datesel));
      break;
    case view_week_view:
      current_view = gtk_week_view_new (viewtime);
      gtk_date_sel_set_mode (datesel, GTKDATESEL_WEEK);
      gtk_widget_show (GTK_WIDGET (datesel));
      break;
    case view_month_view:
      current_view = gtk_month_view_new (viewtime);
      gtk_date_sel_set_mode (datesel, GTKDATESEL_MONTH);
      gtk_widget_show (GTK_WIDGET (datesel));
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
	}
      else
	current_view = gtk_event_list_new ();
      gtk_widget_hide (GTK_WIDGET (datesel));
      break;
    }

  if (GTK_IS_VIEW (current_view))
    g_signal_connect (G_OBJECT (current_view),
		      "time-changed", G_CALLBACK (time_changed), NULL);

  gtk_container_add (current_view_container, current_view);
  gtk_widget_show (current_view);

  event_list_consider ();

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
  import_vcal (NULL, NULL);
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

  g_key_file_set_integer (conf, "gpe-calendar", "width-width",
			  main_window->allocation.width);
  g_key_file_set_integer (conf, "gpe-calendar", "width-height",
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
          import_vcal (NULL, NULL);
        break;	
        case GDK_t:
          set_today();
        break;	
        case GDK_q:
          gpe_cal_exit();
        break;	
      }
#endif /*IS_HILDON*/
  /* ignore if ctrl or alt pressed */    
  if ((k->state & GDK_CONTROL_MASK) 
       || (k->state & GDK_MOD1_MASK))
    return FALSE;
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
	  import_vcal (NULL, files);
	}
      else if (strcmp (var, "VIEWTIME") == 0 && value)
	{
	  time_t t = atoi (value);
	  if (t > 0)
	    viewtime = t;
	}
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
  gboolean schedule_only=FALSE;
  char *state = NULL;
  GSList *import_files = NULL;

  int option_letter;
  extern char *optarg;
  while ((option_letter = getopt (argc, argv, "s:e:i:")) != -1)
    {
      if (option_letter == 's')
	schedule_only = TRUE;
      else if (option_letter == 'i')
	{
	  if (! current_dir)
	    current_dir = g_get_current_dir ();

	  char *s
	    = g_strdup_printf ("%s%sIMPORT_FILE=%s%s%s",
			       state ?: "", state ? "\n" : "",
			       *optarg == '/' ? "" : current_dir,
			       *optarg == '/' ? "" : G_DIR_SEPARATOR_S,
			       optarg);
	  g_free (state);
	  state = s;

	  import_files = g_slist_append (import_files, optarg);
	}
    }

  if (current_dir)
    g_free (current_dir);

  if (! schedule_only)
    {
      char *s = g_strdup_printf ("%s%sFOCUS", state ?: "", state ? "\n" : "");
      g_free (state);
      state = s;
    }

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
  if (schedule_only)
    /* No instance running but called with -s.  */
    {
      event_db = event_db_new (CALENDAR_FILE);
      if (! event_db)
	{
	  g_critical ("Failed to open event database.");
	  exit (1);
	}

      schedule_wakeup (1);
      exit (EXIT_SUCCESS);
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
  event_db = event_db_new (CALENDAR_FILE);
  if (! event_db)
    {
      g_critical ("Failed to open event database.");
      exit (1);
    }
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
  GSList *i;
  for (i = import_files; i; i = i->next)
    {
      const char *files[] = { i->data, NULL };
      import_vcal (NULL, files);
    }
  g_slist_free (import_files);
	
  vcal_export_init();
    
#ifdef IS_HILDON
  /* Initialize maemo application */
  osso_context = osso_initialize(APPLICATION_DBUS_SERVICE, "0.1", TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL)
    return OSSO_ERROR;
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
  char *filename = CONF_FILE ();
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
  hildon_app_set_title (HILDON_APP (main_window), _("Calendar"));
  main_appview = hildon_appview_new (_("Main"));
  hildon_app_set_appview (HILDON_APP (main_window),
			  HILDON_APPVIEW (main_appview));
#else    
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), _("Calendar"));
  gtk_window_set_default_size (GTK_WINDOW (main_window), window_x, window_y);
#endif
  g_signal_connect (G_OBJECT (main_window), "delete-event",
                    G_CALLBACK (gpe_cal_exit), NULL);
  gtk_widget_show (main_window);

  GtkBox *win = GTK_BOX (gtk_vbox_new (FALSE, 0));
#if IS_HILDON
  gtk_container_add (GTK_CONTAINER (main_appview), GTK_WIDGET (win));
#else
  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (win));
#endif
  gtk_widget_show (GTK_WIDGET (win));

  /* Menu bar.  */
  GtkMenuShell *menu_main;
#ifdef IS_HILDON
  menu_main
    = GTK_MENU_SHELL (hildon_appview_get_menu (HILDON_APPVIEW (main_appview)));
#else
  menu_main = GTK_MENU_SHELL (gtk_menu_bar_new ());
  gtk_box_pack_start (win, GTK_WIDGET (menu_main), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (menu_main));
#endif

  /* Tool bar.  We fill it before the menu bar as the menu bar
     requires some of its widgets.  */
  tooltips = gtk_tooltips_new ();
  gtk_tooltips_enable (tooltips);

  GtkWidget *toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);

#ifdef IS_HILDON
  hildon_appview_set_toolbar (HILDON_APPVIEW (main_appview),
			      GTK_TOOLBAR (toolbar));
  gtk_widget_show_all (main_appview);
#else
  gtk_box_pack_start (GTK_BOX (win), toolbar, FALSE, FALSE, 0);
#endif

  /* Calendars button.  */
  p = gpe_find_icon_scaled ("icon", 
                            gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  GtkToolItem *item = gtk_tool_button_new (pw, _("Calendars"));
  g_signal_connect (G_OBJECT(item), "clicked",
		    G_CALLBACK(calendars_button_clicked), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), 
			_("Tap here to select the calendars to show."), NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);

  if (window_x > 260) 
    {	  
      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
    }

  /* Initialize the day view button.  */
  p = gpe_find_icon_scaled ("day_view", 
                            gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  item = gtk_radio_tool_button_new (NULL);
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
  day_button = GTK_WIDGET(item);    
    
  /* Initialize the week view button.  */
  p = gpe_find_icon_scaled ("week_view",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
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
  GtkWidget *week_button = GTK_WIDGET (item);    
  
  /* Initialize the month view button.  */
  p = gpe_find_icon_scaled ("month_view",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  item = gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(item));
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
  GtkWidget *month_button = GTK_WIDGET (item);

  /* Initialize the upcoming view button.  */
  p = gpe_find_icon_scaled ("future_view",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
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
  GtkWidget *event_list_button = GTK_WIDGET (item);

  if (window_x > 260)
    {	  
      item = gtk_separator_tool_item_new();
      gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
  
  /* Initialize the alarm button.  */
  p = gpe_find_icon_scaled ("bell",
			    gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  item = gtk_tool_button_new (pw, _("Alarms"));
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (alarm_button_clicked), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), 
			_("Tap here to view alarms pending acknowledgement."),
			NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);
  
  if (window_x > 260)
    {	  
      item = gtk_separator_tool_item_new();
      gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
  
  /* Initialize the "now" button.  */
  pw = gtk_image_new_from_stock (GTK_STOCK_HOME, 
                                 gtk_toolbar_get_icon_size
				 (GTK_TOOLBAR (toolbar)));
  item = gtk_tool_button_new (pw, _("Today"));
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(set_today), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET(item), 
			_("Switch to today."), NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);


  datesel = GTK_DATE_SEL (gtk_date_sel_new (GTKDATESEL_FULL, viewtime));
  g_signal_connect (G_OBJECT (datesel), "changed",
		    G_CALLBACK (datesel_changed), NULL);
  gtk_widget_show (GTK_WIDGET (datesel));
  
  GtkRequisition toolbar_req;
  gtk_widget_size_request (GTK_WIDGET (toolbar), &toolbar_req);
  GtkRequisition datesel_req;
  gtk_widget_size_request (GTK_WIDGET (toolbar), &datesel_req);

  if (toolbar_req.width + datesel_req.width + 20 < window_x)
    {
      item = gtk_tool_item_new ();
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

      gtk_container_add (GTK_CONTAINER (item), GTK_WIDGET (datesel));
    }
  else
    gtk_box_pack_start (GTK_BOX (win), GTK_WIDGET (datesel), FALSE, FALSE, 0);

  gpe_set_window_icon (main_window, "icon");

  g_signal_connect (G_OBJECT (main_window), "key_press_event", 
		    G_CALLBACK (main_window_key_press_event), NULL);
            
  gtk_widget_add_events (GTK_WIDGET (main_window), 
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  
#ifdef IS_HILDON
  gtk_widget_show(app);
  gtk_widget_show(main_appview);
#endif
  gtk_widget_show_all (toolbar);


  /* File menu.  */
  GtkWidget *mitem = gtk_menu_item_new_with_mnemonic (_("_File"));
  GtkMenuShell *menu = GTK_MENU_SHELL (gtk_menu_new ());
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (menu_main, GTK_WIDGET (mitem));
  gtk_widget_show (mitem);

  /* File -> New.  */
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (new_appointment), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* File -> Open.  */
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, NULL);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (import_callback), NULL);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* File -> Quit.  */
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (gpe_cal_exit), NULL);
  gtk_menu_shell_append (menu, mitem);
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

  gtk_box_pack_start (win, GTK_WIDGET (main_panel), TRUE, TRUE, 0);
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
      int d = display_landscape ? gdk_screen_width () : gdk_screen_height ();
      int w = MAX (150, d / 6);
      gtk_widget_set_size_request (f, display_landscape ? w : -1,
				   display_landscape ? -1 : w);
      gtk_paned_pack2 (GTK_PANED (main_panel), f, FALSE, TRUE);
      sidebar_consider ();
    }
  
  current_view_consider ();

  gpe_calendar_start_xsettings (update_view);

  gtk_main ();

  return 0;
}
