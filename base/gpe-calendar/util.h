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

#include <time.h>
#include <gtk/gtk.h>

#ifndef UTIL_H
#define UTIL_H
#include <libintl.h>

#define _(x) dgettext(PACKAGE, x)

#define CALENDAR_FILE_ "/.gpe/calendar"
#define CALENDAR_FILE() \
  g_strdup_printf ("%s" CALENDAR_FILE_, g_get_home_dir ())

extern GdkGC *pen_new (GtkWidget *widget, guint red, guint green, guint blue);

/* Call strftime() on the format and convert the result to UTF-8.  Any
   non-% expressions in the format must be in the locale's character
   set, since they will undergo UTF-8 conversion.  Careful with
   translations!  */
extern gchar *strftime_strdup_utf8_locale (const char *fmt, struct tm *tm);

/* As above but format string is UTF-8.  */
extern gchar *strftime_strdup_utf8_utf8 (const char *fmt, struct tm *tm);

extern void g_date_set_time_t (GDate *date, time_t t);

#define is_reminder(ev) \
  ({ int ret = 0; \
     if (event_get_untimed (ev)) \
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

#endif
