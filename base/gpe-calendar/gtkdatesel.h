/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GTK_DATE_SEL_H
#define GTK_DATE_SEL_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TYPE_DATE_SEL                  (gtk_date_sel_get_type ())
#define GTK_DATE_SEL(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_DATE_SEL, GtkDateSel))
#define GTK_DATE_SEL_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_DATE_SEL, GtkDateSelClass))
#define GTK_IS_DATE_SEL(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_DATE_SEL))
#define GTK_IS_DATE_SEL_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DATE_SEL))

typedef struct _GtkDateSel GtkDateSel;
typedef struct _GtkDateSelClass GtkDateSelClass;

typedef enum
  {
    GTKDATESEL_FULL,
    GTKDATESEL_WEEK,
    GTKDATESEL_YEAR,
    GTKDATESEL_MONTH
  } GtkDateSelMode;

typedef enum
  {
    GTKDATESEL_MONTH_LONG,              /* November */
    GTKDATESEL_MONTH_SHORT,             /* Nov */
    GTKDATESEL_MONTH_NUMERIC,           /* 11 */
    GTKDATESEL_MONTH_ROMAN              /* xi */
  } GtkDateSelMonthStyle;

GType           gtk_date_sel_get_type (void);

GtkWidget      *gtk_date_sel_new (GtkDateSelMode mode, time_t time);

extern void gtk_date_sel_set_mode (GtkDateSel *datesel, GtkDateSelMode mode);

time_t          gtk_date_sel_get_time (GtkDateSel *sel);
void            gtk_date_sel_set_time (GtkDateSel *sel, time_t time);
void            gtk_date_sel_set_month_style (GtkDateSel *sel, GtkDateSelMonthStyle style);
void            gtk_date_sel_set_day (GtkDateSel *sel, int year, int month, int day);

void		gtk_date_sel_move_day (GtkDateSel *sel, int d);
void		gtk_date_sel_move_week (GtkDateSel *sel, int d);
void		gtk_date_sel_move_month (GtkDateSel *sel, int d);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
