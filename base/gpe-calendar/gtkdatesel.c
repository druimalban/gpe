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

static void
day_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = gtk_object_get_data (GTK_OBJECT (w), "direction") ? 1 : -1;
  sel->time += d * 60 * 60 * 24;
  sel->day_clamped = -1;
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static void
week_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = gtk_object_get_data (GTK_OBJECT (w), "direction") ? 1 : -1;
  sel->time += d * 60 * 60 * 24 * 7;
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static void
month_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = gtk_object_get_data (GTK_OBJECT (w), "direction") ? 1 : -1;
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
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static void
year_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = gtk_object_get_data (GTK_OBJECT (w), "direction") ? 1 : -1;
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
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static void
format_text (time_t *time, GtkWidget *w, char *fmt)
{
  struct tm tm;
  char buf[64];
  localtime_r (time, &tm);
  strftime (buf, sizeof (buf), fmt, &tm);
  gtk_label_set_text (GTK_LABEL (w), buf);
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

  gtk_widget_show (arrow_l);
  gtk_widget_show (arrow_r);

  gtk_container_add (GTK_CONTAINER (e->arrow_l), arrow_l);
  gtk_container_add (GTK_CONTAINER (e->arrow_r), arrow_r);

  gtk_button_set_relief (GTK_BUTTON (e->arrow_l), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (e->arrow_r), GTK_RELIEF_NONE);

  gtk_object_set_data (GTK_OBJECT (e->arrow_l), "direction", (gpointer)0);
  gtk_object_set_data (GTK_OBJECT (e->arrow_r), "direction", (gpointer)1);

  gtk_signal_connect (GTK_OBJECT (e->arrow_l), "clicked", GTK_SIGNAL_FUNC (click), sel);
  gtk_signal_connect (GTK_OBJECT (e->arrow_r), "clicked", GTK_SIGNAL_FUNC (click), sel);

  gtk_box_pack_start (GTK_BOX (sel), e->arrow_l, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sel), e->text, TRUE, TRUE, 1);
  gtk_box_pack_start (GTK_BOX (sel), e->arrow_r, TRUE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (sel), "changed", GTK_SIGNAL_FUNC (update), e->text);
  update (sel, e->text);
}

static void
gtk_date_sel_init (GtkDateSel *sel)
{
  time (&sel->time);
  sel->day_clamped = -1;

  make_field (sel, &sel->day, day_click, day_update);
  make_field (sel, &sel->week, week_click, week_update);
  make_field (sel, &sel->month, month_click, month_update);
  make_field (sel, &sel->year, year_click, year_update);

  sel->month_style = GTKDATESEL_MONTH_SHORT;
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
    case GTKDATESEL_FULL:	/* day, month, year */
      show_field (&sel->day);
      show_field (&sel->month);
      break;
    case GTKDATESEL_WEEK:	/* week, year */
      show_field (&sel->week);
      break;
    case GTKDATESEL_MONTH:	/* month, year */
      show_field (&sel->month);
      break;
    case GTKDATESEL_YEAR:	/* year */
      break;
    }
  show_field (&sel->year);
}

static void
gtk_date_sel_class_init (GtkDateSelClass * klass)
{
  GtkObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (gtk_hbox_get_type ());
  oclass = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->show = gtk_date_sel_show;

  my_signals[0] = gtk_signal_new ("changed",
				  GTK_RUN_LAST,
#if GTK_MAJOR_VERSION < 2
				  oclass->type,
#else
				  GTK_CLASS_TYPE (oclass),				  
#endif
				  GTK_SIGNAL_OFFSET (GtkDateSelClass, changed),
				  gtk_marshal_NONE__NONE,
				  GTK_TYPE_NONE, 0);

#if GTK_MAJOR_VERSION < 2
  gtk_object_class_add_signals (oclass, my_signals, 1);
#endif
}

GtkType
gtk_date_sel_get_type (void)
{
  static guint date_sel_type = 0;

  if (! date_sel_type)
    {
      static const GtkTypeInfo date_sel_info =
      {
	"GtkDateSel",
	sizeof (GtkDateSel),
	sizeof (GtkDateSelClass),
	(GtkClassInitFunc) gtk_date_sel_class_init,
	(GtkObjectInitFunc) gtk_date_sel_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      date_sel_type = gtk_type_unique (gtk_hbox_get_type (), 
				       &date_sel_info);
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
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

void
gtk_date_sel_set_month_style (GtkDateSel *sel, GtkDateSelMonthStyle style)
{
  sel->month_style = style;
}
