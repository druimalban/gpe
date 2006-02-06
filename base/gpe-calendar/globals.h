/*
 * Copyright (C) 2001, 2002, 2006 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <time.h>
#include <gpe/event-db.h>

extern time_t viewtime;
extern gboolean force_today;

extern void update_current_view (void);
extern void update_all_views (void);

extern guint days_in_month (guint year, guint month);
extern gboolean week_starts_monday;
extern guint week_offset;
extern gboolean day_view_combined_times;
extern void set_time_and_day_view(time_t selected_time);
extern void set_time_all_views(void);
extern time_t time_from_day(int year, int month, int day);


extern GdkPixmap *close_pix, *day_pix;
extern GdkBitmap *close_mask, *day_mask;

extern GtkWidget *main_window, *pop_window;

extern GdkFont *yearfont, *datefont;
extern gchar *strftime_strdup_utf8_locale (const char *fmt, struct tm *tm);
extern gchar *strftime_strdup_utf8_utf8 (const char *fmt, struct tm *tm);
extern guint day_of_week(guint year, guint month, guint day);

#define SECONDS_IN_DAY (24*60*60)

/* stuff that could perhaps be configurable */
#define TIMEFMT "%R"
#define MONTHTIMEFMT "%a %d"

/* Maemo's stupid application installer uses a wrong prefix, 
 * so we define the icon files here. */
#ifdef IS_HILDON
  #define INST_PREFIX  "/var/lib/install/usr/share/gpe/pixmaps/default/"
  #define DAY_ICON     INST_PREFIX "day_view.png"
  #define WEEK_ICON    INST_PREFIX "week_view.png"
  #define MONTH_ICON   INST_PREFIX "month_view.png"
  #define BELL_ICON    INST_PREFIX "bell.png"
  #define RECUR_ICON   INST_PREFIX "recur.png"
  #define BELLRECUR_ICON INST_PREFIX "bell_recur.png"
  #define APP_ICON     "/var/lib/install/usr/share/pixmaps/gpe-calendar.png"
#else
  #define DAY_ICON       "day_view"
  #define WEEK_ICON      "week_view"
  #define MONTH_ICON     "month_view"
  #define BELL_ICON      "bell"
  #define RECUR_ICON     "recur"
  #define BELLRECUR_ICON "bell_recur"
  #define APP_ICON       PREFIX "/share/pixmaps/gpe-calendar.png"
#endif

#define get_cloned_ev(arg) \
  ((arg->flags & FLAG_CLONE) ? (event_t)arg->cloned_ev : arg)

#define is_reminder(ev) (ev->duration == 0 && (ev->flags & FLAG_UNTIMED))
