/*
 * Copyright (C) 2001, 2002, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <time.h>
#include <gpe/event-db.h>
#include <libintl.h>

#define _(x) dgettext(PACKAGE, x)

extern EventDB *event_db;

extern gboolean week_starts_sunday;
extern guint week_offset;
extern gboolean day_view_combined_times;
extern void set_time_and_day_view (time_t selected_time);

extern GdkGC *pen_new (GtkWidget *widget, guint red, guint green, guint blue);

extern GtkWidget *main_window;

extern gchar *strftime_strdup_utf8_locale (const char *fmt, struct tm *tm);
extern gchar *strftime_strdup_utf8_utf8 (const char *fmt, struct tm *tm);

#define SECONDS_IN_DAY (24*60*60)

extern const gchar *TIMEFMT;

/* Maemo's stupid application installer uses a wrong prefix, 
 * so we define the icon files here. */
#ifdef IS_HILDON
  #define INST_PREFIX  "/usr/share/gpe/pixmaps/default/"
  #define DAY_ICON     INST_PREFIX "day_view.png"
  #define WEEK_ICON    INST_PREFIX "week_view.png"
  #define MONTH_ICON   INST_PREFIX "month_view.png"
  #define FUTURE_ICON   INST_PREFIX "future_view.png"
  #define BELL_ICON    INST_PREFIX "bell.png"
  #define RECUR_ICON   INST_PREFIX "recur.png"
  #define BELLRECUR_ICON INST_PREFIX "bell_recur.png"
  #define APP_ICON     "/usr/share/pixmaps/gpe-calendar.png"
#else
  #define DAY_ICON       "day_view"
  #define WEEK_ICON      "week_view"
  #define MONTH_ICON     "month_view"
  #define FUTURE_ICON    "future_view"
  #define BELL_ICON      "bell"
  #define RECUR_ICON     "recur"
  #define BELLRECUR_ICON "bell_recur"
  #define APP_ICON       PREFIX "/share/pixmaps/gpe-calendar.png"
#endif

#define is_reminder(ev) \
  ({ int ret = 0; \
     if (event_get_duration (ev) == 0 && event_get_untimed (ev)) \
       ret = 1; \
     else if (event_get_duration (ev) == 24 * 60 * 60) \
       { \
         time_t s = event_get_start (ev); \
         struct tm tm; \
         localtime_r (&s, &tm); \
         ret = tm.tm_hour == 0 && tm.tm_min == 0 && tm.tm_sec == 0; \
       } \
     ret; \
    })
