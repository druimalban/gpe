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
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "util.h"

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

#ifndef HAVE_g_date_set_time_t
void         
g_date_set_time_t (GDate *date, time_t t)
{
  struct tm tm;
  localtime_r (&t, &tm);

  g_date_set_dmy (date, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}
#endif
