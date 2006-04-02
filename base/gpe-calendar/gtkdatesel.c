/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2006 Philip Blundell <philb@gnu.org>
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

#include <time.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "gtkdatesel.h"
#include <libintl.h>
#include <stdlib.h>

#include "globals.h"

#define _(x) dgettext(PACKAGE, x)

#ifdef IS_HILDON
  #define LABEL_ADD  16
  #define ARROW_SIZE 30
#else
  #define LABEL_ADD   8
  #define ARROW_SIZE 20
#endif

static guint my_signals[1];

struct elem
{
  GtkWidget *container;
  GtkWidget *display;
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
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
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
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
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

static gboolean
year_update (GtkWidget *sel, GtkWidget *entry)
{
  struct tm tm;
  gchar *year;

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);
  year = strftime_strdup_utf8_utf8 (_("%Y"), &tm);
  if (year)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), year);
      g_free (year);
    }

  return FALSE;
}

static gboolean
year_key_press (GtkWidget *entry, GdkEventKey *event, GtkWidget *sel)
{
  int index;
  struct tm tm;

  if (event->keyval != GDK_Return)
    return FALSE;

  index = atoi (gtk_entry_get_text (GTK_ENTRY (entry)));

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);

  if (index >= 1900)
    tm.tm_year = index - 1900;
  else
    /* If e.g. a two digit year, go to with 2000 + index.  */
    tm.tm_year = 100 + index;

  GTK_DATE_SEL (sel)->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);

  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);

  return FALSE;
}

static gboolean
month_change (GtkComboBox *combo, GtkDateSel *sel)
{
  int index;
  struct tm tm;
  guint mdays;

  index = gtk_combo_box_get_active (combo);
  if (index < 0)
    return FALSE;

  localtime_r (&sel->time, &tm);
  tm.tm_mon = index;
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
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

  return FALSE;
}

static void
month_update (GtkDateSel *sel, GtkWidget *combo)
{
  struct tm tm;
  localtime_r (&sel->time, &tm);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), tm.tm_mon);
}

static void
week_update (GtkDateSel *sel, GtkWidget *w)
{
  struct tm tm;
  gchar *sbuf;
  localtime_r (&sel->time, &tm);

  sbuf = strftime_strdup_utf8_utf8 (_("Week %V"), &tm);
  if (sbuf)
    {
      gtk_label_set_text (GTK_LABEL (w), sbuf);
      g_free (sbuf);
    }
}

static gboolean
day_update (GtkWidget *sel, GtkWidget *entry)
{
  struct tm tm;
  gchar *day;

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);
  day = strftime_strdup_utf8_utf8 (_("%d"), &tm);
  if (day)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), day);
      g_free (day);
    }

  return FALSE;
}

static gboolean
day_key_press (GtkWidget *entry, GdkEventKey *event, GtkWidget *sel)
{
  int index;
  struct tm tm;
  guint mdays;

  if (event->keyval != GDK_Return)
    return FALSE;

  index = atoi (gtk_entry_get_text (GTK_ENTRY (entry)));
  if (index <= 0)
    index = 1;

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
  if (index > mdays)
    index = mdays;

  tm.tm_mday = index;
  GTK_DATE_SEL (sel)->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);

  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);

  return FALSE;
}

static gboolean
entry_button_press (GtkWidget *entry, GdkEventKey *event, gpointer data)
{
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
  gtk_widget_grab_focus (entry);
  return TRUE;
}

static int
get_max_month_width (GtkDateSel *sel)
{
  PangoLayout *layout;
  PangoRectangle logical_rect;
  int max_width, i;
  struct tm tm;

  localtime_r (&sel->time, &tm);

  layout = gtk_widget_create_pango_layout (GTK_WIDGET (sel), NULL);

  max_width = 0;
  for (i = 0; i < 11; i++)
    {
      gchar *str = NULL;

      tm.tm_mon = i;

      switch (sel->month_style)
	{
	case GTKDATESEL_MONTH_SHORT:
	  str = strftime_strdup_utf8_utf8 (_("%b"), &tm);
	  break;
	case GTKDATESEL_MONTH_LONG:
	  str = strftime_strdup_utf8_utf8 (_("%B"), &tm);
	  break;
	case GTKDATESEL_MONTH_NUMERIC:
	  str = strftime_strdup_utf8_utf8 (_("%m"), &tm);
	  break;
	case GTKDATESEL_MONTH_ROMAN:
	  /* NYS */
	  break;
	}

      pango_layout_set_text (layout, str, -1);
      g_free (str);

      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      max_width = MAX (max_width, logical_rect.width + LABEL_ADD);
    }

  g_object_unref (layout);

  return max_width;
}

static int
get_max_day_width (GtkDateSel *sel)
{
  PangoLayout *layout;
  PangoRectangle logical_rect;
  int max_width, i, w;
  struct tm tm;
  gchar *str;

  localtime_r (&sel->time, &tm);

  layout = gtk_widget_create_pango_layout (GTK_WIDGET (sel), NULL);

  max_width = 0;
  for (i = 0; i < 31; i++)
    {
      tm.tm_mday = i;

      str = strftime_strdup_utf8_utf8 (_("%d"), &tm);
      pango_layout_set_text (layout, str, -1);
      g_free (str);

      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      max_width = MAX (max_width, logical_rect.width + LABEL_ADD);
    }
  w = max_width;
  
  max_width = 0;
  for (i = 0; i < 7; i++)
    {
      tm.tm_wday = i;

      str = strftime_strdup_utf8_utf8 (_("%a"), &tm);
      pango_layout_set_text (layout, str, -1);
      g_free (str);

      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      max_width = MAX (max_width, logical_rect.width + LABEL_ADD);
    }

  g_object_unref (layout);

  return w + max_width;
}

/* Assumes that E->DISPLAY is valid.  */
static void
make_field (GtkDateSel *sel, struct elem *e,
	    void (*click)(GtkWidget *, GtkDateSel *))
{
  GTK_IS_WIDGET (e->display);

  GtkWidget *arrow_l, *arrow_r, *arrow_button_l, *arrow_button_r;
#ifdef IS_HILDON
  arrow_l = gtk_image_new_from_stock(GTK_STOCK_GO_BACK,
				     GTK_ICON_SIZE_SMALL_TOOLBAR);
  arrow_r = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,
				     GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
  arrow_l = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);
  arrow_r = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
#endif
  arrow_button_l = gtk_button_new ();
  arrow_button_r = gtk_button_new ();
  e->container = gtk_hbox_new (FALSE, 0);

  gtk_widget_set_size_request (arrow_button_l, ARROW_SIZE, ARROW_SIZE);
  gtk_widget_set_size_request (arrow_button_r, ARROW_SIZE, ARROW_SIZE);
  GTK_WIDGET_UNSET_FLAGS (arrow_button_l, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (arrow_button_r, GTK_CAN_FOCUS);

  gtk_container_add (GTK_CONTAINER (arrow_button_l), arrow_l);
  gtk_container_add (GTK_CONTAINER (arrow_button_r), arrow_r);

  gtk_button_set_relief (GTK_BUTTON (arrow_button_l), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (arrow_button_r), GTK_RELIEF_NONE);

  g_object_set_data (G_OBJECT (arrow_button_l), "direction", (gpointer)0);
  g_object_set_data (G_OBJECT (arrow_button_r), "direction", (gpointer)1);

  g_signal_connect (G_OBJECT (arrow_button_l), "clicked", G_CALLBACK (click), sel);
  g_signal_connect (G_OBJECT (arrow_button_r), "clicked", G_CALLBACK (click), sel);

  gtk_box_pack_start (GTK_BOX (e->container), arrow_button_l, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (e->container), e->display, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (e->container), arrow_button_r, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (sel->subbox), e->container, FALSE, FALSE, 0);
}

static void
show_field (struct elem *e)
{
  gtk_widget_show_all (e->container);
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
      show_field (&sel->month);
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
        (GInstanceInitFunc) NULL,
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
  GtkDateSel *sel = GTK_DATE_SEL (w);
  int i;
  gchar buffer[255];

  sel->mode = mode;

  time (&sel->time);
  sel->day_clamped = -1;

  sel->subbox = gtk_hbox_new (FALSE, 0);

  if (mode == GTKDATESEL_WEEK)
    /* Display the week.  */
    {
      struct elem *e = &sel->week;

      e->display = gtk_label_new ("");
      make_field (sel, e, week_click);

      g_signal_connect (G_OBJECT (sel), "changed",
			G_CALLBACK (week_update), e->display);
    }
  if (mode == GTKDATESEL_FULL)
    /* Display the day.  */
    {
      struct elem *e = &sel->day;

      e->display = gtk_entry_new ();
      gtk_entry_set_width_chars (GTK_ENTRY (e->display), 3);
      make_field (sel, e, day_click);

      g_signal_connect (G_OBJECT (sel), "changed",
			G_CALLBACK (day_update),
			e->display);
      g_signal_connect (G_OBJECT (e->display), "key-press-event",
			G_CALLBACK (day_key_press), sel);
      g_signal_connect (G_OBJECT (e->display), "button-press-event",
			G_CALLBACK (entry_button_press), 0);
    }
  if (mode == GTKDATESEL_FULL || mode == GTKDATESEL_MONTH
      || mode == GTKDATESEL_WEEK)
    /* Display the month.  */
    {
      struct elem *e = &sel->month;
      struct tm tm;

      sel->month.display = gtk_combo_box_new_text ();
      make_field (sel, &sel->month, month_click);
      sel->month_style = GTKDATESEL_MONTH_SHORT;

      for (i = 0; i < 12; i ++)
	{
	  gchar *str;

	  tm.tm_mon = i;
	  str = strftime_strdup_utf8_utf8 (_("%b"), &tm);
	  gtk_combo_box_append_text (GTK_COMBO_BOX (e->display), str);
	  g_free (str);
	}

      g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (month_update),
			e->display);
      g_signal_connect (G_OBJECT (e->display), "changed",
			G_CALLBACK (month_change), sel);
    }

  /* Always display the year.  */
  {
      struct elem *e = &sel->year;

      e->display = gtk_entry_new ();
      gtk_entry_set_width_chars (GTK_ENTRY (e->display), 5);
      make_field (sel, e, year_click);

      g_signal_connect (G_OBJECT (sel), "changed",
			G_CALLBACK (year_update),
			e->display);
      g_signal_connect (G_OBJECT (e->display), "key-press-event",
			G_CALLBACK (year_key_press), sel);
      g_signal_connect (G_OBJECT (e->display), "button-press-event",
			G_CALLBACK (entry_button_press), 0);
    }

  gtk_widget_show (sel->subbox);
  gtk_box_pack_start (GTK_BOX (sel), sel->subbox, TRUE, FALSE, 0);

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

  gtk_widget_set_usize (sel->month.display, get_max_month_width (sel), -1);
}
