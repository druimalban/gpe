/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
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

#include <time.h>
#include <gtk/gtk.h>
#include "gtkdatesel.h"
#include <libintl.h>

#define _(x) dgettext(PACKAGE, x)

static guint my_signals[1];

struct elem
{
  GtkWidget *arrow_l;
  GtkWidget *text;
  GtkWidget *arrow_r;
};

struct _GtkDateSel
{
  GtkHBox hbox;
  GtkWidget *subbox;

  struct elem day, week, month, year;

  guint day_width, month_width, year_width, week_width;
  gint day_clamped;

  time_t time;

  GtkDateSelMode mode;
  GtkDateSelMonthStyle month_style;
};

struct _GtkDateSelClass
{
  GtkHBoxClass parent_class;
  void (* changed)      (GtkDateSel *sel);
};

static GtkHBoxClass *parent_class = NULL;

static int
days_in_month (struct tm *tm)
{
  if (tm->tm_mon == 1)
    {
      guint y = 1900 + tm->tm_year;
      if ((y % 4) == 0
          && (((y % 100) != 0) || (y % 400) == 0))
        return 29;
      else
        return 28;
    }

  switch (tm->tm_mon)
    {
    case 3:
    case 5:
    case 8:
    case 10:
      return 30;
    default:
      return 31;
    }
}

void
gtk_date_sel_move_day (GtkDateSel *sel, int d)
{
  sel->time += d * 60 * 60 * 24;
  sel->day_clamped = -1;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
day_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  gtk_date_sel_move_day (sel, d);
}

void
gtk_date_sel_move_week (GtkDateSel *sel, int d)
{
  sel->time += d * 60 * 60 * 24 * 7;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
week_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  gtk_date_sel_move_week (sel, d);
}

void
gtk_date_sel_move_month (GtkDateSel *sel, int d)
{
  struct tm tm;
  guint mdays;
  localtime_r (&sel->time, &tm);
  if (d < 0)
    {
      if (tm.tm_mon)
        tm.tm_mon--;
      else
        {
          tm.tm_year--;
          tm.tm_mon = 11;
        }
    }
  else
    {
      if (tm.tm_mon < 11)
        tm.tm_mon++;
      else
        {
          tm.tm_year++;
          tm.tm_mon = 0;
        }
    }
  mdays = days_in_month (&tm);
  if (tm.tm_mday > mdays)
    {
      sel->day_clamped = tm.tm_mday;
      tm.tm_mday = mdays;
    }
  else
    {
      if (sel->day_clamped != -1)
        {
          tm.tm_mday = mdays;
          if (tm.tm_mday > sel->day_clamped)
            tm.tm_mday = sel->day_clamped;
        }
    }
  sel->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
month_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  gtk_date_sel_move_month (sel, d);
}

static void
year_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  struct tm tm;
  guint mdays;
  localtime_r (&sel->time, &tm);
  tm.tm_year += d;
  mdays = days_in_month (&tm);
  if (tm.tm_mday > mdays)
    {
      sel->day_clamped = tm.tm_mday;
      tm.tm_mday = mdays;
    }
  else
    {
      if (sel->day_clamped != -1)
        {
          tm.tm_mday = mdays;
          if (tm.tm_mday > sel->day_clamped)
            tm.tm_mday = sel->day_clamped;
        }
    }
  sel->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
format_text (time_t *time, GtkWidget *w, char *fmt)
{
  struct tm tm;
  char buf[64];
  gchar *sbuf, *sfmt;
  localtime_r (time, &tm);
  sfmt = g_locale_from_utf8 (fmt, -1, NULL, NULL, NULL);
  if (sfmt)
    {
      strftime (buf, sizeof (buf), sfmt, &tm);
      sbuf = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
      if (sbuf)
	{
	  gtk_label_set_text (GTK_LABEL (w), sbuf);
	  g_free (sbuf);
	}
      g_free (sfmt);
    }
}

static void
year_update (GtkDateSel *sel, GtkWidget *w)
{
  format_text (&sel->time, w, _("%Y"));
}

static void
month_update (GtkDateSel *sel, GtkWidget *w)
{
  switch (sel->month_style)
    {
    case GTKDATESEL_MONTH_SHORT:
      format_text (&sel->time, w, _("%b"));
      break;
    case GTKDATESEL_MONTH_LONG:
      format_text (&sel->time, w, _("%B"));
      break;
    case GTKDATESEL_MONTH_NUMERIC:
      format_text (&sel->time, w, _("%m"));
      break;
    case GTKDATESEL_MONTH_ROMAN:
      /* NYS */
      break;
    }
}

static void
week_update (GtkDateSel *sel, GtkWidget *w)
{
  format_text (&sel->time, w, _("Week %V"));
}

static void
day_update (GtkDateSel *sel, GtkWidget *w)
{
  format_text (&sel->time, w, _("%a, %d"));
}

static void
make_field (GtkDateSel *sel, struct elem *e, void (*click)(GtkWidget *, GtkDateSel *),
            void (*update)(GtkDateSel *, GtkWidget *))
{
  GtkWidget *arrow_l = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);
  GtkWidget *arrow_r = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  e->arrow_l = gtk_button_new ();
  e->arrow_r = gtk_button_new ();
  e->text = gtk_label_new ("");

  gtk_widget_set_size_request(e->arrow_l, 20, 20);
  gtk_widget_set_size_request(e->arrow_r, 20, 20);
  GTK_WIDGET_UNSET_FLAGS(e->arrow_l, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS(e->arrow_r, GTK_CAN_FOCUS);
  gtk_widget_show (arrow_l);
  gtk_widget_show (arrow_r);

  gtk_container_add (GTK_CONTAINER (e->arrow_l), arrow_l);
  gtk_container_add (GTK_CONTAINER (e->arrow_r), arrow_r);

  gtk_button_set_relief (GTK_BUTTON (e->arrow_l), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (e->arrow_r), GTK_RELIEF_NONE);

  g_object_set_data (G_OBJECT (e->arrow_l), "direction", (gpointer)0);
  g_object_set_data (G_OBJECT (e->arrow_r), "direction", (gpointer)1);

  g_signal_connect (G_OBJECT (e->arrow_l), "clicked", G_CALLBACK (click), sel);
  g_signal_connect (G_OBJECT (e->arrow_r), "clicked", G_CALLBACK (click), sel);

  gtk_box_pack_start (GTK_BOX (sel->subbox), e->arrow_l, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sel->subbox), e->text, FALSE, FALSE, 1);
  gtk_box_pack_start (GTK_BOX (sel->subbox), e->arrow_r, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (update), e->text);
  update (sel, e->text);
}

static void
gtk_date_sel_init (GtkDateSel *sel)
{
  time (&sel->time);
  sel->day_clamped = -1;

  sel->subbox = gtk_hbox_new (FALSE, 0);

  make_field (sel, &sel->day, day_click, day_update);
  make_field (sel, &sel->week, week_click, week_update);
  make_field (sel, &sel->month, month_click, month_update);
  make_field (sel, &sel->year, year_click, year_update);

  sel->month_style = GTKDATESEL_MONTH_SHORT;

  gtk_widget_show (sel->subbox);
  gtk_box_pack_start (GTK_BOX (sel), sel->subbox, TRUE, FALSE, 0);
}

static void
show_field (struct elem *e)
{
  gtk_widget_show (e->arrow_l);
  gtk_widget_show (e->text);
  gtk_widget_show (e->arrow_r);
}

static void
gtk_date_sel_show (GtkWidget *widget)
{
  GtkDateSel *sel;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DATE_SEL (widget));

  GTK_WIDGET_CLASS (parent_class)->show (widget);

  sel = GTK_DATE_SEL (widget);
  switch (sel->mode)
    {
    case GTKDATESEL_FULL:       /* day, month, year */
      show_field (&sel->day);
      show_field (&sel->month);
      break;
    case GTKDATESEL_WEEK:       /* week, year */
      show_field (&sel->week);
      break;
    case GTKDATESEL_MONTH:      /* month, year */
      show_field (&sel->month);
      break;
    case GTKDATESEL_YEAR:       /* year */
      break;
    }
  show_field (&sel->year);
}

static void
gtk_date_sel_class_init (GtkDateSelClass * klass)
{
  GObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (gtk_hbox_get_type ());
  oclass       = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->show = gtk_date_sel_show;

  my_signals[0] = g_signal_new ("changed",
                                G_OBJECT_CLASS_TYPE (oclass),
                                G_SIGNAL_RUN_LAST,
                                G_STRUCT_OFFSET (GtkDateSelClass, changed),
                                NULL,
                                NULL,
                                g_cclosure_marshal_VOID__VOID,
                                G_TYPE_NONE,
                                0);

}

GType
gtk_date_sel_get_type (void)
{
  static GType date_sel_type = 0;

  if (! date_sel_type)
    {
      static const GTypeInfo date_sel_info =
      {
        sizeof (GtkDateSelClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gtk_date_sel_class_init,
        NULL, /* class finalizer */
        NULL, /* class data */
        sizeof (GtkDateSel),
        0, /* # preallocs */
        (GInstanceInitFunc) gtk_date_sel_init,
      };

      date_sel_type = g_type_register_static (GTK_TYPE_HBOX, "GTKDateSel",
                                              &date_sel_info, 0);
    }
  return date_sel_type;
}

GtkWidget *
gtk_date_sel_new (GtkDateSelMode mode)
{
  GtkWidget *w = GTK_WIDGET (gtk_type_new (gtk_date_sel_get_type ()));
  GTK_DATE_SEL (w)->mode = mode;
  return w;
}

time_t
gtk_date_sel_get_time (GtkDateSel *sel)
{
  return sel->time;
}

void
gtk_date_sel_set_time (GtkDateSel *sel, time_t time)
{
  sel->time = time;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

void
gtk_date_sel_set_day(GtkDateSel *sel, int year, int month, int day)
{
  struct tm tm;
  time_t selected_time;
  localtime_r (&sel->time, &tm);
  tm.tm_year = year;
  tm.tm_mon = month;
  tm.tm_mday = day;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  selected_time = mktime (&tm);
  sel->time = selected_time;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

void
gtk_date_sel_set_month_style (GtkDateSel *sel, GtkDateSelMonthStyle style)
{
  sel->month_style = style;
}
