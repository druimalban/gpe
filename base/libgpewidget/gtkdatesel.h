/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GTK_DATE_SEL_H
#define GTK_DATE_SEL_H

#include <gtk/gtk.h>

#define GTK_TYPE_DATE_SEL                  (gtk_date_sel_get_type ())
#define GTK_DATE_SEL(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_DATE_SEL, GtkDateSel))
#define GTK_DATE_SEL_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_DATE_SEL, GtkDateSelClass))
#define GTK_IS_DATE_SEL(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_DATE_SEL))
#define GTK_IS_DATE_SEL_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DATE_SEL))

typedef struct _GtkDateSel	   GtkDateSel;
typedef struct _GtkDateSelClass  GtkDateSelClass;

typedef enum 
  {
    GTKDATESEL_FULL,
    GTKDATESEL_WEEK,
    GTKDATESEL_YEAR,
    GTKDATESEL_MONTH
  } GtkDateSelMode;

GtkType		gtk_date_sel_get_type	   (void);

GtkWidget      *gtk_date_sel_new (GtkDateSelMode mode);

time_t		gtk_date_sel_get_time	   (GtkDateSel *sel);
void            gtk_date_sel_set_time      (GtkDateSel *sel, time_t time);

#endif
