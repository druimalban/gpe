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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

#include <gpe/event-db.h>
#include <gpe/schedule.h>

#include <handoff.h>

#ifdef IS_HILDON
/* Hildon includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <gpe/pim-categories-ui.h>
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
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

#include <gpe/pim-categories.h>

#include <locale.h>

#define _(_x) gettext (_x)

extern gboolean gpe_calendar_start_xsettings (void);

#define CALENDAR_FILE_ "/.gpe/calendar"
#define CALENDAR_FILE \
  ({ \
    const char *home = g_get_home_dir (); \
    char *buffer = alloca (strlen (home) + strlen (CALENDAR_FILE_) + 1); \
    sprintf (buffer, "%s" CALENDAR_FILE_, home); \
    buffer; \
   })

/* Absolute path to the executable.  */
static const char *gpe_calendar;

EventDB *event_db;

time_t viewtime;
gboolean just_new = FALSE;

GtkWidget *main_window, *pop_window;
GtkWidget *view_container, *toolbar;
extern GtkWidget* day_list;

struct gpe_icon my_icons[] = {
  { "day_view",   DAY_ICON },
  { "week_view",  WEEK_ICON },
  { "month_view", MONTH_ICON },
  { "bell",        BELL_ICON },
  { "recur",       RECUR_ICON },
  { "bell_recur", BELLRECUR_ICON },
  { "icon", APP_ICON },
  { NULL, NULL }
};

static GtkWidget *current_view;
static GtkDateSel *datesel;
static GtkEventCal *calendar;
static GtkEventList *event_list;
static GtkWidget *day_button, *week_button, *month_button, *alarm_button;

static void propagate_time (void);

guint window_x = 240, window_y = 310;

guint week_offset = 0;
gboolean week_starts_monday = TRUE;
gboolean day_view_combined_times;
gchar collected_keys[32] = "";
gint ccpos = 0;
const gchar *TIMEFMT;

static guint nr_days[] = { 31, 28, 31, 30, 31, 30, 
			   31, 31, 30, 31, 30, 31 };

guint
day_of_week (guint year, guint month, guint day)
{
  guint result;

  if (month < 3) 
    {
      month += 12;
      --year;
    }

  result = day + (13 * month - 27)/5 + year + year/4
    - year/100 + year/400;
  return ((result + 6) % 7);
}

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

time_t
time_from_day (int year, int month, int day)
{
  struct tm tm;
  time_t selected_time;
  localtime_r (&viewtime, &tm);
  tm.tm_year = year;
  tm.tm_mon = month;
  tm.tm_mday = day;
  selected_time = mktime (&tm);
  return selected_time;
}

GdkGC *
pen_new (GtkWidget * widget, guint red, guint green, guint blue)
{
  GdkColormap *colormap;
  GdkGC *pen_color_gc;
  GdkColor pen_color;

  colormap = gdk_window_get_colormap (widget->window);
  pen_color_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (pen_color_gc, widget->style->black_gc);
  pen_color.red = red;
  pen_color.green = green;
  pen_color.blue = blue;
  gdk_colormap_alloc_color (colormap, &pen_color, FALSE, TRUE);
  gdk_gc_set_foreground (pen_color_gc, &pen_color);

  return pen_color_gc;
}

/* Call strftime() on the format and convert the result to UTF-8.  Any
   non-% expressions in the format must be in the locale's character
   set, since they will undergo UTF-8 conversion.  Careful with
   translations!  */
gchar *
strftime_strdup_utf8_locale (const char *fmt, struct tm *tm)
{
  char buf[1024];
  size_t n;

  buf[0] = '\001';
  n = strftime (buf, sizeof (buf), fmt, tm);
  if (n == 0 && buf[0] == '\001')
    return NULL;		/* Something went wrong */

  return g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
}

/* As above but format string is UTF-8.  */
gchar *
strftime_strdup_utf8_utf8 (const char *fmt, struct tm *tm)
{
  gchar *sfmt, *sval;

  sfmt = g_locale_from_utf8 (fmt, -1, NULL, NULL, NULL);
  if (sfmt == NULL)
    return NULL;		/* Conversion failed */
  sval = strftime_strdup_utf8_locale (sfmt, tm);
  g_free (sfmt);

  return sval;
}

/* Schedule atd to wake us up when the next alarm goes off.  */
static gboolean
schedule_wakeup (gboolean reload)
{
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

  return FALSE;
}

void
update_view (void)
{
  if (current_view)
    gtk_view_reload_events (GTK_VIEW (current_view));
  if (calendar)
    gtk_event_cal_reload_events (calendar);
  if (event_list)
    gtk_event_list_reload_events (event_list);
  schedule_wakeup (TRUE);
}

static gboolean
do_reset_new(gpointer d)
{
  just_new = FALSE;
  return FALSE;
}


static gboolean
do_insert_text (GtkWidget *window)
{
  GtkWidget *entry;
  
  entry = g_object_get_data(G_OBJECT(window), "default-entry");
  if (entry)
    {
      gtk_entry_prepend_text(GTK_ENTRY(entry), collected_keys);
      gtk_editable_set_position(GTK_EDITABLE(entry),-1);
      memset(collected_keys, 0, sizeof(gchar) * 32);
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
  
  appt = new_event (viewtime, 0);
  g_timeout_add(500, (GSourceFunc)(do_insert_text), (gpointer)appt);
  gtk_widget_show (appt);
}

static void
set_today (void)
{
  static time_t saved_time;

  if (saved_time)
    {
      time_t now;
      struct tm ntm;
      struct tm vtm;

      time (&now);
      localtime_r (&now, &ntm);
      localtime_r (&viewtime, &vtm);

      if (ntm.tm_year != vtm.tm_year || ntm.tm_yday != vtm.tm_yday)
	/* Current view time is not today.  Go to today.  */
	{
	  saved_time = viewtime;
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

static void
time_changed (GtkView *view, time_t time, gpointer data)
{
  viewtime = time;
  propagate_time ();
}

static void
new_view (GtkWidget * (*new) (time_t time))
{
  if (pop_window)
    gtk_widget_destroy (pop_window);
  pop_window = NULL;

  if (current_view)
    gtk_container_remove (GTK_CONTAINER (view_container), current_view);

  current_view = new (viewtime);
  gtk_box_pack_start (GTK_BOX (view_container), current_view,
		      TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (current_view),
		    "time-changed", G_CALLBACK (time_changed), NULL);
  gtk_widget_show (current_view);
}

void
set_time_and_day_view (time_t selected_time)
{
  viewtime = selected_time;
  propagate_time ();
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (day_button), TRUE);
  new_view (gtk_day_view_new);
}

static void
day_view_button_clicked (GtkWidget *widget, gpointer d)
{
  if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    return;

  gtk_date_sel_set_mode (datesel, GTKDATESEL_FULL);
  if (calendar)
    gtk_widget_show (GTK_WIDGET (calendar));
  new_view (gtk_day_view_new);
}

static void
week_view_button_clicked (GtkWidget *widget, gpointer d)
{
  if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    return;

  gtk_date_sel_set_mode (datesel, GTKDATESEL_WEEK);
  if (calendar)
    gtk_widget_show (GTK_WIDGET (calendar));
  new_view (gtk_week_view_new);
}

static void
month_view_button_clicked (GtkWidget *widget, gpointer d)
{
  if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
    return;

  gtk_date_sel_set_mode (datesel, GTKDATESEL_MONTH);
  if (calendar)
    gtk_widget_hide (GTK_WIDGET (calendar));
  new_view (gtk_month_view_new);
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
}

#ifdef IS_HILDON
static void
menu_toggled (GtkWidget *widget, gpointer data)
{
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (data), TRUE);
}
#endif

static void
gpe_cal_exit (void)
{
  g_object_unref (event_db);
  gtk_main_quit ();
}

#ifdef IS_HILDON
static void
gpe_cal_fullscreen_toggle (void)
{
  static int fullscreen_toggle = TRUE;
  hildon_appview_set_fullscreen(HILDON_APPVIEW(main_window), fullscreen_toggle);
  fullscreen_toggle = !fullscreen_toggle;
}

static void
toggle_toolbar(GtkCheckMenuItem *menuitem, gpointer user_data)
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

  dialog = gpe_pim_categories_dialog (NULL, FALSE, NULL, NULL);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), 
                               GTK_WINDOW(gtk_widget_get_toplevel(w)));
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
}

#endif /*IS_HILDON*/

static int 
import_one_file(gchar *filename)
{
  int result = 0;
    
  result = import_vcal (filename);
    
  update_view ();

  return result;
}

static void
on_import_vcal (GtkWidget *widget, gpointer data)
{
  GtkWidget *filesel, *feedbackdlg;
  
#if IS_HILDON	
  filesel = hildon_file_chooser_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)), 
                                                      GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(filesel), TRUE);
#else
  filesel = gtk_file_selection_new(_("Choose file"));
  gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(filesel),TRUE);
#endif	
  if (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_OK)
    {
      gchar *errstr = NULL;
      int ec = 0, i = 0;
#ifdef IS_HILDON
      gchar **files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(filesel));
#else		
      gchar **files = 
        gtk_file_selection_get_selections(GTK_FILE_SELECTION(filesel));
#endif
      gtk_widget_hide(filesel); 
      while (files[i])
        {
          if (import_one_file(files[i]) < 0) 
            {
              gchar *tmp;
              if (!errstr) 
                errstr=g_strdup("");
              ec++;
			  tmp = g_strdup_printf("%s\n%s",errstr,strrchr(files[i],'/')+1);
              if (errstr) 
                 g_free(errstr);
              errstr = tmp;
            }
          i++;  
        }
      if (ec)
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(main_window)),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          "%s %i %s\n%s",_("Import of"),ec,_("files failed:"),errstr);
      else
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(main_window)),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          _("Import successful"));
      gtk_dialog_run(GTK_DIALOG(feedbackdlg));
      gtk_widget_destroy(feedbackdlg);
    }
  gtk_widget_destroy(filesel);

  update_view ();  
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
        gpe_cal_fullscreen_toggle();
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
          on_import_vcal(widget, NULL);
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
        if (ccpos < 31) 
          collected_keys[ccpos] = k->string[0];
        ccpos++;
        return TRUE;
    }
    
  return FALSE;
}

#ifdef IS_HILDON
static void
create_app_menu(HildonAppView *appview)
{
  GtkMenu *menu_main = hildon_appview_get_menu(appview);
  GtkWidget *menu_event = gtk_menu_new();
  GtkWidget *menu_categories = gtk_menu_new();
  GtkWidget *menu_view = gtk_menu_new();
  GtkWidget *menu_tools = gtk_menu_new();
    
  GtkWidget *item_event = gtk_menu_item_new_with_label(_("Event"));
  GtkWidget *item_categories = gtk_menu_item_new_with_label(_("Categories"));
  GtkWidget *item_view = gtk_menu_item_new_with_label(_("View"));
  GtkWidget *item_close = gtk_menu_item_new_with_label(_("Close"));
  GtkWidget *item_add = gtk_menu_item_new_with_label(_("Add new"));
  GtkWidget *item_delete = gtk_menu_item_new_with_label(_("Delete"));
  GtkWidget *item_catedit = gtk_menu_item_new_with_label(_("Edit categories"));
  GtkWidget *item_toolbar = gtk_check_menu_item_new_with_label(_("Show toolbar"));
  GtkWidget *item_tools = gtk_menu_item_new_with_label(_("Tools"));
  GtkWidget *item_import = gtk_menu_item_new_with_label(_("Import vCal / ICS"));
  GtkWidget *item_today = gtk_menu_item_new_with_label(_("Today"));
  GtkWidget *item_day = gtk_menu_item_new_with_label(_("Day"));
  GtkWidget *item_week = gtk_menu_item_new_with_label(_("Week"));
  GtkWidget *item_month = gtk_menu_item_new_with_label(_("Month"));
  GtkWidget *item_sep = gtk_separator_menu_item_new();
  GtkWidget *item_alarms = gtk_menu_item_new_with_label(_("Alarms"));

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_event), menu_event);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_categories), menu_categories);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_view), menu_view);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_tools), menu_tools);

  gtk_menu_append(menu_main, item_event);
  gtk_menu_append(menu_main, item_categories);
  gtk_menu_append(menu_main, item_view);
  gtk_menu_append(menu_main, item_tools);
  gtk_menu_append(menu_main, item_close);
  
  gtk_menu_append(menu_event, item_add);
  gtk_menu_append(menu_view, item_today);
  gtk_menu_append(menu_tools, item_import);
  gtk_menu_append(menu_view, item_day);
  gtk_menu_append(menu_view, item_week);
  gtk_menu_append(menu_view, item_month);
  gtk_menu_append(menu_view, item_sep);
  gtk_menu_append(menu_view, item_alarms);
  gtk_menu_append(menu_view, item_toolbar);
  gtk_menu_append(menu_categories, item_catedit);
  
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_toolbar), TRUE);
  
  g_signal_connect(G_OBJECT(item_add), "activate", G_CALLBACK(new_appointment), NULL);
  g_signal_connect(G_OBJECT(item_today), "activate", G_CALLBACK(set_today), NULL);
  g_signal_connect(G_OBJECT(item_import), "activate", G_CALLBACK(on_import_vcal), NULL);
  g_signal_connect(G_OBJECT(item_day), "activate", G_CALLBACK(menu_toggled), day_button);
  g_signal_connect(G_OBJECT(item_week), "activate", G_CALLBACK(menu_toggled), week_button);
  g_signal_connect(G_OBJECT(item_month), "activate", G_CALLBACK(menu_toggled), month_button);
  g_signal_connect(G_OBJECT(item_alarms), "activate", G_CALLBACK(alarm_button_clicked), NULL);
  g_signal_connect(G_OBJECT(item_toolbar), "activate", G_CALLBACK(toggle_toolbar), NULL);
  g_signal_connect(G_OBJECT(item_catedit), "activate", G_CALLBACK(edit_categories), NULL);
  g_signal_connect(G_OBJECT(item_close), "activate", G_CALLBACK(gpe_cal_exit), NULL);

  gtk_widget_show_all(GTK_WIDGET(menu_main));
}
#endif

static void
import_file (char *ifile)
{
  GtkWidget *dialog;
  if (import_one_file (ifile))
    dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_INFO,
				     GTK_BUTTONS_OK,
				     _("Could not import file %s."),
				     ifile);
  else
    dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_INFO,
				     GTK_BUTTONS_OK,
				     _("File %s imported sucessfully."),
				     ifile);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
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
	import_file (value);
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
  return g_strdup_printf ("VIEWTIME=%ld\n", viewtime);
}

int
main (int argc, char *argv[])
{
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkToolItem *item;
  GtkTooltips *tooltips;    
#ifdef IS_HILDON
  GtkWidget *app, *main_appview;
  osso_context_t *osso_context;
#endif

  if (g_path_is_absolute (argv[0]))
    gpe_calendar = argv[0];
  else
    gpe_calendar = g_build_filename (g_get_current_dir (), argv[0], NULL);

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
	  char *s = g_strdup_printf ("%s%sIMPORT_FILE=%s",
				     state ?: "", state ? "\n" : "", optarg);
	  g_free (state);
	  state = s;

	  import_files = g_slist_append (import_files, optarg);
	}
    }

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

  if (gpe_pim_categories_init () == FALSE)
    exit (1);

  /* Import any files specified on the command line.  */
  GSList *i;
  for (i = import_files; i; i = i->next)
    import_file (i->data);
  g_slist_free (import_files);
	
  vcal_export_init();
    
#ifdef IS_HILDON
  /* Initialize maemo application */
  osso_context = osso_initialize(APPLICATION_DBUS_SERVICE, "0.1", TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL)
  {
    return OSSO_ERROR;
  }
#endif

  /* If a display migration occurred, this may already be set.  */
  if (! viewtime)
    time (&viewtime);

  /* Build the main window.  */
  window_x = CLAMP (gdk_screen_width () * 7 / 8, 240, 1000);
  window_y = CLAMP (gdk_screen_height () * 7 / 8, 310, 800);
  int tiny = gdk_screen_width () < 300;
  int landscape = gdk_screen_width () > gdk_screen_height ()
    && gdk_screen_width () >= 640;

  /*   --- Toolbar ----
     +-Datesel--------------------------------+
     +----------------------------------------+
     +-primary--------------------------------+
     |+-viewcontainer----------++-sidebar----+|
     ||+-current_view---------+||+-Calendar-+||
     |||                      ||||          |||
     |||                      |||+----------+||
     |||                      |||+Event-List+||
     |||                      ||||          |||
     |||                      ||||          |||
     ||+----------------------+||+----------+||
     |+------------------------++------------+|
     +----------------------------------------+
  */


#ifdef IS_HILDON
  app = hildon_app_new();
  hildon_app_set_two_part_title(HILDON_APP(app), FALSE);
  hildon_app_set_title(HILDON_APP(app), _("Calendar"));
  main_appview = hildon_appview_new(_("Main"));
  hildon_app_set_appview(HILDON_APP(app), HILDON_APPVIEW(main_appview));
  main_window = main_appview;
#else    
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), _("Calendar"));
  g_signal_connect (G_OBJECT (main_window), "delete-event",
                    G_CALLBACK (gpe_cal_exit), NULL);
  gtk_window_set_default_size (GTK_WINDOW (main_window), window_x, window_y);

  gtk_widget_realize (main_window);
#endif
  gtk_widget_show (main_window);

  GtkBox *win = GTK_BOX (gtk_vbox_new (FALSE, 0));
  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (win));
  gtk_widget_show (GTK_WIDGET (win));


  tooltips = gtk_tooltips_new ();
  gtk_tooltips_enable (tooltips);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);

#ifdef IS_HILDON
  hildon_appview_set_toolbar (HILDON_APPVIEW (main_appview),
			      GTK_TOOLBAR (toolbar));
  gtk_widget_show_all (main_appview);
#else
  gtk_box_pack_start (GTK_BOX (win), toolbar, FALSE, FALSE, 0);
#endif

  /* Initialize new event button.  */
  item = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK(new_appointment), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET(item), 
			_("Tap here to add a new appointment or reminder"),
			NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);

  if (window_x > 260)
    {	  
      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR(toolbar), item, -1);
    }

  /* Initialize the "now" button.  */
  pw = gtk_image_new_from_stock (GTK_STOCK_HOME, 
                                 gtk_toolbar_get_icon_size
				 (GTK_TOOLBAR (toolbar)));
  item = gtk_tool_button_new (pw, _("Today"));
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(set_today), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET(item), 
			_("Switch to today and stay there/return to day selecting."), NULL);
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
  item = gtk_radio_tool_button_new(NULL);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Day"));
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), pw);
  g_signal_connect(G_OBJECT(item), "clicked",
		   G_CALLBACK (day_view_button_clicked), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Tap here to select day-at-a-time view."), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);
  day_button = GTK_WIDGET(item);    
    
  /* Initialize the week view button.  */
  p = gpe_find_icon_scaled ("week_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  item = gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Week"));
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), pw);
  g_signal_connect(G_OBJECT(item), "clicked",
		   G_CALLBACK (week_view_button_clicked), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Tap here to select week-at-a-time view."), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);
  week_button = GTK_WIDGET(item);    
  
  /* Initialize the month view button.  */
  p = gpe_find_icon_scaled ("month_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  item = gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Month"));
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), pw);
  g_signal_connect (G_OBJECT(item), "clicked",
		    G_CALLBACK (month_view_button_clicked), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Tap here to select month-at-a-time view."), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);
  month_button = GTK_WIDGET (item);

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
  alarm_button = GTK_WIDGET (item);
  
  if (window_x > 260)
    {	  
      item = gtk_separator_tool_item_new();
      gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
  
  pw = gtk_image_new_from_stock(GTK_STOCK_OPEN, 
                                gtk_toolbar_get_icon_size(GTK_TOOLBAR (toolbar)));
  item = gtk_tool_button_new(pw, _("Import"));
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(on_import_vcal), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Open file to import an event from it."), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);

#ifdef IS_HILDON
  pw = gtk_image_new_from_file(ICON_PATH "/qgn_list_gene_bullets.png");
  item = gtk_tool_button_new(pw, _("Categories"));
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK (edit_categories), 
                       NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
#else
  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_tool_item_set_expand(item, TRUE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  item = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(gpe_cal_exit), NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
                       _("Tap here to exit the program"), NULL);
  GTK_WIDGET_UNSET_FLAGS(item, GTK_CAN_FOCUS);
#endif

/* hildon has its special menu, GPE a window icon */
#ifdef IS_HILDON
  create_app_menu(HILDON_APPVIEW(main_appview));
#else
  gpe_set_window_icon (main_window, "icon");
#endif
	 
  g_signal_connect (G_OBJECT (main_window), "key_press_event", 
		    G_CALLBACK (main_window_key_press_event), NULL);
            
  gtk_widget_add_events (GTK_WIDGET (main_window), 
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  
#ifdef IS_HILDON
  gtk_widget_show(app);
  gtk_widget_show(main_appview);
#endif
  gtk_widget_show_all (toolbar);


  datesel = GTK_DATE_SEL (gtk_date_sel_new (GTKDATESEL_FULL, viewtime));

  gtk_box_pack_start (win, GTK_WIDGET (datesel), FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (datesel), "changed",
		    G_CALLBACK (datesel_changed), NULL);
  gtk_widget_add_events (GTK_WIDGET (datesel),
			 GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  gtk_widget_show (GTK_WIDGET (datesel));

  GtkPaned *primary
    = GTK_PANED ((landscape ? &gtk_hpaned_new : &gtk_vpaned_new) ());
  gtk_box_pack_start (win, GTK_WIDGET (primary), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (primary));

  view_container = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack1 (primary, GTK_WIDGET (view_container), TRUE, FALSE);
  gtk_widget_show (GTK_WIDGET (view_container));

  if (! tiny)
    {
      GtkWidget *sidebar;

      if (landscape)
	sidebar = gtk_vbox_new (FALSE, 0);
      else
	sidebar = gtk_hpaned_new ();

      gtk_paned_pack2 (primary, GTK_WIDGET (sidebar), FALSE, TRUE);
      gtk_widget_show (GTK_WIDGET (sidebar));

      calendar = GTK_EVENT_CAL (gtk_event_cal_new ());
      GTK_WIDGET_UNSET_FLAGS (calendar, GTK_CAN_FOCUS);
      gtk_calendar_set_display_options (GTK_CALENDAR (calendar),
					GTK_CALENDAR_SHOW_DAY_NAMES
					| (week_starts_monday ?
					   GTK_CALENDAR_WEEK_START_MONDAY : 0));
      if (landscape)
	gtk_box_pack_start (GTK_BOX (sidebar), GTK_WIDGET (calendar),
			    FALSE, FALSE, 0);
      else
	gtk_paned_pack1 (GTK_PANED (sidebar), GTK_WIDGET (calendar),
			 FALSE, TRUE);

      g_signal_connect (G_OBJECT (calendar),
			"day-selected", G_CALLBACK (calendar_changed), NULL);
      gtk_widget_show (GTK_WIDGET (calendar));
      
      event_list = GTK_EVENT_LIST (gtk_event_list_new ());
      if (landscape)
	gtk_box_pack_start (GTK_BOX (sidebar), GTK_WIDGET (event_list),
			    TRUE, TRUE, 0);
      else
	gtk_paned_pack2 (GTK_PANED (sidebar), GTK_WIDGET (event_list),
			 TRUE, FALSE);
      gtk_widget_set_size_request (GTK_WIDGET (event_list), 150, 150);
      gtk_widget_show (GTK_WIDGET (event_list));
    }

  gpe_calendar_start_xsettings ();

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (day_button), TRUE);
  new_view (gtk_day_view_new);
  gtk_day_view_scroll (GTK_DAY_VIEW (current_view), TRUE);

  gtk_main ();

  return 0;
}
