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

#include "util.h"

#ifdef IS_HILDON
#if HILDON_VER > 0
#include <hildon/hildon-program.h>
#else
#include <hildon-widgets/hildon-program.h>
#endif /* HILDON_VER  */
#endif

#define ERROR_DOMAIN() g_quark_from_static_string ("gpe-calendar")

extern EventDB *event_db;

extern gboolean week_starts_sunday;
extern guint week_offset;
extern gboolean day_view_combined_times;
extern void set_time_and_day_view (time_t selected_time);

#ifdef IS_HILDON
#if HILDON_VER > 0
#if MAEMO_VERSION_MAJOR < 5
  extern HildonWindow *main_window;
#else /* MAEMO_VERSION_MAJOR < 5 */
  extern HildonStackableWindow *main_window;
#endif /* MAEMO_VERSION_MAJOR < 5 */
#else
  extern GtkWidget *main_window;
#endif /* HILDON_VER  */
#else
  extern GtkWidget *main_window;
#endif


#define SECONDS_IN_DAY (24*60*60)

extern const gchar *TIMEFMT;

/* Maemo's stupid application installer uses a wrong prefix, 
 * so we define the icon files here. */
#ifdef IS_HILDON
  #define INST_PREFIX  "/usr/share/pixmaps/gpe/default/"
  #define DAY_ICON     INST_PREFIX "day_view.png"
  #define WEEK_ICON    INST_PREFIX "week_view.png"
  #define MONTH_ICON   INST_PREFIX "month_view.png"
  #define FUTURE_ICON   INST_PREFIX "future_view.png"
  #define BELL_ICON    INST_PREFIX "bell.png"
  #define RECUR_ICON   INST_PREFIX "recur.png"
  #define BELLRECUR_ICON INST_PREFIX "bell_recur.png"
#else
  #define DAY_ICON       "day_view"
  #define WEEK_ICON      "week_view"
  #define MONTH_ICON     "month_view"
  #define FUTURE_ICON    "future_view"
  #define BELL_ICON      "bell"
  #define RECUR_ICON     "recur"
  #define BELLRECUR_ICON "bell_recur"
#endif
#define APP_ICON       PREFIX "/share/pixmaps/gpe-calendar.png"

#ifndef PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING
#define PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING 0
#endif
#ifndef PANGO_ATTR_INDEX_TO_TEXT_END
#include <limits.h>
#define PANGO_ATTR_INDEX_TO_TEXT_END INT_MAX
#endif
